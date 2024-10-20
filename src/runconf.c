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

#include "runconf.h"

#include <string.h>
#include <err.h>

#include <xmem.h>

#include "util.h"
#include "memreg.h"
#include "owlapi.h"

#define RUNCONF_FILENAME "run.sh.conf"


/**
 * Parses a volume binding specification (of the kind expected by
 * Docker's -v option, e.g. "/host/path:/container/path").
 *
 * @param spec   The specification to parse.
 * @param lineno The line number from the configuration file.
 * @param cfg    The configuration object to update.
 *
 * @return 0 if successful, -1 if the specification is invalid.
 */
static int
process_bind_spec(char *spec, size_t lineno, odk_run_config_t *cfg)
{
    char *dst, *options, *tmp = NULL;

    dst = strchr(spec, ':');
#if defined (ODK_RUNNER_WINDOWS)
    /* We need to ignore the ':' after the drive letter */
    if ( dst == spec + 1 && *(dst + 1) != '\0' )
        dst = strchr(dst + 1, ':');
#endif

    if ( ! dst || *(dst + 1) == '\0' || *(dst + 1) == ':' ) {
        warnx(RUNCONF_FILENAME ":%lu:Ignoring invalid \"ODK_BINDS\" value \"%s\"", lineno, spec);
        return -1;
    }

    *dst++ = '\0';
    if ( (options = strchr(dst, ':')) ) {
        /* TODO: Bind options are not supported yet */
        warnx(RUNCONF_FILENAME ":%lu:Ignoring unsupported binding option for \"%s:%s\"", lineno, spec, dst);
        *options++ = '\0';
    }

    if ( *spec == '~' ) {
        char *home;

#if defined (ODK_RUNNER_WINDOWS)
        if ( (home = getenv("USERPROFILE")) ) {
#else
        if ( (home = getenv("HOME")) ) {
#endif
            xasprintf(&tmp, "%s%s", home, spec + 1);
            spec = tmp;
        } else {
            warnx(RUNCONF_FILENAME ":%lu:Cannot expand '~' in binding \"%s:%s\"", lineno, spec, dst);
            return -1;
        }
    }

    if ( odk_add_binding(cfg, spec, mr_strdup(NULL, dst)) == -1 ) {
        warn(RUNCONF_FILENAME ":%lu:Cannot add binding \"%s:%s\"", lineno, spec, dst);
        return -1;
    }

    if ( tmp )
        free(tmp);

    return 0;
}


/**
 * Parses a single line from a run.sh.conf file.
 *
 * @param line   A buffer containing the line to parse. The buffer will
 *               be reused or freed at the end of parsing, so its
 *               content should be copied to another buffer if needed.
 * @param len    The length of the line.
 * @param lineno The line number, from the beginning of the file.
 * @param cfg    The configuration object to update.
 *
 * @return 0 if successful, or -1 upon a configuration error.
 */
static int
process_line(char *line, size_t len, size_t lineno, odk_run_config_t *cfg)
{
    int ret = 0;

#define DO_WARN(msg, ...)                                           \
    do {                                                            \
        warnx(RUNCONF_FILENAME ":%lu:" msg, lineno,  __VA_ARGS__);   \
        ret = -1;                                                   \
    } while ( 0 )

    if ( len > 0 && line[0] != '#' ) {
        char *value;

        if ( ! (value = strchr(line, '=')) )
            DO_WARN("Ignoring value-less option \"%s\"", line);
        else {
            size_t value_len;

            *value++ = '\0';
            value_len = len - (value - line);

            /* Remove enclosing quotes around the value, if any */
            if ( (*value == '\'' || *value == '\"') && *(value + value_len - 1) == *value ) {
                *(value + value_len - 1) = '\0';
                value += 1;
                value_len -= 2;
            }

            if ( value_len == 0 )
                DO_WARN("Ignoring empty value for option \"%s\"", line);
            else if ( strcmp(line, "ODK_IMAGE") == 0 )
                cfg->image_name = mr_strdup(NULL, value);
            else if ( strcmp(line, "ODK_TAG") == 0 )
                cfg->image_tag = mr_strdup(NULL, value);
            else if ( strcmp(line, "ODK_DEBUG") == 0 && strcmp(value, "yes") == 0 ) {
                cfg->flags |= ODK_FLAG_TIMEDEBUG;
                odk_add_env_var(cfg, "ODK_DEBUG", "yes");
            } else if ( strcmp(line, "ODK_JAVA_OPTS") == 0 ) {
                char * token;

                while ( (token = strtok(value, " ")) ) {
                    odk_add_java_opt(cfg, mr_strdup(NULL, token));
                    value = NULL;
                }
            } else if ( strcmp(line, "ODK_BINDS") == 0 ) {
                char *token;

                while ( (token = strtok(value, ",")) ) {
                    ret += process_bind_spec(token, lineno, cfg);
                    value = NULL;
                }
            } else if ( strncmp(line, "OWLAPI_", 7) == 0 ) {
                char *property, *errmsg = NULL;

                if ( get_owlapi_java_property_from_name(line + 7, value, &property, &errmsg) != -1 )
                    odk_add_java_property(cfg, property, mr_strdup(NULL, value));
                else {
                    DO_WARN("Ignoring invalid OWLAPI option \"%s=%s\": %s", line + 7, value, errmsg);
                    free(errmsg);
                }
            } else if ( strcmp(line, "ODK_USER_ID") == 0 ) {
                /* We only support the case where ID == 0,
                 * to run as the super-user. */
                if ( *value == '0' && *(value + 1) == '\0' )
                    cfg->flags |= ODK_FLAG_RUNASROOT;
                else
                    DO_WARN("Ignoring \"ODK_USER_ID\" with value other than 0", NULL);
            } else
                DO_WARN("Ignoring unsupported option \"%s\"", line);
        }
    }

    return ret;
}

/**
 * Reads a run.sh configuration file.
 *
 * @param cfg The configuration to update from the file.
 *
 * @return
 * - -1 if an error occured when attempting to read the file (check
 *   errno for details);
 * - 0 if there was no configuration file to read;
 * - 1 if the file was successfully read;
 * - >= 2 if configuration errors were found in the file.
 */
int
load_run_conf(odk_run_config_t *cfg)
{
    int ret = 0;

    if ( file_exists(RUNCONF_FILENAME) == 0 ) {
        FILE *f;

        if ( (f = fopen(RUNCONF_FILENAME, "r")) ) {
            char *line = NULL;
            size_t n = 0, line_nr = 0;
            ssize_t len;

            while ( ! feof(f) && ret != -1 ) {
                len = getline(&line, &n, f);

                if ( len != -1 ) {
                    line_nr += 1;

                    /* Remove line terminator */
                    if ( len > 0 && line[len - 1] == '\n' ) {
                        line[--len] = '\0';
                    }

                    ret += -process_line(line, len, line_nr, cfg);
                } else if ( ! feof(f) )
                    ret = -1;   /* Error when reading the line */
            }

            if ( line )
                free(line);

            fclose(f);
        } else
            ret = -1;
    }

    return ret;
}
