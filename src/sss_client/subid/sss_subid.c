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
#include <stdio.h>  /* for 'FILE' used in 'subid.h' */
#include <shadow/subid.h>


/* Find all subid ranges delegated to a user.
 *
 * Usage in shadow-utils:
 *      libsubid: get_sub?id_ranges() -> list_owner_ranges()
 */
enum subid_status shadow_subid_list_owner_ranges(const char *user,
                                                 enum subid_type id_type,
                                                 struct subid_range **ranges,
                                                 int *count)
{
    if ( !user || !ranges || !count ||
          ((id_type != ID_TYPE_UID) && (id_type != ID_TYPE_GID)) ) {
        return SUBID_STATUS_ERROR;
    }

    /* TODO */

    return SUBID_STATUS_UNKNOWN_USER;
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
    struct subid_range *r;
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

    ret = shadow_subid_list_owner_ranges(owner, id_type, &r, &amount);
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

