
# Dazibao Makefile
#

CC=gcc
SRC=src
CFLAGS=-g -Wall -Wextra -Wundef -std=gnu99 -I$(SRC)
UTILS=$(SRC)/dazibao.h $(SRC)/tlv.h $(SRC)/utils.h $(SRC)/notification-server.h
TARGET=dazibao
SERVER=notification-server
CLIENT=notification-client

ifndef UNUSED
CFLAGS+= -Wno-unused-parameter
endif

ifdef DEBUG
CFLAGS+= -DDEBUG=1
endif

CPPCHECK=cppcheck \
	--enable=warning,style \
	--language=c -q

.DEFAULT: all
.PHONY: clean cleantmp check

all: check $(TARGET) $(SERVER) $(CLIENT)

$(TARGET): main.o dazibao.o tlv.o
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER): notification-server.o
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT): notification-client.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: $(SRC)/%.c $(UTILS)
	$(CC) $(CFLAGS) -o $@ -c $<

cleantmp:
	rm -f *~ */*~

clean: cleantmp
	rm -f $(TARGET) *.o

check: cleantmp
	./utils/stylecheck.sh
	$(CPPCHECK) -I$(SRC) $(SRC)
