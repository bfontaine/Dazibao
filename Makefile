# Dazibao Makefile
#

SRC=src
WEBSRC=$(SRC)/web

VALGRIND=valgrind
VALFLAGS=-v --tool=memcheck --leak-check=full --track-origins=yes

CC=gcc
CFLAGS=-g -Wall -Wextra -Wundef -Wpointer-arith -std=gnu99 -I$(SRC)

UTILS=$(SRC)/tlv.h $(SRC)/utils.h
WUTILS=$(WEBSRC)/webutils.h

TARGET=dazibao
SERVER=notification-server
CLIENT=notification-client
WSERVER=daziweb
TARGETS=$(TARGET) $(SERVER) $(CLIENT) $(WSERVER)

# FIXME check how to merge these two 'ifndef'
ifndef UNUSED
ifndef STRICT
CFLAGS+= -Wno-unused-parameter -Wno-unused-variable
endif
endif

ifdef DEBUG
CFLAGS+= -DDEBUG=1 -g
endif

ifdef STRICT
# regarding the use of -O2, see:
# http://www.tldp.org/HOWTO/Secure-Programs-HOWTO/c-cpp.html
CFLAGS+= -Wstrict-prototypes -Werror -O2
endif

CPPCHECK=cppcheck \
	--enable=warning,style \
	--language=c -q

.DEFAULT: all
.PHONY: clean cleantmp check

all: check $(TARGETS)

$(TARGET): main.o dazibao.o tlv.o
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER): $(SERVER).o $(SRC)/notification-server.h
	$(CC) $(CFLAGS) -o $@ $<

$(CLIENT): notification-client.o
	$(CC) $(CFLAGS) -o $@ $^

$(WSERVER): $(WEBSRC)/$(WSERVER).o $(WEBSRC)/request.o $(WEBSRC)/routing.o \
		$(WEBSRC)/routes.o $(WEBSRC)/http.o $(WEBSRC)/webutils.o \
		$(WEBSRC)/html.o \
		dazibao.o tlv.o
	$(CC) $(CFLAGS) -o $@ $^

$(WEBSRC)/%.o: $(WEBSRC)/%.c $(WEBSRC)/%.h $(WUTILS)
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: $(SRC)/%.c $(SRC)/%.h $(UTILS)
	$(CC) $(CFLAGS) -o $@ -c $<

cleantmp:
	rm -f *~ */*~

clean: cleantmp
	rm -f *.o $(SRC)/*.o $(WEBSRC)/*.o

cleanall: clean
	rm -f $(TARGETS)

check: cleantmp
	./utils/stylecheck.sh
	$(CPPCHECK) -I$(SRC) -I$(WEBSRC) $(SRC) $(WEBSRC)

memcheck-%: %
	$(VALGRIND) $(VALFLAGS) ./$<
