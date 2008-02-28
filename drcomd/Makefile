
TOPSRCDIR ?= ..
DESTDIR ?= $(HOME)/bin

CC = gcc
WARN = -Wall -W -Wstrict-prototypes -Wmissing-prototypes -pedantic
INCLUDE = -I$(TOPSRCDIR)/include
OPTIMIZE = -O2
CFLAGS = $(WARN) $(INCLUDE) $(OPTIMIZE)
LDFLAGS = -lm -lpthread

BINS = drcomd
OBJS = drcomd.o kmodule.o signal.o dialog.o keepalive.o login.o logout.o md5.o misc.o passwd.o readconf.o watchport.o log.o ../utils/utils.o
SRCS = drcomd.c kmodule.c signal.c dialog.c keepalive.c login.c logout.c md5.c misc.c passwd.c readconf.c watchport.c log.c ../utils/utils.c
INCS = log.h

.PHONY: all clean install

all: $(BINS)

clean:
	rm -f $(BINS) $(OBJS)

install: $(BINS)
	mkdir -p $(DESTDIR)
	install -m 700 drcomd $(DESTDIR)

drcomd: $(OBJS)

drcomd.o: log.h

