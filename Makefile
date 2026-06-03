
obj-m += wavl_tree_mod.o


wavl_tree_mod-objs := wavltree.o wavl_test.o
obj-m += tree_benchmark_mod.o
tree_benchmark_mod-objs := wavltree.o rbtree_wrapper_mod.o


ccflags-y := -pg -O2

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)


all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean