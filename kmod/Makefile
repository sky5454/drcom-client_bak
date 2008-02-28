#
DESTDIR ?= $(HOME)/bin

ifneq ($(KERNELRELEASE),)
# call from kernel build system
obj-m	:= drcom.o
drcom-y := init.o proc.o proto.o
#EXTRA_CFLAGS = -I ../include 

else

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD    := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install: default
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	/sbin/depmod -a $(shell uname -r)

.PHONY: default install

endif

myinstall: drcom.ko
	mkdir -p $(DESTDIR)
	install -m 755 drcom.ko $(DESTDIR)

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers
 
