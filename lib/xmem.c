/*
 * xmem - Incenp.org Notch Library: die-on-error memory functions
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

#include <xmem.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

void (*xmem_error)(size_t) = NULL;

#define mem_error(a)                                            \
    do {                                                        \
        if ( xmem_error )                                       \
            (*xmem_error)((a));                                 \
        err(EXIT_FAILURE, "Cannot allocate %lu bytes", (a));    \
    } while ( 0 )

void *
xmalloc(size_t s)
{
    void *p;

    if ( ! (p = malloc(s)) && s )
        mem_error(s);

    return p;
}

void *
xcalloc(size_t n, size_t s)
{
    void *p;

    if ( ! (p = calloc(n, s)) && n && s )
        mem_error(n * s);

    return p;
}

void *
xrealloc(void *p, size_t s)
{
    void *np;

    if ( ! (np = realloc(p, s)) && s )
        mem_error(s);

    return np;
}

char *
xstrdup(const char *s)
{
    char *dup;
    size_t len;

    len = strlen(s);
    if ( ! (dup = malloc(len + 1)) )
        mem_error(len + 1);

    strcpy(dup, s);

    return dup;
}

char *
xstrndup(const char *s, size_t n)
{
    char *dup;

    if ( ! (dup = malloc(n + 1)) )
        mem_error(n + 1);

    strncpy(dup, s, n);
    dup[n] = '\0';

    return dup;
}

int
xasprintf(char **s, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = xvasprintf(s, fmt, ap);
    va_end(ap);

    return ret;
}

int
xvasprintf(char **s, const char *fmt, va_list ap)
{
    int n;
    va_list aq;

    va_copy(aq, ap);
    n = vsnprintf(NULL, 0, fmt, aq) + 1;
    va_end(aq);

    if ( ! (*s = malloc(n)) )
        mem_error((size_t) n);

    n = vsnprintf(*s, n, fmt, ap);

    return n;
}
