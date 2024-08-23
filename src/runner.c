/*
 * ODK Runner
 * Copyright (C) 2024 Damien Goutte-Gattat
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "runner.h"
#include "procutil.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <xmem.h>
#include <memreg.h>
#include <sbuffer.h>

/**
 * Initialises a ODK configuration structure.
 *
 * @param cfg The ODK configuration.
 */
void
odk_init_config(odk_run_config_t *cfg)
{
    cfg->image_name = "obolibrary/odkfull";
    cfg->image_tag = "latest";
    cfg->work_directory = "/work";
    cfg->bindings = NULL;
    cfg->n_bindings = 0;
    cfg->env_vars = NULL;
    cfg->n_env_vars = 0;
    cfg->java_opts = NULL;
    cfg->n_java_opts = 0;
    cfg->flags = 0;
}

/**
 * Frees all resources associated with a ODK configuration.
 *
 * @param cfg The ODK configuration.
 */
void
odk_free_config(odk_run_config_t *cfg)
{
    assert(cfg != NULL);

    if ( cfg-> bindings ) {
        for ( int i = 0; i < cfg->n_bindings; i++ )
            free((char *)cfg->bindings[i].host_directory);
        free(cfg->bindings);
        cfg->bindings = NULL;
        cfg->n_bindings = 0;
    }

    if ( cfg->env_vars ) {
        free(cfg->env_vars);
        cfg->env_vars = NULL;
        cfg->n_env_vars = 0;
    }

    if ( cfg->java_opts ) {
        free(cfg->java_opts);
        cfg->java_opts = NULL;
        cfg->n_java_opts = 0;
    }
}

/**
 * Adds a new binding to the configuration.
 *
 * @param cfg The ODK configuration to update.
 * @param src The path to the host side of the binding.
 * @param dst The path to the container side of the binding. This
 *            pointer must remain valid for the lifetime of the
 *            configuration.
 *
 * @return 0 if successful, or -1 if an error occured when attempting to
 *         canonicalise the src path.
 */
int
odk_add_binding(odk_run_config_t *cfg, const char *src, const char *dst)
{
    char *path;

    assert(cfg != NULL);
    assert(src != NULL);
    assert(dst != NULL);

    if ( ! (path = realpath(src, NULL)) ) {
        /* Do not fail if the path does not exist on the host; assume
         * the users know what they are doing, and simply use the
         * provided path as is; any other error is ground for failure. */
        if ( errno == ENOENT )
            path = xstrdup(src);
        else
            return -1;
    }

    if ( cfg->n_bindings % 10 == 0 )
        cfg->bindings = xrealloc(cfg->bindings, sizeof(odk_bind_config_t) * (cfg->n_bindings + 10));

    cfg->bindings[cfg->n_bindings].host_directory = path;
    cfg->bindings[cfg->n_bindings++].container_directory = dst;

    return 0;
}

/* Common logic to odk_add_env_var and odk_add_java_opt. */
static void
add_var(odk_var_t **vars, size_t *n, const char *name, const char *value)
{
    for ( unsigned i = 0; i < *n; i++ ) {
        if ( strcmp((*vars)[i].name, name) == 0 ) {
            (*vars)[i].value = value;
            return;
        }
    }

    if ( *n % 10 == 0 )
        *vars = xrealloc(*vars, sizeof(odk_var_t) * (*n + 10));

    (*vars)[*n].name = name;
    (*vars)[(*n)++].value = value;
}

/**
 * Adds a new environment variable to the configuration. If the variable
 * already exists (if it has been added by a previous call to this
 * function), the previous value is replaced.
 *
 * @param cfg   The ODK configuration to update.
 * @param name  The name of the new variable.
 * @param value The value of the variable; may be NULL to forcefully
 *              remove an existing value.
 *
 * @note Pointers for both the name and the value must remain valid for
 *       the lifetime of the configuration.
 */
void
odk_add_env_var(odk_run_config_t *cfg, const char *name, const char *value)
{
    assert(cfg != NULL);
    assert(name != NULL);

    add_var(&(cfg->env_vars), &(cfg->n_env_vars), name, value);
}

/**
 * Adds a Java option to the configuration.
 *
 * @param cfg    The ODK configuration to update.
 * @param option The option to add; this should be a valid option as
 *               expected by the java command. The pointer must remain
 *               valid for the lifetime of the configuration.
 */
void
odk_add_java_opt(odk_run_config_t *cfg, const char *option)
{
    assert(cfg != NULL);
    assert(option != NULL);

    add_var(&(cfg->java_opts), &(cfg->n_java_opts), option, NULL);
}

/**
 * Adds a Java system property to the configuration.
 *
 * @param cfg   The ODK configuration to update.
 * @param name  The name of the property to define.
 * @param value The value of the property.
 *
 * @note Pointers for both the name and the value must remain valid for
 *       the lifetime of the configuration.
 */
void
odk_add_java_property(odk_run_config_t *cfg, const char *name, const char *value)
{
    assert(cfg != NULL);
    assert(name != NULL);

    add_var(&(cfg->java_opts), &(cfg->n_java_opts), name, value);
}

/**
 * Compiles all Java options and properties into a string of command
 * line arguments suitable to be passed to a Java virtual machine.
 *
 * @param cfg    The configuration containing the Java options.
 * @param to_env If non-zero, the compiled arguments are added to the
 *               configuration as environment variables.
 *
 * @return A newly allocated buffer containing all the Java command
 *         line arguments. If to_env is true, then that buffer must not
 *         be freed for the lifetime of the configuration.
 */
char *
odk_make_java_args(odk_run_config_t *cfg, int to_env)
{
    char *buffer = NULL;
    size_t needed = 0;
    unsigned i, j;

    for ( i = 0; i < cfg->n_java_opts; i++ ) {
        if ( i > 0 )
            needed += 1;    /* separator */

        needed += strlen(cfg->java_opts[i].name);

        if ( cfg->java_opts[i].value ) {
            needed += 3;    /* -D...= */
            needed += strlen(cfg->java_opts[i].value);
        }
    }

    if ( needed > 0 )
        buffer = xmalloc(needed);

    for ( i = 0, j = 0; i < cfg->n_java_opts; i++ ) {
        if ( i > 0 )
            buffer[j++] = ' ';

        if ( cfg->java_opts[i].value )  /* -D...=... */
            j += sprintf(&(buffer[j]), "-D%s=%s", cfg->java_opts[i].name, cfg->java_opts[i].value);
        else
            j += sprintf(&(buffer[j]), "%s", cfg->java_opts[i].name);
    }

    if ( to_env ) {
        odk_add_env_var(cfg, "ODK_JAVA_OPTS", buffer);
        odk_add_env_var(cfg, "ROBOT_JAVA_ARGS", buffer);
    }

    return buffer;
}
