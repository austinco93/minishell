#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "proto.h"
#include "global.h"

void fdcheck(int *cinfd, int *coutfd, int *cerrfd, int *infd, int *outfd){
	  if(*cinfd != *infd){
        close(*cinfd);
        *cinfd = 0;
      }
      if(*coutfd != *outfd){
        close(*coutfd);
        *coutfd = 1;
      }
      if(*cerrfd != 2){
        close(*cerrfd);
        *cerrfd = 2;
      }
}

void killzombies(){
	int zombiestatus = 0;
	while(waitpid(-1, &zombiestatus, WNOHANG) > 0);
}

void remove_comments(char **line){
    char *temp_line = *line;
    char *temp_cline = *line;
    int i = 0;
    typedef enum { false, true } bool;
    bool prevdollar = false;
    bool prevquote = false;
    while(*temp_line != '\0'){
      if(*temp_line == '#' && prevdollar == false && prevquote == false){
        break;
      }
      if(*temp_line == '$'){
        prevdollar = true;
      } else {
        prevdollar = false;
      }

      if(*temp_line == '\"'){
        prevquote = true;
      }
      temp_cline[i] = *temp_line;
      i++;
      temp_line++;
    }
    temp_cline[i] = '\0';
    *line = temp_cline;
}

static int findunquoted(char **line){
	typedef enum { false, true } bool;
	bool isquotes = false;
	char *line_temp = *line;
	int index = -1;
	int i = 0;
	while(line_temp[i] != '\0'){
		if(line_temp[i] == '\"' && isquotes == false){
			isquotes = true;
			i++;
		} else if(line_temp[i] == '\"' && isquotes == true){
			isquotes = false;
			i++;
		} else if((line_temp[i] == '<' || line_temp[i] == '>') && isquotes == false){
			index = i;
			return index;
		} else {
			i++;
		}
	}
	return index;
}

static void removeQuotes(char *line) {
    char *src, *dst;
    src = dst = line;
    while(*src != '\0'){
      *dst = *src;
      if(*dst != '\"') {
        dst++;
      }
      src++;
    }
    *dst = '\0';
}

static int redir_helper(char **line, int *realindex, int spaceCount, int **realfd, int fd_orig, int flags){
	char filename[1024];
	int i = 0;
	int index = *realindex;
	int *fd = *realfd;
	char *line_temp = *line;

	while(i != spaceCount){
		line_temp[index] = ' ';
		index++;
		i++;
	}
	while(line_temp[index] == ' '){
		index++;
	}
	i = 0;
	while(line_temp[index] != ' ' && line_temp[index] != '\0'){ 
		filename[i] = line_temp[index];
		line_temp[index] = ' ';
		index++;
		i++;
	}
	filename[i] = '\0';
	removeQuotes(filename);
	if(*fd != fd_orig){
		close(*fd);
	}
	if(flags == 0){
		if((*fd = open(filename, O_RDONLY|O_CREAT, 0707)) < 0){
			perror("open");
			return -1;
		}
	} else if(flags == 1){
		if((*fd = open(filename, O_RDWR|O_CREAT|O_APPEND, 0707)) < 0){
			perror("open");
			return -1;
		}
	} else {
		if((*fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0707)) < 0){
			perror("open");
			return -1;
		}
	}
	*realindex = index;
	*realfd = fd;
	*line = line_temp;
	return 0;
}

int redirection(char **line, int *cinfd, int *coutfd, int *cerrfd){ 
	char *line_temp = *line;
	int index;
	int check = 0;
	while((index = findunquoted(&line_temp)) >= 0){ 
		if(line_temp[index] == '<'){ /* redirect stdin */ 
			check = redir_helper(&line_temp, &index, 1, &cinfd, 0, 0);
		}else if(line_temp[index] == '>' && line_temp[index+1] == '>' && line_temp[index-1] == '2' && line_temp[index-2] == ' '){ /* stderr and append */
			index--; 
			check = redir_helper(&line_temp, &index, 4, &cerrfd, 2, 1);
		}else if(line_temp[index] == '>' && line_temp[index-1] == '2' && line_temp[index-2] == ' '){ /* stderr and trunc */
			index--; 
			check = redir_helper(&line_temp, &index, 3, &cerrfd, 2, 2);
		}else if(line_temp[index] == '>' && line_temp[index+1] == '>'){ /* stdout and append */ 
			check = redir_helper(&line_temp, &index, 2, &coutfd, 1, 1);
		}else{/* stdout and trunc */
			check =redir_helper(&line_temp, &index, 1, &coutfd, 1, 2);
		}
		if(check == -1){
			return -1;
		}
	}
	*line = line_temp;
	return 0;
}

static int loadpipecmds(char *line, char **pcommands){
	typedef enum { false, true } bool;
	int count = 0;
	bool is_pipe = false;
  	bool is_quote = false;

 	while(*line != '\0'){
    	if(*line == '\"'){
      		if(is_quote == false){
       			is_quote = true;
       			if(is_pipe == false){
          			pcommands[count] = line;
          			count++;
       			}
      		} else {
         		is_quote = false;
      		}
      		is_pipe = true;
      		line++;
    	}else if(*line == '|'){
      		if(is_quote == false && is_pipe == true){
        		*line = '\0';
        		is_pipe = false;
      		} 
      		line++;
   		} else {
     		if(is_pipe == false && is_quote == false){
       			pcommands[count] = line;
       			count++;
     		}
      		is_pipe = true;
      		line++;
    	}
  	}
  	pcommands[count] = '\0';
  	return count;
}

int piping(char **line, int *infd, int *outfd){
	typedef enum { false, true } bool;
	char *line_temp = *line;
	int pfd[2];
	int pfd2[2];
	int pipenum;
	char **pcommands = (char **)malloc(1024*sizeof(char *));
    if (pcommands == NULL) {
        perror("malloc failed");
        return -1;
    }

	pipenum = loadpipecmds(line_temp, pcommands);
	if(pipenum == 1){
		return 0;
	}

	int i = 0;
	bool pfdclose = true;
	while(i < pipenum){
		if(i == 0){
			pipe(pfd);
			processline(pcommands[i], *infd, pfd[1], 0);
			close(pfd[1]);
			pfdclose = false;
		} else if(i == pipenum - 1 && pfdclose == false){
			processline(pcommands[i], pfd[0], *outfd, WAIT);
			close(pfd[0]);
		} else if(i == pipenum - 1 && pfdclose == true){
			processline(pcommands[i], pfd2[0], *outfd, WAIT);
			close(pfd2[0]);
		} else if(pfdclose == false){
			pipe(pfd2);
			processline(pcommands[i], pfd[0], pfd2[1], 0);
			close(pfd[0]);
			close(pfd2[1]);
			pfdclose = true;
		} else if(pfdclose == true){
			pipe(pfd);
			processline(pcommands[i], pfd2[0], pfd[1], 0);
			close(pfd2[0]);
			close(pfd[1]);
			pfdclose = false;
		}
		i++;
	}
	memset(line_temp, 0, 200000);
	*line = line_temp;
	return pipenum;
}





