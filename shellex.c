/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline, int in_fd, int out_fd);
void eval_pipeline(char *cmdline, int out_fd);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
        /* Read */
        printf("> ");                   
        fgets(cmdline, MAXLINE, stdin); 
        if (feof(stdin))
            exit(0);

        /* Evaluate */
        eval_pipeline(cmdline, STDOUT_FILENO);
    } 
}
/* $end shellmain */

int contains_pipeline(char* cmdline) {
    int quote = 0;
    for (int i=0; cmdline[i]!=0; i++) {
        if (quote && cmdline[i] == '\"') quote = 0;
        else {
            if (cmdline[i] == '\"') quote = 1;
            else if (cmdline[i] == '|') return 1;
        }
    }
    return 0;
}

// get the righttmost command
char* get_last_command(char *cmdline) { 
    int quote = 0;
    int len = strlen(cmdline);
    for (int i=len-1; i>=0; i--) {
        if (quote && cmdline[i]=='\"') quote = 0;
        else {
            if (cmdline[i] == '\"') quote = 1;
            else if (cmdline[i] == '|') {
                cmdline[i] = '\0';
                return cmdline + i + 1;
            }
        }
    }
}

void eval_pipeline(char *cmdline, int out_fd) {
    char* cmd;
    int pipefd[2], status;
    if (!contains_pipeline(cmdline)) {
        eval(cmdline, STDIN_FILENO, out_fd);
        return;
    }

    // pipeline exists
    cmd = get_last_command(cmdline);
    
    pipe(pipefd);
    eval_pipeline(cmdline, pipefd[1]);
    eval(cmd, pipefd[0], out_fd);

    close(pipefd[0]);
    close(pipefd[1]);
}

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline, int in_fd, int out_fd) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 
    if (argv[0] == NULL)  
	    return;   /* Ignore empty lines */


    if (builtin_command(argv)) return;

    pid = Fork();
    
    //child
    if (pid == 0) {
        if (in_fd != STDIN_FILENO)
            dup2(in_fd, STDIN_FILENO);
        if (out_fd != STDOUT_FILENO) {
            dup2(out_fd, STDOUT_FILENO);
            dup2(out_fd, STDERR_FILENO);
        }
        if (execvp(argv[0], argv) < 0) {
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
        }
        return;
    }
    
    // parent
    if (!bg) { // foreground task
        int status;
        Waitpid(pid, &status, 0);
        if (out_fd != STDOUT_FILENO) close(out_fd); // **IMPORTANT** send EOF to the pipe
    }
    else { // background task
        printf("%d %s", pid, cmdline);
        // TODO: add pid to task list
    }
    
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "quit") || !strcmp(argv[0], "exit")) { /* quit command */
	    exit(0);  
    }
    if (!strcmp(argv[0], "&")) { /* Ignore singleton & */
	    return 1;
    }
    if (!strcmp(argv[0], "cd")) {
        if (chdir(argv[1])) {
            printf("invalid directory: %s\n", argv[1]);
        }
        return 1;
    }
    return 0; /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end parseline */


