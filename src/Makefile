CC=gcc
CFLAGS= -Wall -Wextra -std=gnu99
EXEC= 

all: $(EXEC)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean :
	rm -rf *.o $(EXEC)