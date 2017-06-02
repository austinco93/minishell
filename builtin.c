/* CS 352 -- builtin.c 
 *
 *   January 17, 2017, Austin Corotan
 *   Modified January 29, 2017
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "proto.h"
#include "global.h"

typedef int (*funcptr) (char**, int*, int, int ,int); /* function pointer type */

/* struct used to store builtin function names and their pointers */
struct func_array {
	char *name;
	funcptr fptr;
};

/* built-in implementation of exit, will exit with given value */
static int bi_exit(char **argv, int *argcp, int infd, int outfd, int errfd){
	if(*argcp < 2){
		exit(0);
	} else if (*argcp > 2){
		exit(127);
	} else {
		exit(atoi(argv[1]));
	}
}

/* built-in implementation of echo */
static int bi_aecho(char **argv, int *argcp, int infd, int outfd, int errfd){
 	int i;
 	if(*argcp > 2) {
 		if(strcmp(argv[1], "-n") == 0){
 			for (i = 2; i < *argcp - 1; i++){
 			dprintf(outfd, "%s ", argv[i]);
 			}
 			dprintf(outfd, "%s", argv[*argcp - 1]);
 		} else {
 			for (i = 1; i < *argcp - 1; i++){
 				dprintf(outfd, "%s ", argv[i]);
 			}
 			dprintf(outfd, "%s\n", argv[*argcp - 1]);
 		}
 	}else if(*argcp == 2){
 		if(strcmp(argv[1], "-n") == 0){
 			dprintf(outfd, '\0');
 		} else {
 			dprintf(outfd, "%s\n", argv[*argcp - 1]);
 		}
 	}
 	else{
 		dprintf(outfd, "\n");
 	}
 	return 0;
}

/* built-in implementation of envset */
static int bi_envset(char **argv, int *argcp, int infd, int outfd, int errfd){
 	if(*argcp == 3){
 		setenv(argv[1], argv[2], 1);
 		return 0;
 	} else {
 		dprintf(errfd, "Usage: envset name value\n");
 		return 1;
 	}
}

/* built-in implementation of envunset */
static int bi_envunset(char **argv, int *argcp, int infd, int outfd, int errfd){
 	if(*argcp == 2){
 		unsetenv(argv[1]);
 		return 0;
 	} else {
 		dprintf(errfd, "Usage: envunset name\n");
 		return 1;
 	}
}

/* built-in implementation of cd [dir] */
static int bi_cd(char **argv, int *argcp, int infd, int outfd, int errfd){
 	int initcheck; /* check to see chdir worked assuming 2 args */
	int init2check;/* check to see chdir worked using HOME */
	char *home; /* getenv return location */
	if(*argcp == 2){
 		initcheck = chdir(argv[1]);
 		if(initcheck == -1){
 			dprintf(errfd, "No such file or directory\n");
 			return 1;
 		}
 	} else if (*argcp == 1){
 		home = getenv("HOME");
 		if(home != NULL){
 			init2check = chdir(home);
 			if(init2check == -1){
 				dprintf(errfd, "No such file or directory\n");
 				return 1;
 			}
 		} else {
 			dprintf(errfd, "Error: HOME unset\n");
 			return 1;
 		}
 	} else{
 		dprintf(errfd, "No such file or directory\n");
 		return 1;
 	}
 	return 0;
}

/* built-in implementation of shift */
static int bi_shift(char **argv, int *argcp, int infd, int outfd, int errfd){
 	if(*argcp > 1){
 		if((atoi(argv[1]) < mainargc - shift) && (atoi(argv[1]) >= 1)){
 			shift = shift + atoi(argv[1]);
 			return 0;
 		} else {
 			dprintf(errfd, "Error: cannot shift\n");
 			return 1;
 		}
 	} else {
 		if((1 < mainargc - shift)){
 			shift = shift + 1;
 			return 0;
 		} else {
 			dprintf(errfd, "Error: cannot shift\n");
 			return 1;
 		}
 	}
}

/* built-in implementation of unshift */
static int bi_unshift(char **argv, int *argcp, int infd, int outfd, int errfd){
 	if(*argcp > 1){
 		if(atoi(argv[1]) >= shift){
 			dprintf(errfd, "Error: cannot unshift\n");
 			return 1;
 		} else {
 			shift = shift - atoi(argv[1]);
 			return 0;
 		}
 	} else if(*argcp == 1){
 		shift = 1;
 		return 0;
 	} else {
 		return 1;
 	}
}

/* built-in implementation of sstat */
static int bi_sstat(char **argv, int *argcp, int infd, int outfd, int errfd){
 	struct stat finfo; /* file info */
 	struct passwd *access; /* access info */
 	struct group *ginfo; /* group info */
 	struct tm *time; /* time info */
 	int filenum = 1;
 	int statcheck = -1; 
 	char *fname, uname[80], gname[80], *ret_time;
 	char perm_list[12]; 
 	int numlinks, fsize;

 	if(*argcp < 2){
 		dprintf(errfd, "incorrect number of arguments\n");
 		return 1;
 	}

 	while(filenum < *argcp){
 		fname = argv[filenum];
 		statcheck = stat(fname, &finfo);
 		if(statcheck < 0){
 			dprintf(errfd, "%s: stat error\n", strerror(errno));
 			return 1;
 		}
 		numlinks = (int)finfo.st_nlink;
 		fsize = (int)finfo.st_size;

 		access = getpwuid(finfo.st_uid);
 		if(access == NULL){
 			snprintf(uname,80,"%d",(int)finfo.st_uid);
 		}else{
 			snprintf(uname,80,"%s",access->pw_name);
 		}	

 		ginfo = getgrgid(finfo.st_gid);
 		if(ginfo == NULL){
 			snprintf(gname,80,"%d",(int)finfo.st_gid);
 		}else{
 			snprintf(gname,80,"%s",ginfo->gr_name);
 		}

 		time = localtime(&finfo.st_mtime);
 		ret_time = asctime(time);

 		strmode1(finfo.st_mode, perm_list);
 		perm_list[10] = '\0';
 		dprintf(outfd, "%s %s %s %s %d %d %s", fname, uname, gname, perm_list, numlinks, fsize, ret_time);
 		filenum++;
 	}
 	return 0;
}

/* built-in implementation of read */
static int bi_read(char **argv, int *argcp, int infd, int outfd, int errfd){
 	char buffer [1024];
    int len;
 	if(*argcp != 2){
 		dprintf(outfd,"Usage: read variable_name\n");
 		return 1;
 	}else{
 		FILE* s = fdopen(dup(infd),"w+");
 		fgets (buffer, 1024, s);
	     
        /* Get rid of \n at end of buffer. */
	    len = strlen(buffer);
	    if (buffer[len-1] == '\n'){
	        buffer[len-1] = 0;
	    }

	    setenv(argv[1], buffer, 1);
	    fclose(s);
 	}
 	return 0;
}
 
/* checks built-in functions for shell, returns -1 of no builtin found, 0 if builtin success, 1 if builtin fail */
int builtin_check(char **argv, int *argcp, int infd, int outfd, int errfd){
 	struct func_array flist[] = {{"exit", bi_exit}, {"aecho", bi_aecho}, {"envset", bi_envset}, {"envunset", bi_envunset}, 
 								{"cd", bi_cd}, {"shift", bi_shift}, {"unshift", bi_unshift}, {"sstat", bi_sstat},{"read", bi_read}, {NULL, NULL}};
 	int check = -1;
 	int i;
 	for(i = 0; i < 9; i++){
 		if(strcmp(argv[0], flist[i].name) == 0){
 			check = flist[i].fptr(argv, argcp, infd, outfd, errfd);
 			return check;
 		}
 	}
 	return check;
 }

