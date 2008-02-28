TOPSRCDIR ?= ..
DESTDIR ?= $(HOME)/bin

CC = gcc
WARN = -Wall -W -Wstrict-prototypes -Wmissing-prototypes -pedantic
INCLUDE = -I$(TOPSRCDIR)/include
OPTIMIZE = -O2
CFLAGS = $(WARN) $(INCLUDE) $(OPTIMIZE)

BINS = drcomc
OBJS = drcomc.o ../utils/utils.o
SRCS = drcomc.c ../utils/utils.c

.PHONY: all clean install

all: $(BINS)

clean:
	rm -f $(BINS) $(OBJS)

install: $(BINS)
	mkdir -p $(DESTDIR)
	install -m 755 drcomc $(DESTDIR)

drcomc: $(OBJS)

drcomc.o: $(TOPSRCDIR)/include/client_daemon.h

