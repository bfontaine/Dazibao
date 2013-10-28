# Dazibao Makefile
#

CC=gcc
SRC=src
CFLAGS=-Wall -Wextra -Wundef -std=gnu99 -I$(SRC)
UTILS=$(SRC)/dazibao.h $(SRC)/tlv.h $(SRC)/utils.h
TARGET=dazibao

ifdef NO_UNUSED
CFLAGS+= -Wno-unused-parameter
endif

CPPCHECK=cppcheck \
	--enable=warning,style \
	--language=c -q

.DEFAULT: all
.PHONY: clean

all: $(TARGET)

%.o: $(SRC)/%.c $(UTILS)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) *.o *~

check:
	@echo "Check for 80+ chars lines..."
	@egrep -n '.{80,}' src/* | cut -f1,2 -d:
	$(CPPCHECK) -I$(SRC) $(SRC)
