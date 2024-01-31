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

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(HAVE_SYS_WAIT_H)
#include <unistd.h>
#include <sys/wait.h>
#elif defined(HAVE_WINDOWS_H)
#include <windows.h>
#include <sbuffer.h>
#endif

#include <xmem.h>

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
#if defined(HAVE_SYS_WAIT_H)
    pid_t pid;

    if ( (pid = fork()) == 0 ) {
        char **argv, **cursor;
        size_t n, i = 0;

        n = 9 + (cfg->n_bindings * 2) + (cfg->n_env_vars * 2);
        if ( cfg->flags & ODK_FLAG_TIMEDEBUG ) {
            n += 3;
        }
        for ( cursor = &command[0]; *cursor; cursor++ ) {
            n += 1;
        }
        argv = xmalloc(sizeof(char *) * n);

        argv[i++] = "docker";
        argv[i++] = "run";
        argv[i++] = "--rm";
        argv[i++] = "-ti";
        argv[i++] = "-w";
        argv[i++] = (char *)cfg->work_directory;
        for ( int j = 0; j < cfg->n_bindings; j++ ) {
            argv[i++] = "-v";
            xasprintf(&argv[i++], "%s:%s", cfg->bindings[j].host_directory, cfg->bindings[j].container_directory);
        }
        for ( int j = 0; j < cfg->n_env_vars; j++ ) {
            if ( cfg->env_vars[j].value != NULL ) {
                argv[i++] = "-e";
                xasprintf(&argv[i++], "%s=%s", cfg->env_vars[j].name, cfg->env_vars[j].value);
            }
        }

        xasprintf(&argv[i++], "%s:%s", cfg->image_name, cfg->image_tag);

        if ( cfg->flags & ODK_FLAG_TIMEDEBUG ) {
            argv[i++] = "/usr/bin/time";
            argv[i++] = "-f";
            argv[i++] = "### DEBUG STATS ###\nElapsed time: %E\nPeak memory: %M kb";
        }

        for ( cursor = &command[0]; *cursor; cursor++ ) {
            argv[i++] = *cursor;
        }
        argv[i] = NULL;

        execvp("docker", argv);

    } else if ( pid > 0 ) {
        int status;
        waitpid(pid, &status, 0);
        if ( WIFEXITED(status) ) {
            return WEXITSTATUS(status);
        }
    }
#elif defined(HAVE_WINDOWS_H)
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    string_buffer_t sb;
    char **cursor;

    sb_init(&sb, 512);
    sb_addf(&sb, "docker run --rm -ti -w %s", cfg->work_directory);
    for ( int i = 0; i < cfg->n_bindings; i++ ) {
        sb_addf(&sb, " -v %s:%s", cfg->bindings[i].host_directory, cfg->bindings[i].container_directory);
    }
    for ( int i = 0; i < cfg->n_env_vars; i++ ) {
        if ( cfg->env_vars[i].value != NULL ) {
            sb_addf(&sb, "-e %s=\"%s\"", cfg->env_vars[i].name, cfg->env_vars[i].value);
        }
    }

    sb_addf(&sb, " %s:%s", cfg->image_name, cfg->image_tag);

    if ( cfg->flags & ODK_FLAG_TIMEDEBUG ) {
        sb_add(&sb, " /usr/bin/time -f \"### DEBUG STATS ###\nElapsed time: %E\nPeak memory: %M kb\"");
    }

    for ( cursor = &command[0]; *cursor; cursor++ ) {
        sb_addc(&sb, ' ');
        sb_add(&sb, *cursor);
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if ( CreateProcess(NULL, sb_get_copy(&sb), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) ) {
        DWORD status;

        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &status);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return status;
    }
#endif

    return -1;
}
