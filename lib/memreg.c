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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <memreg.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#ifdef FAIL_ON_ENOMEM
#include <xmem.h>
#endif


/*
 * "Global" registry, to be used if no local registry is passed.
 */
static mem_registry_t mr_global_registry = { 0 };
static int cleanup_registered = 0;

#define MAYBE_GLOBAL(r)                                         \
    do {                                                        \
        if ( ! (r) ) {                                          \
            (r) = &mr_global_registry;                          \
            if ( ! cleanup_registered ) {                       \
                atexit(cleanup_global_registry);                \
                cleanup_registered = 1;                         \
            }                                                   \
        }                                                       \
    } while ( 0 )

static void
cleanup_global_registry(void)
{
    mr_free(&mr_global_registry);
}


/**
 * Register a pre-allocated buffer.
 *
 * @param[in] reg   The registry to use (NULL to use the global registry).
 * @param[in] ptr   The buffer to register. If NULL, this function does
 *                  nothing.
 * @param[in] flags If set to MEMREG_FREE_ON_ERROR, the buffer is freed
 *                  if registration fails.
 *
 * @return The registered buffer (same as the 'ptr' parameter), or NULL
 *         if the buffer could not be registered.
 */
void *
mr_register(mem_registry_t *reg, void *ptr, int flags)
{
    MAYBE_GLOBAL(reg);

    if ( ptr ) {
        if ( reg->count % 10 == 0 ) {
#ifdef FAIL_ON_ENOMEM
            reg->items = xrealloc(reg->items, (reg->count + 10) * sizeof(void*));
#else
            void **tmp = realloc(reg->items, (reg->count + 10) * sizeof(void*));
            if ( ! tmp ) {
                if ( flags & MEMREG_FREE_ON_ERROR )
                    free(ptr);
                return NULL;
            }
            reg->items = tmp;
#endif
        }

        reg->items[reg->count++] = ptr;
    }

    return ptr;
}


/**
 * Allocate a new buffer on the registry.
 *
 * @param[in] reg   The registry to use (NULL to use the global registry).
 * @param[in] size  The size of the buffer to allocate.
 *
 * @return The newly registered buffer, or NULL if the buffer could not
 *         be registered.
 */
void *
mr_alloc(mem_registry_t *reg, size_t size)
{
    void *p;

#ifdef FAIL_ON_ENOMEM
    p = xmalloc(size);
#else
    p = malloc(size);
#endif

    return mr_register(reg, p, MEMREG_FREE_ON_ERROR);
}


/**
 * Resize a buffer on the registry. If the buffer was not already on the
 * registry, it is registered after being resized.
 *
 * @param[in] reg   The registry to use (NULL to use the global registry).
 * @param[in] ptr   The buffer to resize. If NULL, a new buffer is
 *                  allocated and registered.
 * @param[in] size  The new size of the buffer.
 *
 * @param The newly resized buffer, or NULL if it could not be resized
 *        or registered (if needed).
 */
void *
mr_realloc(mem_registry_t *reg, void *ptr, size_t size)
{
    int i;
    void *p;

    if ( ! size ) {
        /* Resizing to zero to mimic free() is not allowed! */
        errno = EINVAL;
        return NULL;
    }

    MAYBE_GLOBAL(reg);

    /* Find the buffer in the registry */
    for ( i = 0; i < reg->count && reg->items[i] != ptr; i++ ) ;

#ifdef FAIL_ON_ENOMEM
    p = xrealloc(ptr, size);
#else
    p = realloc(ptr, size);
    if ( p ) {
#endif
        if ( i == reg->count ) {
            /* The buffer was not registered, do it now */
            p = mr_register(reg, p, MEMREG_FREE_ON_ERROR);
        }
        else
            reg->items[i] = p;
#ifndef FAIL_ON_ENOMEM
    }
#endif

    return p;
}


/**
 * Duplicate a string into a newly registered buffer.
 *
 * @param[in] reg   The registry to use (NULL to use the global registry).
 * @param[in] s     The string to copy.
 *
 * @return The newly registered buffer, or NULL if a buffer could not
 *         be registered.
 */
char *
mr_strdup(mem_registry_t *reg, const char *s)
{
    char *dup;

#ifdef FAIL_ON_ENOMEM
    dup = xstrdup(s);
#else
    dup = strdup(s);
#endif

    return mr_register(reg, dup, MEMREG_FREE_ON_ERROR);
}


/**
 * Print to a newly registred buffer.
 *
 * @param[in] reg   The registry to use (NULL to use the global registry).
 * @param[in] fmt   The format string.
 * @param[in] ...   The data to format.
 *
 * @return A pointer to the registered buffer containing the formatted
 *         string, or NULL if a buffer could not be registered.
 */
char *
mr_sprintf(mem_registry_t *reg, const char *fmt, ...)
{
    int n;
    char *p;
    va_list ap;

    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap) + 1;
    va_end(ap);

    if ( (p = mr_alloc(reg, n)) ) {
        va_start(ap, fmt);
        vsnprintf(p, n, fmt, ap);
        va_end(ap);
    }

    return p;
}


/**
 * Free the entire registry. This frees the individual buffers as well
 * as the registry itself.
 *
 * @param[in] reg   The registry to free (NULL to free the global registry).
 */
void
mr_free(mem_registry_t *reg)
{
    if ( ! reg )
        reg = &mr_global_registry;

    for ( int i = 0; i < reg->count; i++ )
        free(reg->items[i]);

    if ( reg->items )
        free(reg->items);

    reg->items = NULL;
    reg->count = 0;
}
