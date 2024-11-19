#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <string.h>
#include "commod.h"

typedef enum {INTERACT, COMM_ARG, SCRIPT_ARG} exec_mode;

#define PROMPT "$ "

#define Last(X) ((X)[strlen(X) - 1])

void get_user_input(string, string);
int decouper(string, char *, string*, int);
void usage(string);
exec_mode analyse_ldc(int, string*);
string parse_redir(string*, char*, string*, int(*)[], string);
void make_redir(char, string*, int(*)[]);
void exec_comm_suite(string, int*, string);
void suppr_antislash(string);
void input_heredoc(int*, string);
