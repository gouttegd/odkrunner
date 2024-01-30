/*
 * compat - Incenp.org Notch Library:  header for missing functions
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

#ifndef ICP20110203_COMPAT_H
#define ICP20110203_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif


#if HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME

/*
 * The GNU C library, combined with the GNU linker, automatically
 * stores the program name into the global variable
 * `program_invocation_short_name'. So `setprogname()' can be defined
 * to nothing and `getprogname()' can be aliased to that variable.
 */

#include <errno.h>

#define setprogname(a)
#define getprogname()   program_invocation_short_name

#else

/*
 * If the above-mentioned variable is not available, check for the
 * BSD functions `getprogname()' and `setprogname()', and provide our
 * own implementation if they are missing.
 */

#ifndef HAVE_SETPROGNAME
void
setprogname(const char *);
#endif

#ifndef HAVE_GETPROGNAME
const char *
getprogname(void);
#endif

#endif /* ! HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME */


#ifndef HAVE_REALPATH

char *
realpath(const char *restrict, char *restrict);

#endif


#ifdef __cplusplus
}
#endif

#endif /* !ICP20110203_COMPAT_H */
