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

#ifndef ICP20240622_BACKEND_H
#define ICP20240622_BACKEND_H

#include "runner.h"

/* Holds backend-specific data. */
typedef struct odk_backend_info {
    unsigned long total_memory;
} odk_backend_info_t;

typedef struct odk_backend odk_backend_t;

/* Represents a ODK backend, with (1) function pointers to the actual
 * implementations and (2) backend-specific data. */
struct odk_backend {
    odk_backend_info_t info;

    /**
     * Updates the runner configuration with backend-specific infos.
     *
     * @param backend The backend in use.
     * @param cfg     The ODK configuration to update.
     *
     * @return 0 if successful, or -1 if an error occured.
     */
    int   (*prepare)(odk_backend_t *backend, odk_run_config_t *cfg);

    /**
     * Executes a ODK command.
     *
     * @param backend The backend in use.
     * @param cfg     The ODK configuration.
     * @param command The command to execute, as a NULL-terminated array
     *                of arguments.
     *
     * @return 0 if successful, or -1 if an error occured.
     */
    int   (*run)(odk_backend_t *backend, odk_run_config_t *cfg,
                 char **command);

    /*
     * Frees resources associated with the backend.
     *
     * @param backend The backend in use.
     *
     * @return 0 if successful, or -1 if an error occured.
     */
    int   (*close)(odk_backend_t *backend);
};

/**
 * Initialises the backend.
 *
 * @param backend The backend to initialise.
 *
 * @return 0 if successful, or -1 if an error occured.
 */
typedef int (*odk_backend_init)(odk_backend_t *backend);


#endif /* !ICP20240622_BACKEND_H */
