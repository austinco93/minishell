/* CS 352 -- proto.h 
 *
 *   January 17, 2017, Austin Corotan
 *	 Modified January 29, 2017
 */

char ** arg_parse (char *line, int *argcp);

int builtin_check(char **argv, int *argcp, int infd, int outfd, int errfd);

int expand (char *orig, char *new, int newsize, int infd, int flags);

void strmode1(mode_t mode, char *p);

int processline (char *line, int infd, int outfd, int flags);

void remove_comments(char **line);

int redirection(char **line, int *cinfd, int *coutfd, int *cerrfd);

void fdcheck(int *cinfd, int *coutfd, int *cerrfd, int *infd, int *outfd);

void killzombies();

int piping(char **line, int *infd, int *outfd);

void got_sig(int sig);

int statements(char *commandline, int infd, int outfd, int errfd);