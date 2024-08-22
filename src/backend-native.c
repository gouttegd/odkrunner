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

#include "backend-native.h"

#include <xmem.h>

#include "procutil.h"
#include "util.h"

#if !defined(ODK_RUNNER_WINDOWS)

static int
run(odk_backend_t *backend, odk_run_config_t *cfg, char **command)
{
    int rc;

    (void) backend;

    /* Setting up the environment */
    for ( int j = 0; j < cfg->n_env_vars; j++ ) {
        if ( cfg->env_vars[j].value != NULL )
            setenv(cfg->env_vars[j].name, cfg->env_vars[j].value, 1);
        else
            unsetenv(cfg->env_vars[j].name);
    }

    if ( cfg->flags & ODK_FLAG_TIMEDEBUG || cfg->flags & ODK_FLAG_SEEDMODE ) {
        /* In debug mode, the provided command line must be prefixed
         * with the time command; in seed mode, it must be prefixed with
         * the call to "odk.py seed". */
        char **argv, **cursor;
        size_t n = 0, i = 0;

        if ( cfg->flags & ODK_FLAG_TIMEDEBUG )
            n += 3;
        if ( cfg->flags & ODK_FLAG_SEEDMODE )
            n += 6;

        for ( cursor = &command[0]; *cursor; cursor++ )
            n += 1;

        argv = xmalloc(sizeof(char *) * n);
        if ( cfg->flags & ODK_FLAG_TIMEDEBUG ) {
            argv[i++] = "/usr/bin/time";
            argv[i++] = "-f";
            argv[i++] = "### DEBUG STATS ###\nElapsed time: %E\nPeak memory: %M kb";
        }
        if ( cfg->flags & ODK_FLAG_SEEDMODE ) {
            argv[i++] = "odk.py";   /* We assume the odk.py script is in PATH */
            argv[i++] = "seed";
            argv[i++] = "--gitname";
            argv[i++] = cfg->git_user;
            argv[i++] = "--gitemail";
            argv[i++] = cfg->git_email;
        }
        for ( cursor = &command[0]; *cursor; cursor++ )
            argv[i++] = *cursor;
        argv[i] = NULL;

        rc = spawn_process(argv);
        free(argv);
    } else
        /* We can use the provided command line as it is. */
        rc = spawn_process(command);

    return rc;
}

static int
close(odk_backend_t *backend)
{
    (void) backend;

    return 0;
}

#endif /* !ODK_RUNNER_WINDOWS */

int
odk_backend_native_init(odk_backend_t *backend)
{
#if defined(ODK_RUNNER_WINDOWS)
    errno = ENOSYS;
    return -1;
#else
    backend->prepare = NULL;
    backend->run = run;
    backend->close = close;

    backend->info.total_memory = get_physical_memory();

    return 0;
#endif
}
