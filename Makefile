# msh Makefile
# Austin Corotan, CSCI 352, 01/24/17

CC=gcc
CFLAGS= -g -Wall
FILES= msh.c arg_parse.c builtin.c expand.c strmode.c lineproc.c statement.c
OBJS= ${FILES:.c=.o}

msh: $(OBJS)
	$(CC) $(CFLAGS) -o msh $(OBJS)

clean:
	rm -r msh $(OBJS)

# dependency list

$(OBJS): proto.h global.h statement.h


