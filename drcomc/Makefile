DESTDIR ?= /usr/local/bin

CC = gcc
#WARN = -Wall -W -Wstrict-prototypes -Wmissing-prototypes -pedantic
WARN = -Wall
INCLUDE = -I../include
OPTIMIZE = -O2
CFLAGS = $(WARN) $(INCLUDE) $(OPTIMIZE)

BINS = drcomc
OBJS = drcomc.o ../utils/utils.o
SRCS = drcomc.c ../utils/utils.c

.PHONY: all clean install

all: $(BINS)

clean:
	@echo "  CLEAN drcomc"
	@rm -f $(BINS) $(OBJS)

install: $(BINS)
	@echo "  INSTALL drcomc"
	@mkdir -p $(DESTDIR)
	@install -m 755 drcomc $(DESTDIR)

drcomc: $(OBJS)
	@echo "  LD drcomc"
	@$(CC) -o $@ $^ $(LDFLAGS)

drcomc.o: ../include/client_daemon.h

.c.o:
	@echo "  CC $<"
	@$(CC) -c -o $@ $< $(CFLAGS)

