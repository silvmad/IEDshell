CC=gcc
CFLAGS= -Wall
EXEC=iedshell
SRC= shell.c user_input.c redirections.c
OBJ=$(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^

shell.o: shell.c shell.h redirections.h commod.h
	$(CC) -o $@ -c $< $(CFLAGS)

user_input.o: user_input.c user_input.h commod.h
	$(CC) -o $@ -c $< $(CFLAGS)

redirections.o: redirections.c redirections.h commod.h
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean cleanall

clean:
	rm -rf *.o

cleanall: clean
	rm -rf $(EXEC)
