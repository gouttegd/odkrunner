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

#include <string.h>

#if defined(ODK_RUNNER_LINUX)
#include <unistd.h> /* for getuid/getgid */
#endif

#include <memreg.h>
#include <sbuffer.h>

#include "procutil.h"
#include "util.h"

#define SINGULARITY_SSH_SOCKET "/run/host-services/ssh-auth.sock"

static int
prepare(odk_backend_t *backend, odk_run_config_t *cfg)
{
    int ret = 0;
    char *ssh_socket;

    if ( (cfg->flags & ODK_FLAG_RUNASROOT) == 0 ) {
#if defined(ODK_RUNNER_LINUX)
        char *user_id = mr_sprintf(NULL, "%u", getuid());
        char *group_id = mr_sprintf(NULL, "%u", getgid());
#else
        char *user_id = "1000";
        char *group_id = "1000";
#endif

        odk_add_env_var(cfg, "ODK_USER_ID", user_id, 0);
        odk_add_env_var(cfg, "ODK_GROUP_ID", group_id, 0);
    }

    if ( (ssh_socket = getenv("SSH_AUTH_SOCK")) ) {
        odk_add_env_var(cfg, "SSH_AUTH_SOCK", SINGULARITY_SSH_SOCKET, 0);
        ret = odk_add_binding(cfg, ssh_socket, SINGULARITY_SSH_SOCKET, 0);
    }

    return ret;
}

static int
run(odk_backend_t *backend, odk_run_config_t *cfg, char **command)
{
    int rc;
    size_t n, i = 0;
    char **argv, **cursor, *image_qualifier;
    mem_registry_t mr = { 0 };
    string_buffer_t sb;

    (void) backend;

    image_qualifier = strchr(cfg->image_name, '/') ? "" : "obolibrary/";

    /* Number of tokens in the command line */
    n = 7;
    if ( cfg->n_bindings > 0 )
        n += 2;
    if ( cfg->n_env_vars > 0 )
        n += 2;
    if ( cfg->flags & ODK_FLAG_TIMEDEBUG )
        n += 3;
    if ( cfg->flags & ODK_FLAG_SEEDMODE )
        n += 2;
    for ( cursor = &command[0]; *cursor; cursor++ )
        n += 1;

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
                if ( j > 0 )
                    sb_addc(&sb, ',');
                sb_addf(&sb, "%s=%s", cfg->env_vars[j].name, cfg->env_vars[j].value);
            }
        }
        argv[i++] = mr_register(&mr, sb_get_copy(&sb), 0);
        sb_empty(&sb);
    }
    if ( cfg->n_bindings > 0 ) {
        argv[i++] = "--bind";
        for ( int j = 0; j < cfg->n_bindings; j++ ) {
            if ( j > 0 )
                sb_addc(&sb, ',');
            sb_addf(&sb, "%s:%s", cfg->bindings[j].host_directory, cfg->bindings[j].container_directory);
        }
        argv[i++] = mr_register(&mr, sb_get_copy(&sb), 0);
        sb_empty(&sb);
    }
    argv[i++] = "-W";
    argv[i++] = (char *)cfg->work_directory;
    argv[i++] = mr_sprintf(&mr, "docker://%s%s:%s", image_qualifier, cfg->image_name, cfg->image_tag);
    if ( cfg->flags & ODK_FLAG_TIMEDEBUG ) {
        argv[i++] = "/usr/bin/time";
        argv[i++] = "-f";
        argv[i++] = "### DEBUG STATS ###\nElapsed time: %E\nPeak memory: %M kb";
    }
    if ( cfg->flags & ODK_FLAG_SEEDMODE ) {
        argv[i++] = "/tools/odk.py";
        argv[i++] = "seed";
    }
    for ( cursor = &command[0]; *cursor; cursor++ )
        argv[i++] = *cursor;
    argv[i] = NULL;

    /* Execute */
    rc = spawn_process(argv);
    mr_free(&mr);
    free(sb.buffer);

    return rc;
}

static
int close_backend(odk_backend_t *backend)
{
    (void) backend;

    return 0;
}

int
odk_backend_singularity_init(odk_backend_t *backend)
{
    backend->prepare = prepare;
    backend->run = run;
    backend->close = close_backend;

    backend->info.total_memory = get_physical_memory();

    return 0;
}
