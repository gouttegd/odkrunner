/*
 * sbuffer - Incenp.org Notch Library: string buffer module
 * Copyright (C) 2011,2018 Damien Goutte-Gattat
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

#ifndef ICP20110203_SBUFFER_H
#define ICP20110203_SBUFFER_H

#include <stdlib.h>
#include <stdarg.h>

typedef struct
{
    char   *buffer;
    size_t  len;
    size_t  size;
    size_t  block;
#ifndef FAIL_ON_ENOMEM
    int     error;
#endif
} string_buffer_t;

#ifdef __cplusplus
extern "C" {
#endif

int
sb_init(string_buffer_t *, size_t);

const char *
sb_get(string_buffer_t *);

char *
sb_get_copy(string_buffer_t *);

int
sb_empty(string_buffer_t *);

int
sb_addc(string_buffer_t *, char);

int
sb_add(string_buffer_t *, const char *);

int
sb_addn(string_buffer_t *, const char *, size_t);

int
sb_vaddf(string_buffer_t *, const char *, va_list);

int
sb_addf(string_buffer_t *, const char *, ...);

#ifdef __cplusplus
}
#endif

#endif /* !ICP20110203_SBUFFER_H */
