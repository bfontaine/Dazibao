# Dazibao Makefile
#

CC=gcc
SRC=src
CFLAGS=-Wall -Wextra -Wundef -std=gnu99 -I$(SRC)
UTILS=$(SRC)/dazibao.h $(SRC)/tlv.h $(SRC)/utils.h
TARGET=dazibao

CPPCHECK=cppcheck \
	--enable=warning,style \
	--language=c

.DEFAULT: all
.PHONY: clean

all: $(TARGET)

%.o: $(SRC)/%.c $(UTILS)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) *.o *~

check:
	$(CPPCHECK) -I$(SRC) $(SRC)
