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

#ifndef ICP20110203_ERR_H
#define ICP20110203_ERR_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void err(int, const char *, ...);

void errx(int, const char *, ...);

void verr(int, const char *, va_list);

void verrx(int, const char *, va_list);

void warn(const char *, ...);

void warnx(const char *, ...);

void vwarn(const char *, va_list);

void vwarnx(const char *, va_list);

#ifdef __cplusplus
}
#endif

#endif /* !ICP20110203_ERR_H */
