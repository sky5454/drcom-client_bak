
DESTDIR ?= /usr/local/bin

CC = gcc
#WARN = -Wall -W -Wstrict-prototypes -Wmissing-prototypes -pedantic
WARN = -Wall
INCLUDE = -I../include
OPTIMIZE = -g
CFLAGS = $(WARN) $(INCLUDE) $(OPTIMIZE) 
LDFLAGS = -lm -lpthread

BINS = drcomd
OBJS = drcomd.o kmodule.o signal.o dialog.o keepalive.o login.o logout.o md5.o misc.o passwd.o readconf.o watchport.o log.o ../utils/utils.o
SRCS = drcomd.c kmodule.c signal.c dialog.c keepalive.c login.c logout.c md5.c misc.c passwd.c readconf.c watchport.c log.c ../utils/utils.c
INCS = log.h

.PHONY: all clean install

all: $(BINS)

clean:
	@echo "  CLEAN drcomd"
	@rm -f $(BINS) $(OBJS)

install: $(BINS)
	@echo "  INSTALL drcomd"
	@mkdir -p $(DESTDIR)
	@install -m 700 drcomd $(DESTDIR)

drcomd: $(OBJS)
	@echo "  LD drcomd"
	@$(CC) -o $@ $^ $(LDFLAGS)

$(OBJS): drcomd.h log.h md5.h ../include/daemon_kernel.h ../include/client_daemon.h ../include/daemon_server.h  ../include/utils.h

.c.o:
	@echo "  CC $<"
	@$(CC) -c -o $@ $< $(CFLAGS)

