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

#include "procutil.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(HAVE_SYS_WAIT_H)
#include <unistd.h>
#include <sys/wait.h>
#elif defined(HAVE_WINDOWS_H)
#include <windows.h>
#include <sbuffer.h>
#endif

int
spawn_process(char **argv)
{
#if defined(HAVE_SYS_WAIT_H)
    pid_t pid;

    if ( (pid = fork()) == 0 ) {
        execvp(argv[0], argv);
        exit(EXIT_FAILURE);
    } else if ( pid > 0 ) {
        int status;

        if ( waitpid(pid, &status, 0) != -1 ) {
            if ( WIFEXITED(status) ) {
                return WEXITSTATUS(status);
            }
        }
    }

#elif defined(HAVE_WINDOWS_H)
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    string_buffer_t sb;
    char **cursor, *cmd;

    sb_init(&sb, 512);
    sb_add(&sb, argv[0]);
    for ( cursor = &argv[1]; *cursor; cursor++ ) {
        sb_addc(&sb, ' ');
        if ( strchr(*cursor, ' ') ) {
            sb_addf(&sb, "\"%s\"", *cursor);
        } else {
            sb_add(&sb, *cursor);
        }
    }
    cmd = sb_get_copy(&sb);
    free(sb.buffer);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if ( CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) ) {
        DWORD status;

        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &status);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        free(cmd);

        return status;
    }

    free(cmd);

#else
    errno = ENOSYS;

#endif
    return -1;
}
