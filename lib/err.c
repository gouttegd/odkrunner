/*
 * err - Incenp.org Notch Library: (v)(err|warn)(x) replacement
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
#include <string.h>
#include <stdarg.h>

#define ERR_EXIT_ON_ERROR   0x01
#define ERR_PRINT_ERRNO     0x02

const char *getprogname(void);

static void
do_error(int opt, int eval, const char *fmt, va_list ap)
{
    fprintf(stderr, "%s: ", getprogname());
    if ( fmt )
        vfprintf(stderr, fmt, ap);
    if ( opt & ERR_PRINT_ERRNO )
        fprintf(stderr, ": %s", strerror(errno));
    fputc('\n', stderr);

    if ( opt & ERR_EXIT_ON_ERROR )
        exit(eval);
}

void
err(int eval, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    do_error(ERR_EXIT_ON_ERROR | ERR_PRINT_ERRNO, eval, fmt, ap);
    va_end(ap);
}

void
errx(int eval, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    do_error(ERR_EXIT_ON_ERROR, eval, fmt, ap);
    va_end(ap);
}

void
verr(int eval, const char *fmt, va_list ap)
{
    do_error(ERR_EXIT_ON_ERROR | ERR_PRINT_ERRNO, eval, fmt, ap);
}

void
verrx(int eval, const char *fmt, va_list ap)
{
    do_error(ERR_EXIT_ON_ERROR, eval, fmt, ap);
}

void
warn(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    do_error(ERR_PRINT_ERRNO, 0, fmt, ap);
    va_end(ap);
}

void
warnx(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    do_error(0, 0, fmt, ap);
    va_end(ap);
}

void
vwarn(const char *fmt, va_list ap)
{
    do_error(ERR_PRINT_ERRNO, 0, fmt, ap);
}

void
vwarnx(const char *fmt, va_list ap)
{
    do_error(0, 0, fmt, ap);
}
