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
#include "statement.h"

void init_stmt(stmt_list *stmt){
	stmt->nstmts = 0;
  	stmt->size = 80;
  	stmt->stmts = (line *)malloc(sizeof(line)*stmt->size);
}

void init_line(line *line){
	line->kind = INIT;
	line->elsend = -1;
	line->cmd = NULL;
}

void set_line(line *line, kind_t kind, char *cmd){
	line->kind = kind;
	line->cmd = strdup(cmd);
}

void add_line(stmt_list *stmt, line line){
	if (stmt->nstmts >= stmt->size) {
		double_capacity(stmt);
	}
	stmt->stmts[stmt->nstmts] = line;
	stmt->nstmts++;
}

void double_capacity(stmt_list *stmt){
    stmt->size *= 2;
    stmt->stmts = (line *)realloc(stmt->stmts, sizeof(line)*stmt->size);
}

line get_line(stmt_list *stmt, int index){
	return stmt->stmts[index];
}
int num_lines(stmt_list *stmt){
	return stmt->nstmts;
}
void set_elsend(stmt_list *stmt, int lineindex, int elsendindex){
	stmt->stmts[lineindex].elsend = elsendindex;
}

kind_t first_word(char *line){
	char check[80];
	int i = 0;
	while(*line == ' '){ 
		line++;
	}
	if(*line != '\"'){
		while(*line != ' ' && *line != '\0' && *line != '\n'){
			check[i] = *line;
			i++;
			line++;
		}
		check[i] = '\0';
	}
	if(strcmp(check,"if") == 0){
		return W_IF;
	} else if(strcmp(check,"else") == 0){
		return W_ELSE;
	} else if(strcmp(check,"end") == 0){
		return W_END;
	} else if(strcmp(check,"while") == 0){
		return W_WHILE;
	} else {
		return W_OTHER;
	}
}

int statements(char *commandline, int infd, int outfd, int errfd){
	kind_t kind;
	stmt_list stmt;
	if((kind = first_word(commandline)) != W_OTHER){
		init_stmt(&stmt);
		read_stmt(commandline, kind, &stmt, infd, outfd, errfd);
		run_stmt(0, &stmt);
		free_stmt(&stmt);
		memset(commandline, 0, 200000);
		return 1;
	}
	return 0;
}

int run_stmt(int startloc, stmt_list *stmt){
	line thisline = stmt->stmts[startloc];
	processline (thisline.cmd, 0, 1, WAIT|EXPAND);
	int cmdeval = statuscheck;
	int curloc = startloc + 1;
	if(cmdeval != 0){
		if(thisline.kind == W_IF && stmt->stmts[stmt->stmts[startloc].elsend].kind == W_ELSE){
			curloc = stmt->stmts[startloc].elsend + 1;
		} else {
			return stmt->stmts[startloc].elsend + 1;
		}
	}
	int size = stmt->size;
	while(curloc < size){
		line currline = stmt->stmts[curloc];
		kind_t kindtest = stmt->stmts[curloc].kind;
		switch(kindtest){
			case INIT: break;
			case W_IF: curloc = run_stmt(curloc, stmt); break;
			case W_WHILE: curloc = run_stmt(curloc, stmt); break;
			case W_ELSE: return stmt->stmts[curloc].elsend + 1; break;
			case W_END:
				if(thisline.kind == W_IF){
					return curloc + 1;
				} else {
					return curloc = run_stmt(startloc, stmt);
				}
				break;
			case W_OTHER: 
				processline(currline.cmd, 0, 1, WAIT|EXPAND);
				curloc = curloc + 1;
				break;
		}
	}
	return 0; 
}

void remove_first_word(char **cmd){
	char *temp_cmd = *cmd;
	while (*temp_cmd == ' ' && *temp_cmd != '\0'){
		*temp_cmd = *(temp_cmd + 1);
		temp_cmd++;
	}
	while(*temp_cmd != ' ' && *temp_cmd != '\0'){
		*temp_cmd = *(temp_cmd + 1);
		temp_cmd++;
	}
	*cmd = temp_cmd;
}

void print_prompt(int errfd){
	if(getenv("P2") == 0){
		dprintf (errfd, "> ");
	} else {
		dprintf (errfd, "%s", getenv("P2"));
	}
}

int read_stmt(char *cmd, kind_t kind, stmt_list *stmt, int infd, int outfd, int errfd){
	char newline[1024];
	int index = num_lines(stmt);
	line firstline;
	init_line(&firstline);
	remove_first_word(&cmd);
	set_line(&firstline, kind, cmd);
	add_line(stmt, firstline);
	int num_elses;
	if(kind == W_IF){
		num_elses = 1;
	} else {
		num_elses = 0;
	}
	print_prompt(errfd);
	if (fgets (newline, 1024, fstream) != newline){
		return -1;
	}
	int i = 0;
	while(newline[i] != '\0'){
		if(newline[i] == '\n'){
			newline[i] = '\0';
		}
		i++;
	}
	kind_t newkind = first_word(newline);
	line nextline;
	while(newkind != W_END){
	switch(newkind){
		case INIT: break;
		case W_IF: 
			read_stmt (newline, newkind, stmt, infd, outfd, errfd); break;
		case W_WHILE: 
			read_stmt (newline, newkind, stmt, infd, outfd, errfd); break;
		case W_ELSE: 
			if(num_elses > 0){
				init_line(&nextline);
				set_elsend(stmt, index, num_lines(stmt));
				index = num_lines(stmt);
				set_line(&nextline, W_ELSE, newline);
				add_line(stmt, nextline);
				num_elses--;
			} else {
				dprintf(errfd, "too many elses\n");
				return -1;
			}
			break;
		case W_OTHER: 
			init_line(&nextline);
			set_line(&nextline, W_OTHER, newline);
			add_line(stmt, nextline);
			break;
		case W_END: break;
	}
	print_prompt(errfd);
	if (fgets (newline, 1024, fstream) != newline){
		return -1;
	}
	i = 0;
	while(newline[i] != '\0'){
		if(newline[i] == '\n'){
			newline[i] = '\0';
		}
		i++;
	}
	newkind = first_word(newline);
	}
	set_elsend(stmt, index, num_lines(stmt));
	init_line(&nextline);
	set_line(&nextline, W_END, newline);
	add_line(stmt, nextline);
	return 0;
}

void free_stmt(stmt_list *stmt){
	free(stmt->stmts);
}