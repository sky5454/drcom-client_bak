TOPSRCDIR = $(shell pwd)
DESTDIR = /usr/local/bin/

export TOPSRCDIR DESTDIR

SUBDIRS = drcomc drcomd kmod 

.PHONY: all clean install

all:
	@for x in $(SUBDIRS); do (cd $$x && make all) || exit 1; done

clean:
	@for x in $(SUBDIRS); do (cd $$x && make clean) || exit 1; done

install:
	@for x in $(SUBDIRS); do (cd $$x && make install) || exit 1; done
	@echo
	@echo
	@echo
	@if [ -f /etc/drcom.conf ]; then \
		echo "====================================" && \
		echo "" && \
		echo "/etc/drcom.conf exists.";\
		echo "" && \
		echo "You May Need to Edit /etc/drcom.conf" && \
		echo "" && \
		echo "====================================" \
		;\
	else\
		install -m 600 drcom.conf /etc/drcom.conf && \
		echo "====================================" && \
		echo "" && \
		echo "Do Not Forget To Edit /etc/drcom.conf" && \
		echo "" && \
		echo "====================================" \
		;\
	fi
	@echo

