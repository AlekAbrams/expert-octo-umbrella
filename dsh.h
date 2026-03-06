#define MAXBUF 256  // max number of characteres allowed on command line

// TODO: Function declarations go below

char **split(char *str, char *delim);
void freeArray(char **arr);
char *trim(char *str);
char *resolvePath(char *cmd);
void executeCommand(char *fullPath, char **args, int background);
void builtinCd(char *path);


