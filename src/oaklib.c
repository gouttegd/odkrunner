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

#include "oaklib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "runner.h"

#define OAKLIB_NAME "oaklib"
#define USER_CACHEDIR "/home/odkuser/.data/oaklib"
#define ROOT_CACHEDIR "/root/.data/oaklib"


/*
 * Gets the path to the data directory used by OAK.
 * OAK stores its cached data into a directory obtained by Pystow,
 * so we need to replicate Pystow's logic to determine where that
 * directory is:
 *
 * 1. Is OAKLIB_HOME set?
 *    Then use its value directly.
 *
 * 2. Is PYSTOW_HOME set?
 *    Then use $PYSTOW_HOME/oaklib.
 *
 * 3. Is PYSTOW_USE_APPDIRS set to "true" or "True"?
 *    Then:
 *    * on GNU/Linux: use $XDG_DATA_HOME/oaklib if XDG_DATA_HOME is set,
 *                    or $HOME/.local/share/oaklib otherwise;
 *    * on macOS:     use "$HOME/Library/Application Support/oaklib";
 *    * on Windows:   use $LOCALAPPDATA/oaklib.
 *
 * 4. Is PYSTOW_NAME set?
 *    Then:
 *    * on Windows:      use $USERPROFILE/$PYSTOW_NAME;
 *    * everywhere else: use $HOME/$PYSTOW_NAME.
 *
 * 5. Otherwise:
 *    * on Windows:      use $USERPROFILE/.data;
 *    * everywhere else: use $HOME/.data.
 *
 * @param buffer The buffer to store the path into.
 * @param len    The size of the buffer.
 *
 * @return The length of the path written into the buffer. If it is
 *         equal or greater than len, it means the buffer was too
 *         small to contain the full path. May return -1 if we were
 *         not able to obtain the user's home directory.
 */
static int
get_oaklib_cache_directory(char *buffer, size_t len)
{
    int ret = - 1;
    char *dir, *use_appdirs, *pystow_name;

    if ( (dir = getenv("OAKLIB_HOME")) )
        ret = snprintf(buffer, len, "%s", dir);
    else {
        if ( (dir = getenv("PYSTOW_HOME")) )
            ret = snprintf(buffer, len, "%s/" OAKLIB_NAME, dir);
        else {
            if ( (use_appdirs = getenv("PYSTOW_USE_APPDIRS")) && strcasecmp(use_appdirs, "true") == 0 ) {
#if defined(ODK_RUNNER_WINDOWS)
                if ( (dir = getenv("LOCALAPPDATA")) )
                    ret = snprintf(buffer, len, "%s/" OAKLIB_NAME, dir);
#elif defined(ODK_RUNNER_MACOS)
                if ( (dir = getenv("HOME")) )
                    ret = snprintf(buffer, len, "%s/Library/Application Support/" OAKLIB_NAME, dir);
#else
                if ( (dir = getenv("XDG_DATA_DIR")) )
                    ret = snprintf(buffer, len, "%s/" OAKLIB_NAME, dir);
                else if ( (dir = getenv("HOME")) )
                    ret = snprintf(buffer, len, "%s/.local/share/" OAKLIB_NAME, dir);
#endif
            } else {    /* No PYSTOW_USE_APPDIRS */
                if ( ! (pystow_name = getenv("PYSTOW_NAME")) )
                    pystow_name = ".data";

#if defined(ODK_RUNNER_WINDOWS)
                dir = getenv("USERPROFILE");
#else
                dir = getenv("HOME");
#endif

                if ( dir )
                    ret = snprintf(buffer, len, "%s/%s/" OAKLIB_NAME, dir, pystow_name);
            }
        }
    }

    return ret;
}

#define CACHE_PATH_MAX  2048

/**
 * Configures the runner to share a host-side OAK cache directory with
 * the ODK container.
 *
 * @param cfg The configuration to update.
 * @param dir The cache directory to share, or 'user' to share the
 *            user's own cache, or 'repo' to share a directory from
 *            within the current ODK repository.
 *
 * @return 0 if no error (including no-op), -1 if we couldn't get the
 *         user's cache directory.
 */
int
share_oaklib_cache(odk_run_config_t *cfg, const char *dir)
{
    int ret = 0;
    char cache_dir[CACHE_PATH_MAX], *dest_dir;

    dest_dir = (cfg->flags & ODK_FLAG_RUNASROOT) > 0 ? ROOT_CACHEDIR : USER_CACHEDIR;

    if ( strcasecmp(ODK_SHARING_OAKLIB_USER_CACHE, dir) == 0 ) {
        if ( get_oaklib_cache_directory(cache_dir, CACHE_PATH_MAX) >= CACHE_PATH_MAX ) {
            ret = -1;
            errno = ENAMETOOLONG;
        } else
            odk_add_binding(cfg, cache_dir, dest_dir, 0);
    } else if ( strcasecmp(ODK_SHARING_OAKLIB_REPO_CACHE, dir) == 0 ) {
        /* Only effective when within an ODK repo, otherwise ignored. */
        if ( (cfg->flags & ODK_FLAG_INODKREPO) > 0 ) {
            /* No need to bind anything since the cache directory is
             * already shared along with the rest of the repository.
             * All we need to do is to tell Pystow to use that
             * directory. */
            odk_add_env_var(cfg, "OAKLIB_HOME", "/work/src/ontology/tmp/oaklib", 0);
        }
    } else {
        /* Arbitrary cache dir, we pass it as it is. */
        odk_add_binding(cfg, dir, dest_dir, 0);
    }

    return ret;
}
