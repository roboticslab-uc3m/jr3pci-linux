KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)
EXTRA_CFLAGS	:= -Wall -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common

obj-m	:= jr3pci-driver.o

clean-files	:=	*~

all:
	$(MAKE) -C $(KDIR) M=$(PWD) SUBDIRS=$(PWD) modules

clean modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) SUBDIRS=$(PWD) $@

test:	test.cpp
	g++ -o test test.cpp

jr3mon:	jr3mon.c
	gcc -o jr3mon jr3mon.c -lncurses

reset:  reset.cpp
	g++ -o reset reset.cpp


node:	
	mknod /dev/jr3 c 39 0
