
TOPSRCDIR ?= ..
DESTDIR ?= $(HOME)/bin

CC = gcc
WARN = -Wall -W -Wstrict-prototypes -Wmissing-prototypes -pedantic
INCLUDE = -I$(TOPSRCDIR)/include
OPTIMIZE = -O2
CFLAGS = $(WARN) $(INCLUDE) $(OPTIMIZE)
LDFLAGS = -lm -lpthread

BINS = drcomd
OBJS = drcomd.o daemon.o cleanup.o dialog.o handle.o init.o keepalive.o login.o logout.o md5.o misc.o passwd.o readconf.o watchport.o getaddr.o log.o
SRCS = drcomd.c daemon.c cleanup.c dialog.c handle.c init.c keepalive.c login.c logout.c md5.c misc.c passwd.c readconf.c watchport.c getaddr.c log.c
INCS = daemon.h log.h

.PHONY: all clean install

all: $(BINS)

clean:
	rm -f $(BINS) $(OBJS)

install: $(BINS)
	mkdir -p $(DESTDIR)
	install -m 700 drcomd $(DESTDIR)

drcomd: $(OBJS)

drcomd.o: $(TOPSRCDIR)/include/config.h daemon.h log.h
daemon.o: $(TOPSRCDIR)/include/config.h $(TOPSRCDIR)/include/client_daemon.h $(TOPSRCDIR)/include/drcom_types.h daemon.h log.h

