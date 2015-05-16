ccflags-m += -I.


MODULES_DIR := /lib/modules/$(shell uname -r)
KERNEL_DIR := ${MODULES_DIR}/build

obj-m := ipt_iftag.o

all:	modules

modules:
	make -C ${KERNEL_DIR} M=$$PWD $@ $(MFLAGS)
modules_install:
	make -C ${KERNEL_DIR} M=$$PWD $@;
	depmod -a;
clean:
	make -C ${KERNEL_DIR} M=$$PWD $@;
	rm -rf modules.order
