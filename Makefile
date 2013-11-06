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
.PHONY: clean

all: check $(TARGET)

$(TARGET): main.o dazibao.o tlv.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: $(SRC)/%.c $(UTILS)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) *.o *~

check:
	@T=$$(mktemp); \
	 egrep -n --include=.*\.[ch]$$ '.{80,}' src/* >$$T; \
	 if [ "$$?" -eq "0" ]; then \
		 echo '!! There are 80+ chars lines:'; \
		 cat $$T | cut -f1,2 -d:; \
	 fi; \
	 egrep -n --include=.*\.[ch]$$ ' +$$' src/* >$$T; \
	 if [ "$$?" -eq "0" ]; then \
		 echo '!! There are trailing spaces:'; \
		 cat $$T | cut -f1,2 -d:; \
	 fi; \
	 rm -f $TT;
	$(CPPCHECK) -I$(SRC) $(SRC)
