/*
 * main.c
 *
 *  Created on: Mar 17 2017
 *      Author: david
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <string.h>
#include "dsh.h"

#define MAX_PROC 1024

extern int thisIsGlobal;

int main(int argc, char *argv[]) {

	// DO NOT REMOVE THE BLOCK BELOW (FORK BOMB PREVENTION) //
	struct rlimit limit;
	limit.rlim_cur = MAX_PROC;
	limit.rlim_max = MAX_PROC;
	setrlimit(RLIMIT_NPROC, &limit);
	// DO NOT REMOVE THE BLOCK ABOVE THIS LINE //


	char *cmdline = malloc(MAXBUF); // buffer to store user input from commmand line
	    if (cmdline == NULL) {
        perror("malloc");
        return 1;
    }
	while (1) {

        // print shell prompt
        printf("dsh> ");
        fflush(stdout);

        // Read input
        if (fgets(cmdline, MAXBUF, stdin) == NULL) {
            // Ctrl-D): exit cleanly
            printf("\n");
            break;
        }

        // trailing newline -> null terminator
        cmdline[strcspn(cmdline, "\n")] = '\0';
        char *line = trim(cmdline);

        // Ignore blank lines and prompt again
        if (strlen(line) == 0) {
            continue;
        }

        // exit
        if (strcmp(line, "exit") == 0) {
            free(cmdline);
            exit(0);
        }

        // pwd
        if (strcmp(line, "pwd") == 0) {
            char cwd[MAXBUF];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("getcwd");
            }
            continue;
        }

        // cd path

        if (strcmp(line, "cd") == 0 ||
            (strncmp(line, "cd ", 3) == 0)) {

            // get just argument
            char *arg = (strlen(line) > 3) ? trim(line + 3) : "";
            builtinCd(arg);
            continue;
        }

        // Detect &
        int background = 0;
        int lineLen = strlen(line);
        if (lineLen >= 2 && line[lineLen - 1] == '&' && line[lineLen - 2] == ' ') {
            background = 1;
            line[lineLen - 2] = '\0';   // strip & from the end
            line = trim(line);          // trim again to get rid of extra spaces
        }

        // Tokenise the command line on whitespace
        char **tokens = split(line, " ");
        if (tokens == NULL || tokens[0] == NULL) {
            freeArray(tokens);
            continue;
        }

		char *cmd = tokens[0];  // command name or path

        // size execv args array
        int numTokens = 0;
        while (tokens[numTokens] != NULL) {
            numTokens++;
        }

        // first mode: full path
        if (cmd[0] == '/') {
            if (access(cmd, F_OK | X_OK) == 0) {
                // args array: [fullPath, arg1, ..., NULL]
                char **args = malloc((numTokens + 1) * sizeof(char *));
                for (int i = 0; i < numTokens; i++) {
                    args[i] = tokens[i];
                }
                args[numTokens] = NULL;

                executeCommand(cmd, args, background);
                free(args);
            } else {
                fprintf(stderr, "ERROR: %s not found!\n", cmd);
            }
            freeArray(tokens);
            continue;
        }

        // second mode: resolve path
        char *fullPath = NULL;
        // check current working directory
        char cwd[MAXBUF];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            int candidateLen = strlen(cwd) + 1 + strlen(cmd) + 1;
            char *candidate = malloc(candidateLen);
            if (candidate == NULL) {
                perror("malloc");
                exit(1);
            }
            snprintf(candidate, candidateLen, "%s/%s", cwd, cmd);

            if (access(candidate, F_OK | X_OK) == 0) {
                fullPath = candidate;   // found in cwd
            } else {
                free(candidate);
            }
        }

        // search every directory in path
        if (fullPath == NULL) {
            fullPath = resolvePath(cmd);
        }

        // execute if found, otherwise error
        if (fullPath != NULL) {
            // Build args
            char **args = malloc((numTokens + 1) * sizeof(char *));
            if (args == NULL) {
                perror("malloc");
                exit(1);
            }
            args[0] = fullPath;
            for (int i = 1; i < numTokens; i++) {
                args[i] = tokens[i];
            }
            args[numTokens] = NULL;

            executeCommand(fullPath, args, background);

            free(args);
            free(fullPath);
        } else {
            fprintf(stderr, "ERROR: %s not found!\n", cmd);
        }

        freeArray(tokens);
    }

    free(cmdline);
    return 0;
}

