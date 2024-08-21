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

#include "owlapi.h"

#include <string.h>
#include <stdarg.h>

#include <xmem.h>

#define OWLAPI_OPTION_INTEGER   0x01
#define OWLAPI_OPTION_BOOLEAN   0x02
#define OWLAPI_OPTION_ENUM      0x04
#define OWLAPI_OPTION_STRING    0x08

#define OWLAPI_OPTION_NAMESPACE "org.semanticweb.owlapi.model.parameters.ConfigurationOptions"

/*
 * Checks if arg is one of the arguments in the NULL-terminated
 * variable list of arguments. Returns 1 if it is, 0 otherwise.
 */
static int
is_valid_enum_value(char *arg, ...)
{
    va_list ap;
    char *item = NULL;
    int found = 0;

    va_start(ap, arg);
    while ( ! found && (item = va_arg(ap, char *)) )
        if ( strcmp(item, arg) == 0 )
            found = 1;
    va_end(ap);

    return found;
}

/**
 * Parses a key=value pair into a Java property suitable for the OWLAPI.
 *
 * @param[in] arg       The string to parse.
 * @param[out] property If successful, will hold the corresponding Java
 *                      property as a static string.
 * @param[out] value    If successful, will hold the value to assign to
 *                      the property, as a pointer into the 'arg'
 *                      argument.
 * @param[out] error    If an error occurs, will hold an error message
 *                      in a newly allocated buffer.
 * @return 0 upon success, or -1 if an error occurs.
 */
int
get_owlapi_java_property(char *arg, char **property, char **value, char **error)
{
    const char *opt_name = NULL;
    char *endptr;
    int ok = 1;

    if ( ! (*value = strchr(arg, '=')) || *((*value) + 1) == '\0' ) {
        xasprintf(error, "Missing option value for %s", arg);
        return -1;
    }

    *(*value)++ = '\0';

#define OWLAPI_OPTION(symbol, name, type, ...)                              \
    if ( strcmp(name, arg) == 0 ) {                                         \
        opt_name = name;                                                    \
        switch ( type ) {                                                   \
        case OWLAPI_OPTION_INTEGER:                                         \
            *property = OWLAPI_OPTION_NAMESPACE "." #symbol;                \
            strtol(*value, &endptr, 10);                                    \
            ok = *endptr == '\0';                                           \
            break;                                                          \
                                                                            \
        case OWLAPI_OPTION_BOOLEAN:                                         \
            *property = OWLAPI_OPTION_NAMESPACE "." #symbol;                \
            ok = is_valid_enum_value(*value, "true", "false", NULL);        \
            break;                                                          \
                                                                            \
        case OWLAPI_OPTION_ENUM:                                            \
            *property = OWLAPI_OPTION_NAMESPACE "." #symbol;                \
            ok = is_valid_enum_value(*value, __VA_ARGS__);                  \
            break;                                                          \
                                                                            \
        case OWLAPI_OPTION_STRING:                                          \
            *property = OWLAPI_OPTION_NAMESPACE "." #symbol;                \
            break;                                                          \
        }                                                                   \
    }
#include "owlapi-options.h"
#undef OWLAPI_OPTION

    if ( ! opt_name )
        xasprintf(error, "Unknown option %s", arg);
    else if ( ! ok )
        xasprintf(error, "Invalid value '%s' for option %s", *value, opt_name);

    return opt_name && ok ? 0 : -1;
}

/*
 * Prints all values from the NULL-terminated variable list of arguments.
 */
static void
print_enum_values(FILE *f, ...)
{
    va_list ap;
    char *item = NULL;
    int first = 1;

    va_start(ap, f);
    while ( (item = va_arg(ap, char *)) ) {
        fprintf(f, "%s%s", first ? "" : " | ", item);
        first = 0;
    }
    fprintf(f, "\n");
    va_end(ap);
}

/**
 * Prints a list of all allowed OWLAPI options and their expected values.
 *
 * @param f The stream to print the list to.
 */
void
list_owlapi_options(FILE *f)
{
#define OWLAPI_OPTION(symbol, name, type, ...)                              \
    fprintf(f, "%-30s: ", name);                                            \
    switch ( type ) {                                                       \
    case OWLAPI_OPTION_INTEGER:                                             \
        fprintf(f, "<integer>\n");                                          \
        break;                                                              \
                                                                            \
    case OWLAPI_OPTION_BOOLEAN:                                             \
        print_enum_values(f, "true", "false", NULL);                        \
        break;                                                              \
                                                                            \
    case OWLAPI_OPTION_ENUM:                                                \
        print_enum_values(f, __VA_ARGS__);                                  \
        break;                                                              \
                                                                            \
    case OWLAPI_OPTION_STRING:                                              \
        fprintf(f, "<string>\n");                                           \
        break;                                                              \
    }
#include "owlapi-options.h"
#undef OWLAPI_OPTION
}
