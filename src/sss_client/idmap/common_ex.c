/*
    Authors:
        Sumit Bose <sbose@redhat.com>

    Copyright (C) 2017 Red Hat

    SSSD's enhanced NSS API

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

#include <time.h>
#include <errno.h>

#include "sss_cli.h"
#include "common_private.h"

#define SEC_FROM_MSEC(ms) ((ms) / 1000)
#define NSEC_FROM_MSEC(ms) (((ms) % 1000) * 1000 * 1000)

/* adopted from timersub() defined in /usr/include/sys/time.h */
#define TIMESPECSUB(a, b, result)                                             \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;                          \
    if ((result)->tv_nsec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_nsec += 1000000000;                                        \
    }                                                                         \
  } while (0)

#define TIMESPEC_TO_MS(ts) (  ((ts)->tv_sec * 1000) \
                            + ((ts)->tv_nsec) / (1000 * 1000) )

int sss_nss_timedlock(unsigned int timeout_ms, int *time_left_ms)
{
    return 0;
}
