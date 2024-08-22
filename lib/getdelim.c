/*
 * getdelim - Incenp.org Notch Library: getdelim replacement
 * Copyright (C) 2011 Damien Goutte-Gattat
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef FAIL_ON_ENOMEM
#include <xmem.h>
#endif

#define BLOCK_SIZE  512

ssize_t
getdelim(char **buffer, size_t *len, int delim, FILE *f)
{
    int c;
    unsigned n;

    if ( ! buffer || ! len || ! f ) {
        errno = EINVAL;
        return -1;
    }

    if ( ! *buffer )
        *len = 0;

    c = n = 0;
    while ( c != delim && (c = fgetc(f)) != EOF ) {
        if ( n + 1 >= *len ) {
#ifdef FAIL_ON_ENOMEM
            *buffer = xrealloc(*buffer, *len + BLOCK_SIZE);
#else
            char *p;

            if ( ! (p = realloc(*buffer, *len + BLOCK_SIZE)) )
                return -1;

            *buffer = p;
#endif
            *len += BLOCK_SIZE;
        }

        *(*buffer + n++) = c;
    }

    if ( n > 0 )
        *(*buffer + n) = '\0';

    return n > 0 ? (ssize_t)n : -1;
}
