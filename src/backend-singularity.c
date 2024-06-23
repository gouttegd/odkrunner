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

#include "backend-singularity.h"

#include <memreg.h>
#include <sbuffer.h>

#include "procutil.h"

static int
run(odk_backend_t *backend, odk_run_config_t *cfg, char **command)
{
    int rc;
    size_t n, i = 0;
    char **argv, **cursor;
    mem_registry_t mr = { 0 };
    string_buffer_t sb;

    (void) backend;

    /* Number of tokens in the command line */
    n = 7;
    if ( cfg->n_bindings > 0 ) {
        n += 2;
    }
    if ( cfg->n_env_vars > 0 ) {
        n += 2;
    }
    if ( cfg->flags & ODK_FLAG_TIMEDEBUG ) {
        n += 3;
    }
    for ( cursor = &command[0]; *cursor; cursor++ ) {
        n += 1;
    }

    /* Assembling the command line */
    argv = mr_alloc(&mr, sizeof(char *) * n);
    sb_init(&sb, 512);
    argv[i++] = "singularity";
    argv[i++] = "exec";
    argv[i++] = "--cleanenv";
    if ( cfg->n_env_vars > 0 ) {
        argv[i++] = "--env";
        for ( int j = 0; j < cfg->n_env_vars; j++ ) {
            if ( cfg->env_vars[j].value != NULL ) {
                if ( j > 0 ) {
                    sb_addc(&sb, ',');
                }
                sb_addf(&sb, "%s=%s", cfg->env_vars[j].name, cfg->env_vars[j].value);
            }
        }
        argv[i++] = mr_register(&mr, sb_get_copy(&sb), 0);
        sb_empty(&sb);
    }
    if ( cfg->n_bindings > 0 ) {
        argv[i++] = "--bind";
        for ( int j = 0; j < cfg->n_bindings; j++ ) {
            if ( j > 0 ) {
                sb_addc(&sb, ',');
            }
            sb_addf(&sb, "%s:%s", cfg->bindings[j].host_directory, cfg->bindings[j].container_directory);
        }
        argv[i++] = mr_register(&mr, sb_get_copy(&sb), 0);
        sb_empty(&sb);
    }
    argv[i++] = "-W";
    argv[i++] = (char *)cfg->work_directory;
    argv[i++] = mr_sprintf(&mr, "docker://%s:%s", cfg->image_name, cfg->image_tag);
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
    free(sb.buffer);

    return rc;
}

static
int close(odk_backend_t *backend)
{
    (void) backend;

    return 0;
}

int
odk_backend_singularity_init(odk_backend_t *backend)
{
    backend->run = run;
    backend->close = close;

    return 0;
}
