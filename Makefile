KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)
EXTRA_CFLAGS	:= -Wall -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

obj-m	:= jr3pci-driver.o

clean-files	:=	*~

all:
	$(MAKE) -C $(KDIR) M=$(PWD) SUBDIRS=$(PWD) modules

clean modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) SUBDIRS=$(PWD) $@

# https://stackoverflow.com/a/39895302
install:	modules_install	install-header	modprobe

install-header:
	cp $(PWD)/jr3pci-ioctl.h $(DESTDIR)$(PREFIX)/include/

modprobe:
	modprobe jr3pci-driver

test:	test.cpp
	g++ -o test test.cpp

raw:	raw.cpp
	g++ -o raw raw.cpp

jr3mon:	jr3mon.c
	gcc -o jr3mon jr3mon.c -lncurses

reset:	reset.cpp
	g++ -o reset reset.cpp

node:
	mknod /dev/jr3 c 39 0
