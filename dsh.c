/*
 * dsh.c
 *
 *  Created on: Aug 2, 2013
 *      Author: chiu
 */
#include "dsh.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/types.h>
#include <errno.h>
#include <err.h>
#include <sys/stat.h>
#include <string.h>


// TODO: Your function definitions below (declarations in dsh.h)

//split()

char **split(char *str, char *delim) {
    if (str == NULL || delim == NULL) {
        return NULL;
    }

    //count delimiters
    int numTokens = 1;
    char *ptr = str;
    int delimLen = strlen(delim);
    while ((ptr = strstr(ptr, delim)) != NULL) {
        numTokens++;
        ptr += delimLen;
    }

    // allocate the pointer array
    char **array = malloc((numTokens + 1) * sizeof(char *));
    if (array == NULL) {
        perror("malloc");
        exit(1);
    }

    // strtok messes with it, so make copy
    char *copy = malloc(strlen(str) + 1);
    if (copy == NULL) {
        perror("malloc");
        exit(1);
    }
    strcpy(copy, str);

    // tokenise + fill each slot
    int i = 0;
    char *token = strtok(copy, delim);
    while (token != NULL) {
        array[i] = malloc(strlen(token) + 1);
        if (array[i] == NULL) {
            perror("malloc");
            exit(1);
        }
        strcpy(array[i], token);
        i++;
        token = strtok(NULL, delim);
    }

    // NULL-terminate
    array[i] = NULL;

    free(copy);
    return array;
}


//FreeArray()
void freeArray(char **arr) {
    if (arr == NULL) {
        return;
    }
    for (int i = 0; arr[i] != NULL; i++) {
        free(arr[i]);
    }
    free(arr);
}


//trim()
char *trim(char *str) {
    if (str == NULL) {
        return NULL;
    }

    // Skip leading whitespace
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }

    // Nothing left after skipping
    if (*str == '\0') {
        return str;
    }

    // get rid of trailing whitespace w null terminator
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }

    return str;
}


//resolvePath()
char *resolvePath(char *cmd) {
    char *pathEnv = getenv("PATH");
    if (pathEnv == NULL) {
        return NULL;
    }

    // copy because strtok messes with it
    char *pathCopy = malloc(strlen(pathEnv) + 1);
    if (pathCopy == NULL) {
        perror("malloc");
        exit(1);
    }
    strcpy(pathCopy, pathEnv);

    // Buffer for candidate full path
    char *fullPath = malloc(MAXBUF * 2);
    if (fullPath == NULL) {
        perror("malloc");
        exit(1);
    }

    char *dir = strtok(pathCopy, ":");
    while (dir != NULL) {
        snprintf(fullPath, MAXBUF * 2, "%s/%s", dir, cmd);
        if (access(fullPath, F_OK | X_OK) == 0) {
            free(pathCopy);
            return fullPath;    // caller must free REMEMBER ALEK
        }
        dir = strtok(NULL, ":");
    }

    free(pathCopy);
    free(fullPath);
    return NULL;
}


//executeCommand()
void executeCommand(char *fullPath, char **args, int background) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        // Child
        execv(fullPath, args);
        // execv only returns if it failed
        perror("execv");
        exit(1);
    } else {
        // Parent
        if (!background) {
            int status;
            waitpid(pid, &status, 0);   // block until child finishes
        }
    }
}


//builtinCd()
void builtinCd(char *path) {
    const char *dest;

    if (path == NULL || strlen(path) == 0) {
        // No argument given means go to $HOME
        dest = getenv("HOME");
        if (dest == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
    } else {
        dest = path;
    }

    if (chdir(dest) != 0) {
        fprintf(stderr, "%s: no such file or directory\n", dest);
    }
}





