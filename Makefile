# Dazibao Makefile
#

CC=gcc
SRC=src
CFLAGS=-Wall -Wextra -Wundef -std=gnu99 -I$(SRC)
UTILS=$(SRC)/dazibao.h $(SRC)/tlvs.h $(SRC)/utils.h
TARGET=dazibao

.DEFAULT: all
.PHONY: clean

all: $(TARGET)

%.o: $(SRC)/%.c $(UTILS)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) *.o *~
