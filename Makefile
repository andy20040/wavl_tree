
MODULE_NAME := wavl_module
obj-m := $(MODULE_NAME).o


$(MODULE_NAME)-y := wavl_tree.o wavl_test.o


KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)


all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean