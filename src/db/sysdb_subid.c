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

#include <talloc.h>

#include "db/sysdb_private.h"
#include "db/sysdb_subid.h"

#define SSH_SUBID_SUBDIR "subid_ranges"

errno_t sysdb_store_subid_range(struct sss_domain_info *domain,
                                const char *name,
                                int cache_timeout,
                                time_t now,
                                struct sysdb_attrs *attrs)
{
    TALLOC_CTX *tmp_ctx;
    errno_t ret, sret;
    bool in_transaction = false;

    DEBUG(SSSDBG_TRACE_FUNC, "Storing subid ranges for %s\n", name);

    tmp_ctx = talloc_new(NULL);
    if (!tmp_ctx) {
        return ENOMEM;
    }

    ret = sysdb_transaction_start(domain->sysdb);
    if (ret != EOK) {
        DEBUG(SSSDBG_CRIT_FAILURE, "Failed to start transaction\n");
        goto done;
    }

    in_transaction = true;

    ret = sysdb_attrs_add_string(attrs, SYSDB_OBJECTCLASS, SYSDB_SUBID_RANGE_OC);
    if (ret != EOK) {
        DEBUG(SSSDBG_OP_FAILURE,
              "Could not set object class [%d]: %s\n", ret, strerror(ret));
        goto done;
    }

    ret = sysdb_attrs_add_string(attrs, SYSDB_NAME, name);
    if (ret != EOK) {
        DEBUG(SSSDBG_OP_FAILURE,
              "Could not set name attribute [%d]: %s\n", ret, strerror(ret));
        goto done;
    }

    ret = sysdb_attrs_add_time_t(attrs, SYSDB_LAST_UPDATE, now);
    if (ret != EOK) {
        DEBUG(SSSDBG_OP_FAILURE,
              "Could not set sysdb lastUpdate [%d]: %s\n",
               ret, strerror(ret));
        goto done;
    }

    ret = sysdb_attrs_add_time_t(attrs, SYSDB_CACHE_EXPIRE,
                                 cache_timeout ? (now + cache_timeout) : 0);
    if (ret != EOK) {
        DEBUG(SSSDBG_OP_FAILURE, "Could not set sysdb cache expire [%d]: %s\n",
              ret, strerror(ret));
        goto done;
    }

    ret = sysdb_store_custom(domain, name, SSH_SUBID_SUBDIR, attrs);
    if (ret != EOK) {
        goto done;
    }

    ret = sysdb_transaction_commit(domain->sysdb);
    if (ret != EOK) {
        DEBUG(SSSDBG_CRIT_FAILURE, "Failed to commit transaction\n");
        goto done;
    }

    in_transaction = false;

    ret = EOK;

done:
    if (in_transaction) {
        sret = sysdb_transaction_cancel(domain->sysdb);
        if (sret != EOK) {
            DEBUG(SSSDBG_CRIT_FAILURE, "Could not cancel transaction\n");
        }
    }

    talloc_free(tmp_ctx);

    return ret;
}

