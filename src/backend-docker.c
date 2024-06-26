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

#include "backend-docker.h"

#include <stdio.h>
#include <errno.h>

#include <memreg.h>

#include "procutil.h"

static int
run(odk_backend_t *backend, odk_run_config_t *cfg, char **command)
{
    int rc;
    size_t n, i = 0;
    char **argv, **cursor;
    mem_registry_t mr = { 0 };

    (void) backend;

    /* Number of tokens in the command line */
    n = 9 + (cfg->n_bindings * 2) + (cfg->n_env_vars * 2);
    if ( cfg->flags & ODK_FLAG_TIMEDEBUG )
        n += 3;
    for ( cursor = &command[0]; *cursor; cursor++ )
        n += 1;

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
    for ( cursor = &command[0]; *cursor; cursor++ )
        argv[i++] = *cursor;
    argv[i] = NULL;

    /* Execute */
    rc = spawn_process(argv);
    mr_free(&mr);

    return rc;
}

static int
close(odk_backend_t *backend)
{
    (void) backend;

    return 0;
}

static int
get_total_memory(odk_backend_info_t *info)
{
    FILE *p;
    int ret = -1;

    if ( (p = popen("docker info --format={{.MemTotal}}", "r")) != NULL ) {
        if ( fscanf(p, "%lu", &(info->total_memory)) == 1 )
            ret = 0;
        else
            errno = ESRCH;
        pclose(p);
    }

    return ret;
}


int
odk_backend_docker_init(odk_backend_t *backend)
{
    int ret;

    backend->run = run;
    backend->close = close;

    ret = get_total_memory(&(backend->info));

    return ret;
}
