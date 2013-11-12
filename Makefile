# Dazibao Makefile
#

CC=gcc
SRC=src
CFLAGS=-g -Wall -Wextra -Wundef -std=gnu99 -I$(SRC)
UTILS=$(SRC)/dazibao.h $(SRC)/tlv.h $(SRC)/utils.h
TARGET=dazibao

ifdef NO_UNUSED
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

all: check $(TARGET)

$(TARGET): main.o dazibao.o tlv.o
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
