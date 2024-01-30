/*
 * realpath - Incenp.org Notch Library: realpath replacement
 * Copyright (C) 2024 Damien Goutte-Gattat
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_REALPATH
#ifdef HAVE_WINDOWS_H

#include <windows.h>
#include <stdlib.h>
#include <errno.h>

static long
get_errno(void)
{
    long e;

    switch ( GetLastError() ) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        e = ENOENT;
        break;

    case ERROR_ACCESS_DENIED:
        e = EACCES;
        break;

    case ERROR_INVALID_HANDLE:
        e = EBADF;
        break;

    case ERROR_NOT_ENOUGH_MEMORY:
        e = ENOMEM;
        break;

    default:
        e = ENOSYS;
        break;
    }

    return e;
}

char *
realpath(const char *restrict path, char *restrict resolved_path)
{
    long n;

    if ( ! path ) {
        errno = EINVAL;
        return NULL;
    }

    if ( resolved_path ) {
        /* This implementation only supports allocating its own buffer
         * and does not allow caller to pass its own. */
        errno = EINVAL;
        return NULL;
    }

    if ( (n = GetFullPathName(path, 0, NULL, NULL)) != 0 ) {
        if ( (resolved_path = malloc(n)) != NULL ) {
            if ( GetFullPathName(path, n, resolved_path, NULL) == 0 ) {
                free(resolved_path);
                resolved_path = NULL;

                errno = get_errno();
            }
        }
    } else {
        errno = get_errno();
    }

    return resolved_path;
}

#endif /* HAVE_WINDOWS_H */
#endif /* !HAVE_REALPATH */
