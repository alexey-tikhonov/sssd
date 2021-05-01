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

#include <subid/subid.h>


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
    return SUBID_STATUS_UNKNOWN_USER;
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
    /* Is this function actually required at all?
     * https://github.com/shadow-maint/shadow/pull/321#issuecomment-831329376
     */
    return SUBID_STATUS_UNKNOWN_USER;
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

