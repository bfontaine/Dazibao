# Dazibao Makefile
#

OS=$(shell uname -s)

SRC=src
WEBSRC=$(SRC)/web
NSRC=$(SRC)/notifier

VALGRIND=valgrind
VALFLAGS=-v --tool=memcheck --leak-check=full --track-origins=yes \
	 --show-reachable=yes

CC=gcc
CFLAGS=-g3 -Wall -Wextra -Wundef -Wpointer-arith -std=gnu99 -I$(SRC)

ifneq ($(OS),Darwin)
	# OS X has POSIX threads in libc
	CFLAGS+= -pthread
endif

DOXYGEN=doxygen
DOXYFLAGS=

UTILS=$(SRC)/tlv.h $(SRC)/utils.h
WUTILS=$(SRC)/logging.h $(WEBSRC)/webutils.h $(WEBSRC)/http.h
NUTILS=$(SRC)/logging.h $(NOTIFSRC)/notifutils.h

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

$(TARGET): $(SRC)/main.o $(SRC)/mdazibao.o $(SRC)/tlv.o $(SRC)/utils.o
	$(CC) -o $@ $^ $(CFLAGS)

$(NSERVER): $(NSRC)/$(NSERVER).o $(SRC)/utils.o $(NSRC)/hash.o $(NSRC)/notification-server.o
	$(CC) $(CFLAGS) -o $@ $^

$(NCLIENT): $(SRC)/utils.o $(NSRC)/notification-client.o
	$(CC) $(CFLAGS) -o $@ $^

$(WSERVER): $(WEBSRC)/$(WSERVER).o $(WEBSRC)/request.o $(WEBSRC)/routing.o \
		$(WEBSRC)/routes.o $(WEBSRC)/http.o $(WEBSRC)/webutils.o \
		$(WEBSRC)/html.o $(WEBSRC)/response.o $(WEBSRC)/mime.o \
		$(SRC)/mdazibao.o $(SRC)/tlv.o $(SRC)/utils.o $(SRC)/logging.o
	$(CC) $(CFLAGS) -o $@ $^

$(WEBSRC)/$(WSERVER).o: $(SRC)/mdazibao.o $(WEBSRC)/request.o \
			$(WEBSRC)/routing.o $(WEBSRC)/routes.o
$(WEBSRC)/html.o: $(SRC)/mdazibao.o $(SRC)/tlv.o
$(WEBSRC)/request.o: $(WEBSRC)/http.o
$(WEBSRC)/routes.o: $(SRC)/mdazibao.o $(SRC)/tlv.o $(WEBSRC)/http.o \
			$(WEBSRC)/html.o
$(WEBSRC)/routing.o: $(WEBSRC)/http.o $(WEBSRC)/mime.o
$(WEBSRC)/%.o: $(WEBSRC)/%.c $(WEBSRC)/%.h $(SRC)/utils.o $(WUTILS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

cleantmp:
	find . -name "*~" -delete

clean: cleantmp
	find . -name "*.o" -delete

cleanall: clean
	rm -f $(TARGETS)

check: cleantmp
	./utils/stylecheck.sh
	@# avoid a failed build because cppcheck doesn't exist or is a wrong
	@# version
	$(CPPCHECK) -I$(SRC) -I$(WEBSRC) -I$(NSRC) $(SRC) $(WEBSRC) $(NSRC) || true

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
