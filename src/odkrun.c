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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <locale.h>
#include <errno.h>
#include <err.h>

#include <memreg.h>

#include "runner.h"
#include "util.h"
#include "backend-docker.h"
#include "backend-singularity.h"
#include "backend-native.h"
#include "owlapi.h"


/* Help and information about the program. */

static void
usage(int status)
{
    puts("\
Usage: odkrun [options] [COMMAND...]\n\
Start a ODK container.\n");

    puts("Options:\n\
    -h, --help          Display this help message.\n\
    -v, --version       Display the version message.\n\
");

    puts("\
    -d, --debug         Print debug informations.\n\
    -i, --image NAME    Use the specified image. The default is to use\n\
                        the 'obolibrary/odkfull' image.\n\
    -t, --tag TAG       Use the specified image tag. The default is the\n\
                        'latest' tag.\n\
    -l, --lite          Use the 'obolibrary/odklite' image.\n\
    -s, --singularity   Run the container with Singularity\n\
                        rather than Docker.\n\
    -n, --native        Run in the native system, not in a container.\n\
    --root              When running in a container, run as a superuser.\n\
    --seed              Seed a new ODK repository.\n\
    --gitname           Set Git username to use when seeding.\n\
    --gitemail          Set Git email to use when seeding.\n\
");

    puts("\
    -e, --env NAME=VALUE\n\
                        Pass an environment variable to the container.\n\
    --java-property NAME=VALUE\n\
                        Pass a Java system property to Java programs\n\
                        within the container.\n\
    --owlapi-option NAME=VALUE\n\
                        Pass an option to OWLAPI. Use\n\
                        --owlapi-option=help to list available options.\n\
");

    printf("Report bugs to <%s>.\n", PACKAGE_BUGREPORT);

    exit(status);
}

static void
info(void)
{
    printf("\
odkrun %s\n\
Copyright (C) 2024 Damien Goutte-Gattat\n\
\n\
This program is released under the 3-clause BSD license.\n\
See the COPYING file for more details.\n\
", VERSION);

    exit(EXIT_SUCCESS);
}


/* Helper functions. */

static char *
split_key_value_pair(char *arg, const char *opt_name)
{
    char *value;

    if ( ! (value = strchr(arg, '=')) || *(value + 1) == '\0' )
        errx(EXIT_FAILURE, "Option --%s expects a key=value parameter", opt_name);

    *value++ = '\0';
    return value;
}


#define GH_TOKEN_FILE "ontology-development-kit/github/token"

static void
set_github_token(odk_run_config_t *cfg)
{
    char *token;

    /* First try to get the token from the environment... */
    token = getenv("GH_TOKEN");

    if ( ! token ) {
        char *token_path;

        /* Then try to get it from the current repository... */
        token_path = "../../.github/token.txt";
        if ( file_exists(token_path) == -1 ) {
            char *cfg_dir;

            token_path = NULL;

            /* Then try to get it from a system-wide location. */
#if defined(ODK_RUNNER_LINUX)
            if ( (cfg_dir = getenv("XDG_CONFIG_HOME")) )
                token_path = mr_sprintf(NULL, "%s/" GH_TOKEN_FILE, cfg_dir);
            else if ( (cfg_dir = getenv("HOME")) )
                token_path = mr_sprintf(NULL, "%s/.config/" GH_TOKEN_FILE, cfg_dir);
#elif defined(ODK_RUNNER_MACOS)
            if ( (cfg_dir = getenv("HOME")) )
                token_path = mr_sprintf(NULL, "%s/Library/Application Support/" GH_TOKEN_FILE, cfg_dir);
#elif defined(ODK_RUNNER_WINDOWS)
            if ( (cfg_dir = getenv("LOCALAPPDATA")) )
                token_path = mr_sprintf(NULL, "%s/" GH_TOKEN_FILE ".txt", cfg_dir);
#endif

            if ( file_exists(token_path) == -1 )
                token_path = NULL;
        }

        if ( token_path ) {
            size_t len;

            token = read_file(token_path, &len, 64);
            if ( ! token )
                err(EXIT_FAILURE, "Cannot read Github token file in %s", token_path);

            if ( len > 0 && token[len - 1] == '\n' )
                token[len - 1] = '\0';

            mr_register(NULL, token, 0);
        }
    }

    if ( token )
        odk_add_env_var(cfg, "GH_TOKEN", token);
}

static void
get_git_user_info(odk_run_config_t *cfg)
{
    if ( ! cfg->git_user ) {
        if ( (cfg->git_user = read_line_from_pipe("git config --get user.name")) )
            mr_register(NULL, cfg->git_user, 0);
        else
            err(EXIT_FAILURE, "Cannot get Git user name (needed for --seed)");
    }

    if ( ! cfg->git_email ) {
        if ( (cfg->git_email = read_line_from_pipe("git config --get user.email")) )
            mr_register(NULL, cfg->git_email, 0);
        else
            errx(EXIT_FAILURE, "Cannot get Git user email (needed for --seed)");
    }
}

static void
handle_owlapi_option(odk_run_config_t *cfg, char *option)
{
    char *property, *value, *errmsg;

    if ( strcmp("help", option) == 0 ) {
        list_owlapi_options(stdout);
        exit(0);
    }

    if ( get_owlapi_java_property(option, &property, &value, &errmsg) < 0 )
        errx(EXIT_FAILURE, "Invalid --owlapi-option argument: %s", errmsg);

    odk_add_java_property(cfg, property, value);
}

static int
is_odk_repository(const char *directory)
{
    int ret = 0;

    if ( file_match_exists(directory, "*-odk.yaml") == 0 ) {
        char *path, *p;

        path = realpath(directory, NULL);
#if defined (ODK_RUNNER_WINDOWS)
        if ( (p = strstr(path, "\\src\\ontology")) )
#else
        if ( (p = strstr(path, "/src/ontology")) )
#endif
            ret = *(p + 13) == '\0';

        free(path);
    }

    return ret;
}


/* Main function. */

int
main(int argc, char **argv)
{
    int c;
    int ret = 0;
    char *opt_value;
    odk_run_config_t cfg;
    odk_backend_t backend = { 0 };
    odk_backend_init backend_init = odk_backend_docker_init;

    struct option options[] = {
        { "help",           0, NULL, 'h' },
        { "version",        0, NULL, 'v' },
        { "debug",          0, NULL, 'd' },
        { "image",          1, NULL, 'i' },
        { "tag",            1, NULL, 't' },
        { "lite",           0, NULL, 'l' },
        { "singularity",    0, NULL, 's' },
        { "native",         0, NULL, 'n' },
        { "env",            1, NULL, 'e' },
        { "root",           0, NULL, 256 },
        { "owlapi-option",  1, NULL, 257 },
        { "java-property",  1, NULL, 258 },
        { "seed",           0, NULL, 259 },
        { "gitname",        1, NULL, 260 },
        { "gitemail",       1, NULL, 261 },
        { NULL,             0, NULL, 0 }
    };

    setprogname(argv[0]);
    setlocale(LC_ALL, "");

    odk_init_config(&cfg);

    while ( (c = getopt_long(argc, argv, "hvdi:t:lsne:", options, NULL)) != -1 ) {
        switch ( c ) {
        case 'h':
            usage(EXIT_SUCCESS);
            break;

        case '?':
            usage(EXIT_FAILURE);
            break;

        case 'v':
            info();
            break;

        case 'd':
            cfg.flags |= ODK_FLAG_TIMEDEBUG;
            odk_add_env_var(&cfg, "ODK_DEBUG", "yes");
            break;

        case 'i':
            cfg.image_name = optarg;
            break;

        case 't':
            cfg.image_tag = optarg;
            break;

        case 'l':
            cfg.image_name = "obolibrary/odklite";
            break;

        case 's':
            backend_init = odk_backend_singularity_init;
            break;

        case 'n':
            backend_init = odk_backend_native_init;
            break;

        case 256:
            cfg.flags |= ODK_FLAG_RUNASROOT;
            break;

        case 'e':
            opt_value = split_key_value_pair(optarg, "env");
            odk_add_env_var(&cfg, optarg, opt_value);
            break;

        case 258:
            opt_value = split_key_value_pair(optarg, "java-property");
            odk_add_java_property(&cfg, optarg, opt_value);
            break;

        case 257:
            handle_owlapi_option(&cfg, optarg);
            break;

        case 259:
            cfg.flags |= ODK_FLAG_SEEDMODE;
            break;

        case 260:
            cfg.git_user = optarg;
            break;

        case 261:
            cfg.git_email = optarg;
            break;
        }
    }

    if ( is_odk_repository(".") ) {
        odk_add_binding(&cfg, "../..", "/work");
        cfg.work_directory = "/work/src/ontology";
    }
    else
        odk_add_binding(&cfg, ".", "/work");

    set_github_token(&cfg);

    if ( backend_init(&backend) == -1 )
        err(EXIT_FAILURE, "Cannot initialise backend");

    if ( backend.info.total_memory > 0 ) {
        unsigned long java_mem = backend.info.total_memory * 0.9;
        if ( java_mem > 1024*1024*1024 )
            odk_add_java_opt(&cfg, mr_sprintf(NULL, "-Xmx%luG", java_mem / (1024*1024*1024)));
    }

    if ( cfg.n_java_opts )
        mr_register(NULL, odk_make_java_args(&cfg, 1), 1);

    if ( cfg.flags & ODK_FLAG_SEEDMODE )
        get_git_user_info(&cfg);

    if ( backend.prepare )
        ret = backend.prepare(&backend, &cfg);

    if ( ret == 0 )
        ret = backend.run(&backend, &cfg, &argv[optind]);

    odk_free_config(&cfg);
    backend.close(&backend);

    return ret;
}
