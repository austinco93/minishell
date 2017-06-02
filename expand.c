/* CS 352 -- expand.c 
 *
 *   January 27, 2017, Austin Corotan
 *	 Modified January 29, 2017
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <dirent.h>
#include "proto.h"
#include "global.h"

static int pid_expand (char **orig, char **new, int sizeCheck){
	char* new_temp = *new;
	char* orig_temp = *orig;
	int id = getpid();
	int size = 0;
	char* ids = (char *)malloc(sizeof(char)*10);
	if (ids == NULL) {
        perror("malloc failed");
        return -1;
  	}
  	orig_temp++;
	size = snprintf(ids, 10, "%d", id);
	if(size > sizeCheck){
		dprintf(2,"expansion too long\n");
		return -1;
	}
	while(*ids != '\0'){
		*new_temp = *ids;
		new_temp++;
		ids++;
	}
	*orig = orig_temp;
	*new = new_temp;
	return size;
}

static int env_expand (char **orig, char **new, int sizeCheck){
	char* new_temp = *new;
	char* orig_temp = *orig;
	char *env_var; /* pointer to environment variable to be passed to getenv() */
	char *env_name; /* pointer to result of getenv() */
	int env_len = 0;
	orig_temp = orig_temp+2;
	env_var = orig_temp;
	while(*orig_temp != '}' && *orig_temp != '\0'){
		orig_temp++;
	}
	if(*orig_temp == '}'){
		*orig_temp = '\0';
		env_name = getenv(env_var);
		*orig_temp = '}';
		if(env_name != NULL){
			env_len = strlen(env_name);
			if(env_len <= sizeCheck){
				while(*env_name != '\0'){
					*new_temp = *env_name;
					new_temp++;
					env_name++;
					sizeCheck--;
				}
			} else {
				dprintf(2,"expansion too long\n");
				return -1;
			}
		}
	} else {
		dprintf(2,"no matching }\n");
		return -1;
	}
	*orig = orig_temp;
	*new = new_temp;
	return env_len;
}

static int argnum_expand (char **orig, char **new, int sizeCheck){
	char* new_temp = *new;
	char* orig_temp = *orig;
	int size = 0;
	int currentnum = mainargc - shift;
	char* argnum = (char *)malloc(sizeof(char)*10);
	if (argnum == NULL) {
        perror("malloc failed");
        return -1;
  	}
	size = snprintf(argnum, 10, "%d", currentnum);
	if(size > sizeCheck){
		dprintf(2,"expansion too long\n");
		return -1;
	}
	orig_temp++;
	while(*argnum != '\0'){
		*new_temp = *argnum;
		argnum++;
		new_temp++;
	}
	*orig = orig_temp;
	*new = new_temp;
	return size;
}

static int argid_expand (char **orig, char **new, int sizeCheck){
	char* new_temp = *new;
	char* orig_temp = *orig;
	int argid,size = 0;
    char argnum[10];
	orig_temp++;
	int i = 0;
	while(isdigit(*orig_temp)){
		argnum[i] = *orig_temp;
		orig_temp++;
		i++;
	}
	argnum[i] = '\0';
	orig_temp--;
	argid = atoi(argnum);
	if(argid+shift >= mainargc){
		*orig = orig_temp;
		*new = new_temp;
		return size;
	}
	char* argname = mainargv[argid+shift];
	size = strlen(argname);
	if(size > sizeCheck){
		dprintf(2,"expansion too long\n");
		return -1;
	}
	while(*argname != '\0'){
		*new_temp = *argname;
		argname++;
		new_temp++;
	}
	*orig = orig_temp;
	*new = new_temp;
	return size;
}

static int status_expand (char **orig, char **new, int sizeCheck){
	char* new_temp = *new;
	char* orig_temp = *orig;
	int size = 0;
	char* status = (char *)malloc(sizeof(char)*10);
	if (status == NULL) {
        perror("malloc failed");
        return -1;
  	}
  	orig_temp++;
	size = snprintf(status, 10, "%d", statuscheck);
	if(size > sizeCheck){
		dprintf(2,"expansion too long\n");
		return -1;
	}
	while(*status != '\0'){
		*new_temp = *status;
		new_temp++;
		status++;
	}

	*orig = orig_temp;
	*new = new_temp;
	return size;
}

static int wildcard (char **orig, char **new, int sizeCheck){
	char* new_temp = *new;
	char* orig_temp = *orig;
	DIR *dirp;
	struct dirent *dp;
	char compare[1024];
	int size = 0;
	int j = 0;
	int first_find = 0;
	int ncount = 0;

	*new_temp = *orig_temp;
	orig_temp++;
	new_temp++;
	ncount++;

    while(*orig_temp != ' ' && *orig_temp != '\0' && *orig_temp != '\"'){
        compare[j] = *orig_temp;
        *new_temp = *orig_temp;
		orig_temp++;
		new_temp++;
		ncount++;
        j++;
    }

    compare[j]= '\0';

	dirp = opendir(".");
    if (dirp == NULL){
    	dprintf(2,"opendir failed\n");
    }    
    int len = strlen(compare);
    while ((dp = readdir(dirp)) != NULL) {
    	if(dp->d_name[0] == '.'){
    		continue;
    	}
    	int namelen = strlen(dp->d_name);
    	char* comparestr = ((dp->d_name)+(namelen - len));
        if ((namelen >= len) && (strcmp(comparestr, compare) == 0)) {
        	if(namelen > sizeCheck){
        		dprintf(2,"expansion too long\n");
				return -1;
        	}
        	if(first_find == 0){
        		first_find = 1;
        		new_temp = new_temp - ncount;
        	}
        	int i = 0;
        	while(dp->d_name[i] != '\0'){
				*new_temp = dp->d_name[i];
				new_temp++;
				i++;
				size++;
			}
			*new_temp = ' ';
			new_temp++;
			size++;
        }
   	}
   	if(first_find == 1){
   		new_temp--;
   		*new_temp = '\0';
   		size--;
   	}
   	orig_temp--;
   	*orig = orig_temp;
	*new = new_temp;
    (void)closedir(dirp);        
	return size;
}

static int pipe_expand (char **subcmd, char **new, int sizeCheck, int infd, int flags){
	char *new_temp = *new;
	char *sub_temp = *subcmd;
	int size = 0;
	int pfd[2];
	int pipecheck = -1;
	int cpid;
	int rv;
	int bufsize;
	if(flags & WAIT){
		bufsize = 200000;
	}else{
		bufsize = 1024;
	}
	char new_buf[bufsize];
	int newloc = 0;

	pipecheck = pipe(pfd);
	if (pipecheck < 0) {
            perror ("pipe");
      return -1;
    }
    cpid = processline(sub_temp, infd, pfd[1], EXPAND);
    if(cpid < 0){
    	dprintf(2,"processline\n");
    	close(pfd[0]);
    	close(pfd[1]);
    	return -1;
    } else {
    	close(pfd[1]);
    	while((rv = read(pfd[0], &new_buf[newloc], bufsize - newloc)) != 0){
    		newloc = newloc + rv;
    		if(bufsize - newloc <= 0){
    			dprintf(2,"expansion too long\n");
    			return -1;
    		}
    	}	
    	close(pfd[0]);
    	if(newloc > sizeCheck){
    		dprintf(2,"expansion too long\n");
			return -1;
    	} else {
    		size = newloc;
    		int i = 0;
        if(new_buf[newloc-1] == '\n'){      
          new_buf[newloc-1]='\0';
        }
        
    		while(new_buf[i] != '\0'){
    			if(new_buf[i] == '\n'){
    				new_buf[i] = ' ';
    			}
    			*new_temp = new_buf[i];
    			new_temp++;
    			i++;
    		}
    		new_temp--;
            if(*new_temp != ' '){
                new_temp++;
            }
    	}
    	
    	if(cpid != 0){
    		if (waitpid (cpid,&statuscheck,0) < 0){
        		perror ("wait");
			}

			if(WIFEXITED(statuscheck)){
          	statuscheck = WEXITSTATUS(statuscheck);
      		}else {
          	statuscheck = 127;
      		}
      	}
    if(flags & WAIT){
      memset(new_buf, 0, 200000);
    }else{
      memset(new_buf, 0, 1024);
    }
    killzombies();
    *subcmd = sub_temp;
	*new = new_temp;
	return size;
	}
}

/* checks for expandable variables and stores variable values in a new array */
int expand (char *orig, char *new, int newsize, int infd, int flags){
	int sizeCheck = newsize - 1; 
	int returnSize = 0;
    typedef enum { false, true } bool;
    bool beginline = true;
    int paren_count = 0;
    int subsize = 0;
    char* subcommand = (char *)malloc(sizeof(char)*1024);
    if (subcommand == NULL) {
        perror("malloc failed");
        return -1;
  	}

    while(*orig != '\0'){
        signal(SIGINT, got_sig);
        if(sig_signaled == 1){
            sig_signaled = 0;
            return 1;
        }
        if((*orig == '\\') && (*(orig+1)=='*') && paren_count == 0){
            sizeCheck--;
            if(sizeCheck < 0){
                dprintf(2,"expansion too long\n");
                return 1;
            }
            orig++;
            *new = *orig;
            new++; 
        }else if((*orig == '*') && (beginline == true && paren_count == 0)){ 
            returnSize = wildcard(&orig, &new, sizeCheck);
            if(returnSize == -1){
				return 1;
			} else {
				sizeCheck = sizeCheck - returnSize;
			}
        } else if((*orig == ' ') && (*(orig+1)=='*') && paren_count == 0){
            sizeCheck--;
            if(sizeCheck < 0){
                dprintf(2,"expansion too long\n");
                return 1;
            }
            *new = *orig;
            new++;
            orig++;
            returnSize = wildcard(&orig, &new, sizeCheck);
            if(returnSize == -1){
				return 1;
			} else {
				sizeCheck = sizeCheck - returnSize;
			}
        } else if(*orig == '$' && *(orig+1) == '$' && paren_count == 0){ 
            returnSize = pid_expand(&orig,&new,sizeCheck);
            if(returnSize == -1){
                return 1;
            } else {
                sizeCheck = sizeCheck - returnSize;
            }
        } else if(*orig == '$' && *(orig+1) == '{' && paren_count == 0){ 
            returnSize = env_expand(&orig,&new,sizeCheck);
            if(returnSize == -1){
                return 1;
            } else {
                sizeCheck = sizeCheck - returnSize;
            }
        } else if(*orig == '$' && *(orig+1) == '#' && paren_count == 0){ 
            returnSize = argnum_expand(&orig,&new,sizeCheck);
            if(returnSize == -1){
                    return 1;
            } else {
                    sizeCheck = sizeCheck - returnSize;
            }
        } else if(*orig == '$' && isdigit(*(orig+1)) && paren_count == 0){ 
            returnSize = argid_expand(&orig,&new,sizeCheck);
            if(returnSize == -1){
                    return 1;
            } else {
                    sizeCheck = sizeCheck - returnSize;
            }
        } else if(*orig == '$' && *(orig+1) == '?' && paren_count == 0){ 
            returnSize = status_expand(&orig,&new,sizeCheck);
            if(returnSize == -1){
                    return 1;
            } else {
                    sizeCheck = sizeCheck - returnSize;
                }
        } else if(*orig == '$' && *(orig+1) == '(' && paren_count == 0){ 
    	      paren_count++;
        		orig++;
       	} else if(*orig == '(' && paren_count > 0){ 
        	paren_count++;
        	*subcommand = *orig;
        	subcommand++;
       	  	subsize++;
       	} else if(*orig == ')' && paren_count == 1){
       		  paren_count--;
       		  *subcommand = '\0';
       		  subcommand = subcommand - subsize;
              subsize = 0;
       		  returnSize = pipe_expand(&subcommand,&new,sizeCheck, infd, flags);
       		  if(returnSize == -1){
              return 1;
            } else {
              sizeCheck = sizeCheck - returnSize;
            }
       	} else if(*orig == ')' && paren_count > 1){ 
       		  paren_count--;
       		  *subcommand = *orig;
       		  subcommand++;
      		  subsize++;
       	} else if(paren_count > 0){ 
       		  *subcommand = *orig;
       		  subcommand++;
       		  subsize++;
       	} else { 
            sizeCheck--;
            if(sizeCheck < 0){
                    dprintf(2,"expansion too long\n");
                    return 1;
            }
            *new = *orig;
            new++;
        }
        orig++;
        beginline = false;
    }
    if(paren_count != 0){
        dprintf(2,"missing )\n");
        return 1;
    }
	*new = '\0';
	return 0;
}
