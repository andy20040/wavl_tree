
obj-m += wavl_tree_mod.o


wavl_tree_mod-objs := wavltree.c wavl_test.c


KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)


all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean