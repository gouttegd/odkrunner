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
#include <assert.h>

#include <xmem.h>
#include <memreg.h>

void
odk_init_config(odk_run_config_t *cfg)
{
    cfg->image_name = "obolibrary/odkfull";
    cfg->image_tag = "latest";
    cfg->work_directory = "/work/src/ontology";
    cfg->bindings = NULL;
    cfg->n_bindings = 0;
    cfg->env_vars = NULL;
    cfg->n_env_vars = 0;
    cfg->flags = 0;
}

void
odk_free_config(odk_run_config_t *cfg)
{
    assert(cfg != NULL);

    if ( cfg-> bindings ) {
        for ( int i = 0; i < cfg->n_bindings; i++ ) {
            free((char *)cfg->bindings[i].host_directory);
        }
        free(cfg->bindings);
        cfg->bindings = NULL;
        cfg->n_bindings = 0;
    }

    if ( cfg->env_vars ) {
        free(cfg->env_vars);
        cfg->env_vars = NULL;
        cfg->n_env_vars = 0;
    }
}

void
odk_add_binding(odk_run_config_t *cfg, const char *src, const char *dst)
{
    assert(cfg != NULL);
    assert(src != NULL);
    assert(dst != NULL);

    if ( cfg->n_bindings % 10 == 0 )
        cfg->bindings = xrealloc(cfg->bindings, sizeof(odk_bind_config_t) * (cfg->n_bindings + 10));

    cfg->bindings[cfg->n_bindings].host_directory = realpath(src, NULL);
    cfg->bindings[cfg->n_bindings++].container_directory = dst;
}

void
odk_add_env_var(odk_run_config_t *cfg, const char *name, const char *value)
{
    assert(cfg != NULL);
    assert(name != NULL);

    for ( int i = 0; i < cfg->n_env_vars; i++ ) {
        if ( strcmp(cfg->env_vars[i].name, name) == 0 ) {
            cfg->env_vars[i].value = value;
            return;
        }
    }

    if ( cfg->n_env_vars % 10 == 0 ) {
        cfg->env_vars = xrealloc(cfg->env_vars, sizeof(odk_env_var_t) * (cfg->n_env_vars + 10));
    }

    cfg->env_vars[cfg->n_env_vars].name = name;
    cfg->env_vars[cfg->n_env_vars++].value = value;
}

int
odk_run_command(odk_run_config_t *cfg, char **command)
{
    int rc;
    size_t n, i = 0;
    char **argv, **cursor;
    mem_registry_t mr = { 0 };

    /* Number of tokens in the command line */
    n = 9 + (cfg->n_bindings * 2) + (cfg->n_env_vars * 2);
    if ( cfg->flags & ODK_FLAG_TIMEDEBUG ) {
        n += 3;
    }
    for ( cursor = &command[0]; *cursor; cursor++ ) {
        n += 1;
    }

    /* Assembling the command line */
    argv = mr_alloc(&mr, sizeof(char *) * n);
    argv[i++] = "docker";
    argv[i++] = "run";
    argv[i++] = "--rm";
    argv[i++] = "-ti";
    argv[i++] = "-w";
    argv[i++] = (char *)cfg->work_directory;
    for ( int j = 0; j < cfg->n_bindings; j++ ) {
        argv[i++] = "-v";
        argv[i++] = mr_sprintf(&mr, "%s:%s", cfg->bindings[j].host_directory, cfg->bindings[j].container_directory);
    }
    for ( int j = 0; j < cfg->n_env_vars; j++ ) {
        if ( cfg->env_vars[j].value != NULL ) {
            argv[i++] = "-e";
            argv[i++] = mr_sprintf(&mr, "%s=%s", cfg->env_vars[j].name, cfg->env_vars[j].value);
        }
    }
    argv[i++] = mr_sprintf(&mr, "%s:%s", cfg->image_name, cfg->image_tag);
    if ( cfg->flags & ODK_FLAG_TIMEDEBUG ) {
        argv[i++] = "/usr/bin/time";
        argv[i++] = "-f";
        argv[i++] = "### DEBUG STATS ###\nElapsed time: %E\nPeak memory: %M kb";
    }
    for ( cursor = &command[0]; *cursor; cursor++ ) {
        argv[i++] = *cursor;
    }
    argv[i] = NULL;

    /* Execute */
    rc = spawn_process(argv);
    mr_free(&mr);

    return rc;
}
