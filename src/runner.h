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

#ifndef ICP20240128_RUNNER_H
#define ICP20240128_RUNNER_H

#include <stdlib.h>

typedef struct odk_bind_config {
    const char *host_directory;
    const char *container_directory;
} odk_bind_config_t;

typedef struct odk_var {
    const char *name;
    const char *value;
} odk_var_t;

typedef struct odk_run_config {
    const char         *image_name;
    const char         *image_tag;
    const char         *work_directory;
    odk_bind_config_t  *bindings;
    size_t              n_bindings;
    odk_var_t          *env_vars;
    size_t              n_env_vars;
    size_t              n_java_opts;
    odk_var_t          *java_opts;
    char               *git_user;
    char               *git_email;
    unsigned            flags;
} odk_run_config_t;

typedef int (*odk_run_command)(odk_run_config_t *, char**);

#define ODK_FLAG_TIMEDEBUG  0x0001
#define ODK_FLAG_RUNASROOT  0x0002
#define ODK_FLAG_SEEDMODE   0x0004

#ifdef __cplusplus
extern "C" {
#endif

void
odk_init_config(odk_run_config_t *cfg);

void
odk_free_config(odk_run_config_t *cfg);

void
odk_add_binding(odk_run_config_t *cfg, const char *src, const char *dst);

void
odk_add_env_var(odk_run_config_t *cfg, const char *name, const char *value);

void
odk_add_java_opt(odk_run_config_t *cfg, const char *option);

void
odk_add_java_property(odk_run_config_t *cfg, const char *name, const char *value);

char *
odk_make_java_args(odk_run_config_t *cfg, int);

#ifdef __cplusplus
}
#endif

#endif /* !ICP20240128_RUNNER_H */
