/* Some functions call modified from module-init-tools
 * https://github.com/vadmium/module-init-tools.git
 *
 * Copyright (C) 2017        Canonical Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "modprobe_file_parse.h"
#include "hibernate-handler.h"

/*
 * Replace dashes with underscores.
 * Dashes inside character range patterns (e.g. [0-9]) are left unchanged.
 */
char *underscores(char *string)
{
    unsigned int i;

    if (!string)
        return NULL;

    for (i = 0; string[i]; i++) {
        switch (string[i]) {
        case '-':
            string[i] = '_';
            break;

        case ']':
            syslog(LOG_WARNING,"Unmatched bracket in %s\n", string);
            break;

        case '[':
            i += strcspn(&string[i], "]");
            if (!string[i])
                syslog(LOG_WARNING,"Unmatched bracket in %s\n", string);
            break;
        }
    }   
    return string;
}

/**
 * strsep_skipspace - skip over delimitors in strings
 *
 * @string: string to process
 * @delim:  delimitor (e.g. ' ')
 *
 */
static char *strsep_skipspace(char **string, char *delim)
{
    if (!*string)
        return NULL;
    *string += strspn(*string, delim);
    return strsep(string, delim);
}

/*
 * Read one logical line from a configuration file.
 *
 * Line endings may be escaped with backslashes, to form one logical line from
 * several physical lines.  No end of line character(s) are included in the
 * result.
 *
 * If linenum is not NULL, it is incremented by the number of physical lines
 * which have been read.
 */
char *getline_wrapped(FILE *file, unsigned int *linenum)
{
    int size = 256;
    int i = 0;
    char *buf = malloc(size);
    if (buf == NULL)
    {
        syslog (LOG_ERR, "Memory allocate fail\n");
        return NULL;        
    }

    for(;;) {
        int ch = getc_unlocked(file);

        switch(ch) {
        case EOF:
            if (i == 0) {
                free(buf);
                return NULL;
            }
            /* else fall through */

        case '\n':
            if (linenum)
                (*linenum)++;
            if (i == size)
                buf = realloc(buf, size + 1);
                if (buf == NULL)
                {
                    syslog (LOG_ERR, "Memory allocate fail\n");
                    return NULL;
                }
            buf[i] = '\0';
            return buf;

        case '\\':
            ch = getc_unlocked(file);

            if (ch == '\n') {
                if (linenum)
                    (*linenum)++;
                continue;
            }
            /* else fall through */

        default:
            buf[i++] = ch;

            if (i == size) {
                size *= 2;
                buf = realloc(buf, size);
                if (buf == NULL)
				{   
                    syslog (LOG_ERR, "Memory allocate fail\n");
                    return NULL;    
                }
            }
        }
    }
}

/**
 * parse_config_file - read in configuration file options
 *
 * @filename:	name of file
 * @options:     the options parsed in config file
 *
 */
int parse_config_file(const char *filename, char *options)
{
	char *line;
    char *modulename;
	unsigned int linenum = 0;
	FILE *cfile;

	cfile = fopen(filename, "r");
	if (!cfile)
		return EXIT_FAILURE;

	while ((line = getline_wrapped(cfile, &linenum)) != NULL) {
		char *ptr = line;
		char *cmd, *modname;

		cmd = strsep_skipspace(&ptr, "\t ");
		if (cmd == NULL || cmd[0] == '#' || cmd[0] == '\0') {
			free(line);
			continue;
		}

		if (strcmp(cmd, "options") == 0) {
			modname = strsep_skipspace(&ptr, "\t ");
			if (!modname || !ptr)
				goto syntax_error;

			ptr += strspn(ptr, "\t ");
			modulename = underscores(modname);
            if(strcmp(modulename, RS9113_DRIVER_NAME) != 0) {
                free(line);
                continue;
            }
            strncpy(options, ptr, BUFFER_SZ);
            options = strdup(ptr);
            syslog (LOG_NOTICE, "The %s parameter found %s\n",RS9113_DRIVER_NAME, options);

		} else {
syntax_error:
			syslog (LOG_WARNING, "%s line %u: ignoring bad line starting with '%s'\n",filename, linenum, cmd);
		}

		free(line);
	}
	fclose(cfile);
	return EXIT_SUCCESS;
}
