# Dazibao Makefile
#

SRC=src
WEBSRC=$(SRC)/web
NSRC=$(SRC)/notifier

VALGRIND=valgrind
VALFLAGS=-v --tool=memcheck --leak-check=full --track-origins=yes \
	 --show-reachable=yes

CC=gcc
CFLAGS=-g -Wall -Wextra -Wundef -Wpointer-arith -std=gnu99 -pthread -I$(SRC)

DOXYGEN=doxygen
DOXYFLAGS=

UTILS=$(SRC)/tlv.h $(SRC)/utils.h
WUTILS=$(WEBSRC)/webutils.h $(WEBSRC)/http.h
NUTILS=$(NOTIFSRC)/notifutils.h

TARGET=dazibao
NSERVER=notification-server
NCLIENT=notification-client
WSERVER=daziweb
TARGETS=$(TARGET) $(NSERVER) $(NCLIENT) $(WSERVER)

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
.PHONY: clean cleantmp check checkwhattodo doc

all: check $(TARGETS)

$(TARGET): $(SRC)/main.o $(SRC)/dazibao.o $(SRC)/tlv.o $(SRC)/utils.o
	$(CC) $(CFLAGS) -o $@ $^

$(NSERVER): $(NSRC)/$(NSERVER).o $(NSRC)/notifutils.o $(NSRC)/notification-server.h
	$(CC) $(CFLAGS) -o $@ $^

$(NCLIENT): $(NSRC)/notification-client.o
	$(CC) $(CFLAGS) -o $@ $^

$(WSERVER): $(WEBSRC)/$(WSERVER).o $(WEBSRC)/request.o $(WEBSRC)/routing.o \
		$(WEBSRC)/routes.o $(WEBSRC)/http.o $(WEBSRC)/webutils.o \
		$(WEBSRC)/html.o $(WEBSRC)/response.o $(WEBSRC)/mime.o \
		$(SRC)/dazibao.o $(SRC)/tlv.o $(SRC)/utils.o
	$(CC) $(CFLAGS) -o $@ $^

#$(WEBSRC)/%.o: $(WEBSRC)/%.c $(WEBSRC)/%.h $(WUTILS)
#	$(CC) $(CFLAGS) -o $@ -c $<

#$(SRC)/%.o: $(SRC)/%.c $(SRC)/%.h $(UTILS)
%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

#$(NSRC)/%.o: $(NSRC)/%.c $(NSRC)/%.h $(NUTILS)
#	$(CC) $(CFLAGS) -o $@ -c $<


cleantmp:
	find . -name "*~" -delete

clean: cleantmp
	rm -f *.o $(SRC)/*.o $(WEBSRC)/*.o

cleanall: clean
	rm -f $(TARGETS)

check: cleantmp
	./utils/stylecheck.sh
	$(CPPCHECK) -I$(SRC) -I$(WEBSRC) -I$(NSRC) $(SRC) $(WEBSRC) $(NSRC)

memcheck-%: %
	$(VALGRIND) $(VALFLAGS) ./$< $(CLI_OPTS)

doc:
	$(DOXYGEN) $(DOXYFLAGS) docs/doxygen.conf
	@echo '--> open docs/codedoc/html/index.html for the HTML doc'

checkwhattodo:
	@d=$(SRC);c="TO";f="FIX";x="X"; \
	 for s in $${c}DO $${f}ME $${x}XX; do \
		echo "== $$s =="; \
		grep -nI -e $$s -- \
			$$d/*.c $$d/*.h $$d/*/*.c $$d/*/*.h Makefile;\
	done; true
