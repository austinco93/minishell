/* CS 352 -- Mini Shell!  
 *
 *   Sept 21, 2000,  Phil Nelson
 *   Modified April 8, 2001 
 *   Modified January 6, 2003
 *
 *   January 27, 2017, Austin Corotan
 *   CSCI 352, Assignment 6
 *   Modified March 9, 2017
 *
 */
#define MAIN
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "proto.h"
#include "global.h"

/* Signal Handler */
void got_sig(int sig){
    sig_signaled = 1;
}

/* Shell main */
int
main (int argc, char** argv)
{
    char   buffer [LINELEN];
    int    len;

    mainargv = argv;
    mainargc = argc;
    shift = 1;
    statuscheck = 0;
    sig_signaled = 0;

    if(mainargc > 1){
      fstream = fopen(mainargv[1], "r");
      if(fstream == NULL){
        perror ("open");
        exit(127);
      }  
    }else{
      fstream = stdin;
      shift = 0;
    }

    while (1) {

        /* prompt and get line */
        if(fstream == stdin){
            fprintf (stderr, "%% ");
        }
        if (fgets (buffer, LINELEN, fstream) != buffer)
	        break;
        
        signal(SIGINT, got_sig);
        if(sig_signaled == 1){
            sig_signaled = 0;
        }
            /* Get rid of \n at end of buffer. */
            len = strlen(buffer);
            if (buffer[len-1] == '\n')
                buffer[len-1] = 0;

            /* Run it ... */
            //memset(buffer, 0, LINELEN);
            processline (buffer, 0, 1, WAIT|EXPAND|STMTS);
    }

    if (!feof(fstream))
        perror ("read");
    return 0;		/* Also known as exit (0); */
}

int processline (char *line, int infd, int outfd, int flags)
{
    pid_t  cpid;
    int cinfd = infd;
    int coutfd = outfd;
    int cerrfd = 2;

    /* account for comment within line */
    remove_comments(&line);
    if(flags & STMTS){
      int statementcheck = statements(line, cinfd, coutfd, cerrfd);
      if(statementcheck < 0){
        return -1;
      }
    }
    char *newline = malloc(LINELEN*sizeof(char)); /* array for storing expanded variable line */
      if (newline == NULL) {
        perror("malloc failed");
        return -1;
    } 
    int newsize = LINELEN; /* size of new line array */
    if(flags & EXPAND){
      int expandCheck = expand(line, newline, newsize, infd, flags); /* checks for expandable variables */
      statuscheck = expandCheck;
      /* if expandCheck fails, stop processing current line */
      if(expandCheck != 0){
        return -1;
      }
      fdcheck(&cinfd, &coutfd, &cerrfd, &infd, &outfd);
      int pipecheck = piping(&newline, &cinfd, &coutfd);
      if(pipecheck < 0){
        return -1;
      }
      } else {
        newline = line;
      }

    int redirCheck = redirection(&newline, &cinfd, &coutfd, &cerrfd);
    if(redirCheck != 0){
      return -1;
    }

    int argcp = 0; /* argument counter, includes user command (arg0) */
    char ** argvp = arg_parse(newline, &argcp); /* argument array parameter to be used with exec */

    /* if arg_parse finds no arguments, ignore the line */
    if(argcp == 0){
      return 0;
    }

    /* check for builtin commands */
    int check = builtin_check(argvp, &argcp, cinfd, coutfd, cerrfd);
    if(check >= 0){
      statuscheck = check;
      free(argvp);
      fdcheck(&cinfd, &coutfd, &cerrfd, &infd, &outfd);
      return 0;
    }
    /* Start a new process to do the job. */
    cpid = fork();
    if (cpid < 0) {
      perror ("fork");
      free(argvp);
      return -1;
    }
    
    /* Check for who we are! */
    if (cpid == 0) {
      /* We are the child! */
      dup2(cinfd,  0);
      dup2(coutfd, 1);
      dup2(cerrfd, 2);
      execvp(*argvp, argvp);
      perror ("exec");
      fclose(fstream);
      exit(127);
    }
    
    if(flags & WAIT){
      /* Have the parent wait for child to complete */
      if (waitpid (cpid,&statuscheck,0) < 0){
        perror ("wait");
        free(argvp);
        return -1;
      }

      if(WIFEXITED(statuscheck)){
          statuscheck = WEXITSTATUS(statuscheck);
      }else {
          statuscheck = 127;
      }
      free(argvp);
      fdcheck(&cinfd, &coutfd, &cerrfd, &infd, &outfd);
      killzombies();
      return 0;
    }
    free(argvp);
    fdcheck(&cinfd, &coutfd, &cerrfd, &infd, &outfd);
    killzombies();
    return cpid;
}

