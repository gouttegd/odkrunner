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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sbuffer.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define AVAILABLE_SIZE(s)   ((s)->size - (s)->len)
#define CURSOR(s)           ((s)->buffer + (s)->len)

#ifdef FAIL_ON_ENOMEM

#include <xmem.h>

#define GROW(s,l)                                                       \
    do {                                                                \
        (s)->size = ((((s)->len + (l)) / (s)->block) + 1) * (s)->block; \
        (s)->buffer = xrealloc((s)->buffer, (s)->size);                 \
    } while ( 0 )

#define CHECK_ERROR(s)  0

#else

#define GROW(s,l)                                                       \
    do {                                                                \
        size_t ns = ((((s)->len + (l)) / (s)->block) + 1) * (s)->block; \
        char *np = realloc((s)->buffer, ns);                            \
        if ( ! np )                                                     \
            (s)->error = errno;                                         \
        else {                                                          \
            (s)->buffer = np;                                           \
            (s)->size = ns;                                             \
        }                                                               \
    } while ( 0 )

#define CHECK_ERROR(s)  ((s)->error)

#endif/* ! FAIL_ON_ENOMEM */

int
sb_init(string_buffer_t *s, size_t block_size)
{
    if ( ! s ) {
        errno = EINVAL;
        return -1;
    }

    s->buffer = NULL;
    s->len = s->size = 0;
    s->block = block_size ? block_size : 128;
#ifndef FAIL_ON_ENOMEM
    s->error = 0;
#endif

    return 0;
}

const char *
sb_get(string_buffer_t *s)
{
    const char *str = NULL;

    if ( ! s )
        errno = EINVAL;
#ifndef FAIL_ON_ENOMEM
    else if ( CHECK_ERROR(s) )
        errno = s->error;
#endif
    else
        str = s->buffer;

    return str;
}

char *
sb_get_copy(string_buffer_t *s)
{
    char *str = NULL;

    if ( ! s )
        errno = EINVAL;
    else {
#ifdef FAIL_ON_ENOMEM
        str = xmalloc(s->len + 1);
        strncpy(str, s->buffer, s->len + 1);
#else
        if ( CHECK_ERROR(s) )
            errno = s->error;
        else if ( (str = malloc(s->len + 1)) )
            strncpy(str, s->buffer, s->len + 1);
#endif
    }

    return str;
}

int
sb_empty(string_buffer_t *s)
{
    if ( ! s ) {
        errno = EINVAL;
        return -1;
    }

    s->len = 0;
    if ( s->buffer )
        s->buffer[0] = '\0';

    return 0;
}

int
sb_addc(string_buffer_t *s, char c)
{
    if ( ! s ) {
        errno = EINVAL;
        return -1;
    }

    if ( AVAILABLE_SIZE(s) < 2 )
        GROW(s, 2);

    if ( ! CHECK_ERROR(s) ) {
        *(s->buffer + s->len++) = c;
        *(s->buffer + s->len) = '\0';
    }

    return 0;
}

int
sb_add(string_buffer_t *s, const char *buffer)
{
    if ( ! buffer ) {
        errno = EINVAL;
        return -1;
    }

    return sb_addn(s, buffer, strlen(buffer));
}

int
sb_addn(string_buffer_t *s, const char *buffer, size_t len)
{
    if ( ! s || ! buffer ) {
        errno = EINVAL;
        return -1;
    }

    if ( AVAILABLE_SIZE(s) < len + 1)
        GROW(s, len + 1);

    if ( ! CHECK_ERROR(s) ) {
        strncat(CURSOR(s), buffer, len);
        s->len += len;
    }

    return 0;
}

int
sb_vaddf(string_buffer_t *s, const char *fmt, va_list ap)
{
    int written;
    va_list aq;

    if ( ! s || ! fmt ) {
        errno = EINVAL;
        return -1;
    }

    if ( ! s->size )
        GROW(s, s->block);

    if ( CHECK_ERROR(s) )
        return 0;

    va_copy(aq, ap);
    written = vsnprintf(CURSOR(s), AVAILABLE_SIZE(s), fmt, aq);
    va_end(aq);
    if ( written >= (int)AVAILABLE_SIZE(s) ) {
        GROW(s, written);
        if ( CHECK_ERROR(s) )
            return 0;
        vsprintf(CURSOR(s), fmt, ap);
    }
    s->len += written;

    return 0;
}

int
sb_addf(string_buffer_t *s, const char *fmt, ...)
{
    int ret = 0;
    va_list ap;

    if ( ! s || ! fmt ) {
        errno = EINVAL;
        return -1;
    }

    if ( ! CHECK_ERROR(s) ) {
        va_start(ap, fmt);
        ret = sb_vaddf(s, fmt, ap);
        va_end(ap);
    }

    return ret;
}
