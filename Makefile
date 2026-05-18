
obj-m += wavl_tree_mod.o


wavl_tree_mod-objs := wavltree.o wavl_test.o


KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)


all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean