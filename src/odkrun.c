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
#include "runconf.h"


/* Help and information about the program. */

static void
usage(int status)
{
    puts("\
Usage: odkrun [options] [seed|COMMAND...]\n\
Start a ODK container.\n");

    puts("General options:\n\
    -h, --help          Display this help message.\n\
    -v, --version       Display the version message.\n\
    -d, --debug         Print debug informations.\n\
");

    puts("Image options:\n\
    -i, --image NAME    Use the specified image. The default is to use\n\
                        the 'obolibrary/odkfull' image.\n\
    -t, --tag TAG       Use the specified image tag. The default is the\n\
                        'latest' tag.\n\
    -l, --lite          Use the 'obolibrary/odklite' image. This is\n\
                        equivalent to '--image obolibrary/odklite'.\n\
");

    puts("Backend options:\n\
    -s, --singulary     Run the container with Singularity rather\n\
                        than Docker (experimental).\n\
    -n, --native        Run in the native system, not in a container\n\
                        (VERY experimental).\n\
        --root          Run as a superuser within the container.\n\
");

    puts("Passing settings to the container:\n\
    -e, --env NAME=VALUE\n\
                        Pass an environment variable.\n\
        --java-property NAME=VALUE\n\
                        Pass a Java system property to Java programs\n\
                        (mostly ROBOT) within the container.\n\
        --owlapi-option NAME=VALUE\n\
                        Pass an option to the OWLAPI library. To list\n\
                        available options, use '--owlapi-option=help'.\n\
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


/* Helper functions for command line processing. */

/* Splits a key=value pair around the '=' sign; arg is updated in place
 * so that it only contains the key, while a pointer to the value is
 * returned. */
static char *
split_key_value_pair(char *arg, const char *opt_name)
{
    char *value;

    if ( ! (value = strchr(arg, '=')) || *(value + 1) == '\0' )
        errx(EXIT_FAILURE, "Option --%s expects a key=value parameter", opt_name);

    *value++ = '\0';
    return value;
}

/* Checks that option is a valid OWLAPI option and updates the ODK
 * configuration accordingly. */
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

    odk_add_java_property(cfg, property, value, 0);
}


/* Helper functions to configure the ODK. */

#define GH_TOKEN_FILE "ontology-development-kit/github/token"

/* Configures the ODK to use a GitHub token. */
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
        odk_add_env_var(cfg, "GH_TOKEN", token, 0);
}

/* Configures the ODK with a Git username and email. */
static void
set_git_user(odk_run_config_t *cfg)
{
    char *git_user = NULL, *git_email = NULL;

    if ( ! (git_user = getenv("GIT_AUTHOR_NAME")) )
        if ( (git_user = read_line_from_pipe("git config --get user.name")) )
            mr_register(NULL, git_user, 0);

    if ( ! (git_email = getenv("GIT_AUTHOR_EMAIL")) )
        if ( (git_email = read_line_from_pipe("git config --get user.email")) )
            mr_register(NULL, git_email, 0);

    if ( git_user ) {
        odk_add_env_var(cfg, "GIT_AUTHOR_NAME", git_user, 0);
        odk_add_env_var(cfg, "GIT_COMMITTER_NAME", git_user, 0);
    }

    if ( git_email ) {
        odk_add_env_var(cfg, "GIT_AUTHOR_EMAIL", git_email, 0);
        odk_add_env_var(cfg, "GIT_COMMITTER_EMAIL", git_email, 0);
    }
}

/* Checks whether the specified directory is a ODK repository. */
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

/* Sets and binds the ODK working directory. */
static void
set_work_directory(odk_run_config_t *cfg)
{
    char *cwd = ".";

    if ( is_odk_repository(cwd) ) {
        cwd = "../..";
        cfg->work_directory = "/work/src/ontology";
    }

    if ( odk_add_binding(cfg, cwd, "/work", 0) == -1 )
        err(EXIT_FAILURE, "Cannot bind directory '%s'", cwd);
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
        { NULL,             0, NULL, 0 }
    };

    setprogname(argv[0]);
    setlocale(LC_ALL, "");

    odk_init_config(&cfg);

    while ( (c = getopt_long(argc, argv, "+hvdi:t:lsne:", options, NULL)) != -1 ) {
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
            odk_add_env_var(&cfg, "ODK_DEBUG", "yes", 0);
            break;

        case 'i':
            odk_set_image_name(&cfg, optarg, 0);
            break;

        case 't':
            odk_set_image_tag(&cfg, optarg, 0);
            break;

        case 'l':
            odk_set_image_name(&cfg, "obolibrary/odklite", 0);
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
            odk_add_env_var(&cfg, optarg, opt_value, 0);
            break;

        case 258:
            opt_value = split_key_value_pair(optarg, "java-property");
            odk_add_java_property(&cfg, optarg, opt_value, 0);
            break;

        case 257:
            handle_owlapi_option(&cfg, optarg);
            break;
        }
    }

    if ( load_run_conf(&cfg) == -1 )
        err(EXIT_FAILURE, "Cannot load run.sh.conf");

    if ( optind < argc && strcmp("seed", argv[optind]) == 0 ) {
        cfg.flags |= ODK_FLAG_SEEDMODE;
        optind += 1;
        set_git_user(&cfg);
    }

    if ( backend_init(&backend) == -1 )
        err(EXIT_FAILURE, "Cannot initialise backend");

    if ( backend.info.total_memory > 0 && (cfg.flags & ODK_FLAG_JAVAMEMSET) == 0 ) {
        unsigned long java_mem = backend.info.total_memory * 0.9;
        if ( java_mem > 1024*1024*1024 )
            odk_add_java_opt(&cfg, mr_sprintf(NULL, "-Xmx%luG", java_mem / (1024*1024*1024)), 0);
    }

    if ( cfg.n_java_opts )
        mr_register(NULL, odk_make_java_args(&cfg, 1), 1);

    set_work_directory(&cfg);
    set_github_token(&cfg);

    if ( backend.prepare )
        ret = backend.prepare(&backend, &cfg);

    if ( ret == 0 )
        ret = backend.run(&backend, &cfg, &argv[optind]);

    odk_free_config(&cfg);
    backend.close(&backend);

    return ret;
}
