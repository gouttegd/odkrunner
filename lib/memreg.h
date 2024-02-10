/*
 * memreg - Incenp.org Notch Library: local memory registry
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

#ifndef ICP20240131_MEMREG_H
#define ICP20240131_MEMREG_H

#include <stdlib.h>

typedef struct {
    void      **items;
    size_t      count;
} mem_registry_t;

#define MEMREG_FREE_ON_ERROR    0x01

#ifdef __cplusplus
extern "C" {
#endif

void *
mr_register(mem_registry_t *reg, void *ptr, int flags);

void *
mr_alloc(mem_registry_t *reg, size_t size);

void *
mr_realloc(mem_registry_t *reg, void *ptr, size_t size);

char *
mr_strdup(mem_registry_t *reg, const char *s);

char *
mr_sprintf(mem_registry_t *reg, const char *fmt, ...);

void
mr_free(mem_registry_t *reg);

#ifdef __cplusplus
}
#endif

#endif /* !ICP20240131_MEMREG_H */
