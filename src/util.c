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

#include "util.h"

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(ODK_RUNNER_LINUX)
#include <sys/sysinfo.h>
#include <fnmatch.h>

#elif defined(ODK_RUNNER_MACOS)
#include <sys/sysctl.h>
#include <fnmatch.h>

#elif defined(ODK_RUNNER_WINDOWS)
#include <windows.h>

#endif

#include <xmem.h>

/**
 * Gets the amount of physical memory available.
 *
 * @return The total physical memory (in bytes), or 0 if we couldn't get
 *         that information.
 */
size_t
get_physical_memory(void)
{
    size_t phys_mem = 0;

#if defined(ODK_RUNNER_LINUX)
    struct sysinfo info;

    if ( sysinfo(&info) != -1 )
        phys_mem = info.totalram;

#elif defined(ODK_RUNNER_MACOS)
    int mib_name[] = { CTL_HW, HW_MEMSIZE };
    size_t len = sizeof(phys_mem);

    if ( sysctl(mib_name, 2, &phys_mem, &len, NULL, 0) == -1 )
        phys_mem = 0;

#elif defined(ODK_RUNNER_WINDOWS)
    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof(statex);
    if ( GlobalMemoryStatusEx(&statex) != 0 )
        phys_mem = statex.ullTotalPhys;

#endif

    return phys_mem;
}

/**
 * Checks if the specified file exists.
 *
 * @param path The filename to check.
 *
 * @return 0 if the file exists, -1 otherwise (check errno for details).
 */
int
file_exists(const char *path)
{
    struct stat st;

    assert(path != NULL);

    return stat(path, &st);
}

/**
 * Checks if a file matching the specified pattern exists.
 *
 * @param directory The directory where to search for the file.
 * @param pattern   The pattern to look for.
 *
 * @return 0 if a matching file is found, -1 otherwise.
 */
int
file_match_exists(const char *directory, const char *pattern)
{
    int found = 0;

#if defined(ODK_RUNNER_WINDOWS)
    char *search = NULL;
    WIN32_FIND_DATA find_data;
    HANDLE handle;

    if ( *directory == '.' && *(directory + 1) == '\0' )
        search = (char *) pattern;
    else
        xasprintf(&search, "%s/%s", directory, pattern);

    if ( (handle = FindFirstFile(search, &find_data)) != INVALID_HANDLE_VALUE ) {
        found = 1;
        FindClose(handle);
    }

    if ( search != pattern )
        free(search);
#else
    DIR *dir;

    if ( (dir = opendir(directory)) ) {
        struct dirent *entry;

        while ( ! found && (entry = readdir(dir)) )
            found = fnmatch(pattern, entry->d_name, 0) == 0;

        closedir(dir);
    }
#endif /* ! ODK_RUNNER_WINDOWS */

    return found ? 0 : -1;
}

/**
 * Gets the size of the specified file.
 *
 * @param f The file object.
 *
 * @return The file's size, or -1 if an error occured (check errno for
 *         details).
 */
long
get_file_size(FILE *f)
{
    long s;

    assert(f != NULL);

    if ( fseek(f, 0, SEEK_END) == -1
            || (s = ftell(f)) == -1
            || fseek(f, 0, SEEK_SET) == -1 )
        s = -1;

    return s;
}

/**
 * Reads a file into memory.
 *
 * @param filename The path to the file to read.
 * @param len      The address where the file's size will be stored
 *                 (may be NULL).
 * @param max      Do not read the file if its size exceeds this value;
 *                 if zero, always read the file no matter its size.
 *
 * @return A newly allocated buffer containing the file's data, or
 *         NULL if an error occured (check errno for details).
 */
char *
read_file(const char *filename, size_t *len, size_t max)
{
    FILE *f;
    char *blob = NULL;

    assert(filename != NULL);

    if ( (f = fopen(filename, "r")) ) {
        long size;
        size_t nread;

        if ( (size = get_file_size(f)) != -1 ) {
            if ( max != 0 && (unsigned long) size > max )
                errno = EFBIG;
            else {
                blob = xmalloc(size);
                if ( (nread = fread(blob, 1, size, f)) < (unsigned long) size ) {
                    free(blob);
                    blob = NULL;
                } else if ( len )
                    *len = nread;
            }
        }

        fclose(f);
    }

    return blob;
}

/**
 * Reads a single line from a pipe.
 *
 * @param command The command to open a pipe from.
 *
 * @return A newly allocated buffer containing the first line of output
 *         from the command (not including any newline character), or
 *         NULL if an error occured (check errno for details).
 */
char *
read_line_from_pipe(const char *command)
{
    char *line = NULL;
    size_t n = 0;
    FILE *p;

    if ( (p = popen(command, "r")) ) {
        if ( getline(&line, &n, p) != -1 ) {
            n = strlen(line);
            if ( line[n - 1] == '\n' )
                line[n - 1] = '\0';
        }
        pclose(p);
    }

    return line;
}
