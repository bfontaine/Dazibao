# Dazibao Makefile
#

OS=$(shell uname -s)

SRC=src
WEBSRC=$(SRC)/web
NSRC=$(SRC)/notifier
DCSRC=$(SRC)/cli
TSRC=tests

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

UTILS=$(SRC)/logging.o $(SRC)/utils.o
WUTILS=$(SRC)/logging.o $(SRC)/utils.o $(WEBSRC)/webutils.o $(WEBSRC)/http.o
NSUTILS=$(SRC)/logging.o $(SRC)/utils.o $(NSRC)/hash.o
NCUTILS=$(SRC)/logging.o $(SRC)/utils.o
DCUTILS=$(SRC)/logging.o $(SRC)/utils.o

NSERVER=notification-server
NCLIENT=notification-client
WSERVER=daziweb
DAZICLI=dazicli
TARGETS=$(NSERVER) $(NCLIENT) $(WSERVER) $(DAZICLI)

TESTS=$(TSRC)/utils.test \
      $(TSRC)/logging.test \
      $(TSRC)/tlv.test

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

CPPCHECK_VER:=$(shell cppcheck --version 2>/dev/null)
ifdef CPPCHECK_VER
CPPCHECK=cppcheck \
	--enable=warning,style \
	--language=c -q
else
CPPCHECK=\#
endif

.DEFAULT: all
.PHONY: clean cleantmp check checkwhattodo doc tests

all: check $(TARGETS)

$(DAZICLI): $(DCSRC)/cli.o $(DCSRC)/alternate_cmd.o $(SRC)/mdazibao.o $(SRC)/tlv.o $(DCUTILS)
	$(CC) -o $@ $^ $(CFLAGS)

$(NSERVER): $(NSRC)/$(NSERVER).o $(NSRC)/notification-server.o $(NSUTILS)
	$(CC) $(CFLAGS) -o $@ $^

$(NCLIENT): $(NSRC)/notification-client.o $(NCUTILS)
	$(CC) $(CFLAGS) -o $@ $^

$(WSERVER): $(WEBSRC)/$(WSERVER).o $(WEBSRC)/request.o $(WEBSRC)/routing.o \
		$(WEBSRC)/routes.o $(WEBSRC)/http.o \
		$(WEBSRC)/html.o $(WEBSRC)/response.o $(WEBSRC)/mime.o \
		$(SRC)/mdazibao.o $(SRC)/tlv.o $(WUTILS)
	$(CC) $(CFLAGS) -o $@ $^

$(WEBSRC)/$(WSERVER).o: $(SRC)/mdazibao.o $(WEBSRC)/request.o \
			$(WEBSRC)/routing.o $(WEBSRC)/routes.o
$(WEBSRC)/html.o: $(SRC)/mdazibao.o $(SRC)/tlv.o
$(WEBSRC)/request.o: $(WEBSRC)/http.o
$(WEBSRC)/routes.o: $(SRC)/mdazibao.o $(SRC)/tlv.o $(WEBSRC)/http.o \
			$(WEBSRC)/html.o
$(WEBSRC)/routing.o: $(WEBSRC)/http.o $(WEBSRC)/mime.o
$(WEBSRC)/mime.o: $(SRC)/tlv.o
$(WEBSRC)/%.o: $(WEBSRC)/%.c $(WEBSRC)/%.h $(SRC)/utils.o $(WUTILS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

# find's -delete option is not supported on Nivose

cleantmp:
	find . -name "*~" -exec rm {} \;

clean: cleantmp
	find . -name "*.o" -exec rm {} \;

cleanall: clean
	rm -f $(TARGETS) $(TESTS)

check: cleantmp
	./utils/stylecheck.sh
	@# avoid a failed build because cppcheck doesn't exist or is a wrong
	@# version
	$(CPPCHECK) $(SRC) $(WEBSRC) $(NSRC) $(DCSRC) || true

memcheck-%: %
	$(VALGRIND) $(VALFLAGS) ./$< $(CLI_OPTS)

doc:
	$(DOXYGEN) $(DOXYFLAGS) docs/doxygen.conf
	@echo '--> open docs/codedoc/html/index.html for the HTML doc'

checkwhattodo:
	@d=$(SRC);for s in TODO FIXME XXX; do \
		echo "== $$s =="; \
		grep -nI -e $$s -- \
			$$d/*.c $$d/*.h $$d/*/*.c $$d/*/*.h;\
	done; true

tests: $(TESTS)
	@for f in $^; do \
		g=$${f%%.test};g=$${g##*/}; \
		echo "==> $$g"; \
		./$$f; \
	 done

$(TSRC)/tlv.test: $(TSRC)/test-tlv.o $(SRC)/tlv.o $(SRC)/utils.o \
	$(SRC)/logging.o
$(TSRC)/%.test: $(TSRC)/test-%.o $(SRC)/%.o
	$(CC) $(CFLAGS) -o $@ $^

$(TSRC)/%.o: $(TSRC)/%.c 
	$(CC) $(CFLAGS) -o $@ -c $<
