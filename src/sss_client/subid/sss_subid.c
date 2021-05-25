/*
    Copyright (C) 2021 Red Hat

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include "subid.h" /* #include <subid/subid.h> */
#include "sss_cli.h"


struct range {
	unsigned long start;
	unsigned long count;
};

typedef struct subordinate_range *subid_range_ptr;


/* SUBID_RANGES Reply:
 *
 * 0-3: 32bit unsigned number of UID results
 * 4-7: 32bit unsigned number of GID results
 * For each result (sub-uid ranges first):
 * 0-3: 32bit number with "start" id
 * 4-7: 32bit number with "count" (range size)
 */
static enum subid_status _list_owner_ranges(const char *user,
                                            enum subid_type id_type,
                                            struct range **ranges,
                                            int *count)
{
    size_t user_len;
    enum sss_status ret;
    uint8_t *repbuf = NULL;
    size_t index = 0;
    size_t replen;
    int errnop;
    struct sss_cli_req_data rd;
    uint32_t num_results = 0;
    uint32_t val;


    if ( !user || !ranges || !count ||
          ((id_type != ID_TYPE_UID) && (id_type != ID_TYPE_GID)) ) {
        return SUBID_STATUS_ERROR;
    }

    ret = sss_strnlen(user, SSS_NAME_MAX, &user_len);
    if (ret != 0) {
        return SUBID_STATUS_UNKNOWN_USER;
    }
    rd.len = user_len + 1;
    rd.data = user;

    sss_nss_lock();
    /* Anticipated workflow will always request both
     * sub-uid and sub-gid ranges anyway.
     * So don't bother with dedicated commands -
     * just request everything in one shot.
     * The second request will get data from the cache
     */
    ret = sss_cli_make_request_with_checks(SSS_NSS_GET_SUBID_RANGES, &rd,
                                           SSS_CLI_SOCKET_TIMEOUT,
                                           &repbuf, &replen, &errnop,
                                           SSS_NSS_SOCKET_NAME);
    sss_nss_unlock();

    if (ret != SSS_STATUS_SUCCESS) {
        /* TODO: check 'errnop' and return SUBID_STATUS_ERROR_CONN if appropriate */
        return SUBID_STATUS_ERROR;
    }

    SAFEALIGN_COPY_UINT32(&num_results, repbuf, NULL);
    if (id_type == ID_TYPE_UID) {
        index = 2 * sizeof(uint32_t);
    } else {
        index = (2 + 2*num_results) * sizeof(uint32_t);
        SAFEALIGN_COPY_UINT32(&num_results, repbuf + sizeof(uint32_t), NULL);
    }
    if (num_results == 0) {
        /* TODO: how to distinguish "user not found" vs "user found but doesn't have ranges defined" here?
         * Options:
         *  - special "fake" entry in the cache
         *  - provide 'nss_protocol_done_fn' to 'nss_getby_name' to avoid "ENOENT -> "empty packet" logic
         *  - add custom error code for this case and handle in generic 'nss_protocol_done'
         */
        free(repbuf);
        return SUBID_STATUS_UNKNOWN_USER;
    }

    *count = num_results;
    if (*count < 0) {
        free(repbuf);
        return SUBID_STATUS_ERROR;
    }

    *ranges = malloc(num_results * sizeof(struct range));
    if (!*ranges) {
        free(repbuf);
        return SUBID_STATUS_ERROR;
    }

    for (uint32_t c = 0; c < num_results; ++c) {
        SAFEALIGN_COPY_UINT32(&val, repbuf + index, &index);
        (*ranges)[c].start = val;
        SAFEALIGN_COPY_UINT32(&val, repbuf + index, &index);
        (*ranges)[c].count = val;
    }
    free(repbuf);

    return SUBID_STATUS_SUCCESS;
}


/* Find all subid ranges delegated to a user.
 *
 * Usage in shadow-utils:
 *      libsubid: get_sub?id_ranges() -> list_owner_ranges()
 */
enum subid_status shadow_subid_list_owner_ranges(const char *owner,
                                                 enum subid_type id_type,
                                                 struct subordinate_range ***ranges,
                                                 int *count)
{
    enum subid_status ret;
    struct range *r;

    if (!ranges) {
        return SUBID_STATUS_ERROR;
    }

    ret = _list_owner_ranges(owner, id_type, &r, count);
    if ((ret != SUBID_STATUS_SUCCESS) || (*count == 0)) {
        return ret;
    }

    /* see:
     * https://github.com/shadow-maint/shadow/pull/321#issuecomment-831374237
     * https://github.com/shadow-maint/shadow/pull/321#issuecomment-831997503
     */

    *ranges = (subid_range_ptr *)malloc(*count * sizeof(subid_range_ptr));
    if (!*ranges) {
        free(r);
        return SUBID_STATUS_ERROR;
    }

    for (int i = 0; i < *count; ++i) {
        (*ranges)[i] = (subid_range_ptr)malloc(sizeof(struct subordinate_range));
        if (!(*ranges)[i]) { /* TODO: de-allocate ranges */
            ret = SUBID_STATUS_ERROR;
            break;
        }
        (*ranges)[i]->start = r[i].start;
        (*ranges)[i]->count = r[i].count;
        (*ranges)[i]->owner = strdup(owner);
        if (!(*ranges)[i]->owner) { /* TODO: de-allocate ranges */
            ret = SUBID_STATUS_ERROR;
            break;
        }
    }

    free(r);
    return ret;
}


/* Does a user own a given subid range?
 *
 * Usage in shadow-utils:
 *      newuidmap/user busy : have_sub_uids() -> has_range()
 */
enum subid_status shadow_subid_has_range(const char *owner,
                                         unsigned long start,
                                         unsigned long count,
                                         enum subid_type id_type,
                                         bool *result)
{
    enum subid_status ret;
    struct range *r;
    int amount;
    unsigned long end = start + count;

    if (!result || (end < start)) {
        return SUBID_STATUS_ERROR;
    }

    if (count == 0) {
        *result = true;
        return SUBID_STATUS_SUCCESS;
    }

    /* Anticipated workflow is the following:
     *
     * 1) Podman figures out ranges available for a user:
     *     libsubid::get_subid_ranges() -> ... -> list_owner_ranges()
     *
     * 2) Podman maps available ranges:
     *     newuidmap -> have_sub_uids() -> has_range()
     * At this point all ranges are available in a cache from step (1)
     * so it doesn't make sense to try "smart" LDAP searches (even if possible)
     * Let's just reuse list_owner_ranges() and do a check.
     *
     * It might have some sense to do a check at responder's side (i.e. without
     * fetching all ranges), but range is just a couple of numbers (and FreeIPA
     * only support a single range per user anyway), so this optimization
     * wouldn't save much traffic anyway, but would introduce new
     * `sss_cli_command`/responder handler.
     */

    ret = _list_owner_ranges(owner, id_type, &r, &amount);
    if (ret != SUBID_STATUS_SUCCESS) {
        return ret;
    }

    *result = false;

    for (int i = 0; i < amount; ++i) {
        if ((r[i].start <= start) && (r[i].start + r[i].count >= end)) {
            *result = true;
        }
        /* TODO: handle coverage via multiple ranges */
    }

    free(r);
    return ret;
}


/* Does a user own any subid range?
 *
 * Usage in shadow-utils:
 *      newusers: sub_uid_assigned() -> has_any_range()
 */
enum subid_status shadow_subid_has_any_range(const char *owner,
                                             enum subid_type id_type,
                                             bool *result)
{
    enum subid_status ret;
    struct range *r = NULL;
    int count;

    /* Is this function actually required at all?
     * https://github.com/shadow-maint/shadow/pull/321#issuecomment-831329376
     */

    if (!result) {
        return SUBID_STATUS_ERROR;
    }

    ret = _list_owner_ranges(owner, id_type, &r, &count);

    if (ret == SUBID_STATUS_SUCCESS) {
        *result = (count > 0);
    }

    free(r);
    return ret;
}


/* Find uids who own a given subid.
 *
 * Usage in shadow-utils:
 *      libsubid: get_sub?id_owners() -> find_subid_owners()
 */
enum subid_status shadow_subid_find_subid_owners(unsigned long subid,
                                                 enum subid_type id_type,
                                                 uid_t **uids,
                                                 int *count)
{
    /* Not yet implemented.
     * Currently there are no users of this function.
     */
    return SUBID_STATUS_ERROR;
}

