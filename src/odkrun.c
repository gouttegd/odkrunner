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
#include <getopt.h>
#include <locale.h>
#include <err.h>

#include "runner.h"


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


/* Main function. */

int
main(int argc, char **argv)
{
    char c;
    int ret;
    odk_run_config_t cfg;

    struct option options[] = {
        { "help",       0, NULL, 'h' },
        { "version",    0, NULL, 'v' },
        { "debug",      0, NULL, 'd' },
        { "image",      1, NULL, 'i' },
        { "tag",        1, NULL, 't' },
        { "lite",       0, NULL, 'l' },
        { NULL,         0, NULL, 0 }
    };

    setprogname(argv[0]);
    setlocale(LC_ALL, "");

    odk_init_config(&cfg);

    while ( (c = getopt_long(argc, argv, "hvdi:t:l", options, NULL)) != -1 ) {
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
        }
    }

    odk_add_binding(&cfg, "../..", "/work");
    odk_add_env_var(&cfg, "ROBOT_JAVA_ARGS", "-Xmx6G");
    odk_add_env_var(&cfg, "JAVA_OPTS", "-Xmx6G");

    ret = odk_run_command(&cfg, &argv[optind]);
    odk_free_config(&cfg);

    return ret;
}
