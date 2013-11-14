
# Dazibao Makefile
#

CC=gcc
SRC=src
WEBSRC=$(SRC)/web
CFLAGS=-g -Wall -Wextra -Wundef -std=gnu99 -I$(SRC)
UTILS=$(SRC)/tlv.h $(SRC)/utils.h
WUTILS=$(WEBSRC)/webutils.h
TARGET=dazibao
SERVER=notification-server
CLIENT=notification-client
WSERVER=daziweb

# FIXME check how to merge these two 'ifndef'
ifndef UNUSED
ifndef STRICT
CFLAGS+= -Wno-unused-parameter -Wno-unused-variable
endif
endif

ifdef DEBUG
CFLAGS+= -DDEBUG=1
endif

ifdef STRICT
CFLAGS+= -Werror
endif

CPPCHECK=cppcheck \
	--enable=warning,style \
	--language=c -q

.DEFAULT: all
.PHONY: clean cleantmp check

all: check $(TARGET) $(SERVER) $(CLIENT)

$(TARGET): main.o dazibao.o tlv.o
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER): $(SERVER).o $(SRC)/notification-server.h
	$(CC) $(CFLAGS) -o $@ $<

$(CLIENT): notification-client.o
	$(CC) $(CFLAGS) -o $@ $^

$(WSERVER): $(WEBSRC)/$(WSERVER).o $(WEBSRC)/request.o $(WEBSRC)/routing.o \
				$(WEBSRC)/routes.o $(WEBSRC)/http.o
	$(CC) $(CFLAGS) -o $@ $^

$(WEBSRC)/%.o: $(WEBSRC)/%.c $(WEBSRC)/%.h $(WUTILS)
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: $(SRC)/%.c $(SRC)/%.h $(UTILS)
	$(CC) $(CFLAGS) -o $@ -c $<

cleantmp:
	rm -f *~ */*~

clean: cleantmp
	rm -f $(TARGET) $(SERVER) $(CLIENT) *.o $(SRC)/*.o $(WEBSRC)/*.o

check: cleantmp
	./utils/stylecheck.sh
	$(CPPCHECK) -I$(SRC) $(SRC)
