TOPSRCDIR = $(shell pwd)
DESTDIR = /usr/local/bin/

export TOPSRCDIR DESTDIR

SUBDIRS = drcomc drcomd kmod 

.PHONY: all clean install
.PHONY: $(SUBDIRS) $(SUBDIRS:%=%-clean) $(SUBDIRS:%=%-install)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean: $(SUBDIRS:%=%-clean)

$(SUBDIRS:%=%-clean):
	$(MAKE) -C $(@:%-clean=%) clean

install: $(SUBDIRS:%=%-install)
	@if [ -a /etc/drcom.conf ]; then \
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

$(SUBDIRS:%=%-install):
	$(MAKE) -C $(@:%-install=%) install

