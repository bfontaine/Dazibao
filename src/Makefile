CC=gcc
CFLAGS= -Wall -Wextra -std=gnu99
EXEC= dazibao 

all: $(EXEC)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean :
	rm -rf *.o $(EXEC) *~
