# Dazibao Makefile
#

SRC=src
WEBSRC=$(SRC)/web

VALGRIND=valgrind
VALFLAGS=-v --tool=memcheck --leak-check=full --track-origins=yes \
	 --show-reachable=yes

CC=gcc
CFLAGS=-g -Wall -Wextra -Wundef -Wpointer-arith -std=gnu99 -pthread -I$(SRC)

UTILS=$(SRC)/tlv.h $(SRC)/utils.h
WUTILS=$(WEBSRC)/webutils.h $(WEBSRC)/http.h

TARGET=dazibao
SERVER=notification-server
CLIENT=notification-client
WSERVER=daziweb
TARGETS=$(TARGET) $(SERVER) $(CLIENT) $(WSERVER)

ifndef UNUSED
#ifndef STRICT
CFLAGS+= -Wno-unused-parameter -Wno-unused-variable
#endif
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
.PHONY: clean cleantmp check checkwhattodo

all: check $(TARGETS)

$(TARGET): $(SRC)/main.o $(SRC)/dazibao.o $(SRC)/tlv.o $(SRC)/utils.o
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER): $(SRC)/$(SERVER).o $(SRC)/notification-server.h
	$(CC) $(CFLAGS) -o $@ $<

$(CLIENT): $(SRC)/notification-client.o
	$(CC) $(CFLAGS) -o $@ $^

$(WSERVER): $(WEBSRC)/$(WSERVER).o $(WEBSRC)/request.o $(WEBSRC)/routing.o \
		$(WEBSRC)/routes.o $(WEBSRC)/http.o $(WEBSRC)/webutils.o \
		$(WEBSRC)/html.o $(WEBSRC)/response.o \
		$(SRC)/dazibao.o $(SRC)/tlv.o $(SRC)/utils.o
	$(CC) $(CFLAGS) -o $@ $^

$(WEBSRC)/%.o: $(WEBSRC)/%.c $(WEBSRC)/%.h $(WUTILS)
	$(CC) $(CFLAGS) -o $@ -c $<

$(SRC)/%.o: $(SRC)/%.c $(SRC)/%.h $(UTILS)
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
	$(VALGRIND) $(VALFLAGS) ./$< $(CLI_OPTS)

checkwhattodo:
	@d=$(SRC);c="TO";f="FIX";x="X"; \
	 for s in $${c}DO $${f}ME $${x}XX; do \
		echo "== $$s =="; \
		grep -nI -e $$s -- \
			$$d/*.c $$d/*.h $$d/*/*.c $$d/*/*.h Makefile;\
	done; true
