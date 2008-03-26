#

ifneq ($(KERNELRELEASE),)
# call from kernel build system

obj-m	:= drcom.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

all: default app

app: start stop

start stop: tcptrack.h

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install
	/sbin/depmod -ae -F $(KERNELDIR)/System.map $(shell uname -r)

endif

clean:
	rm -rf *.o
	rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers
	rm -rf start stop

