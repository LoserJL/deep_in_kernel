KERN_DIR := /lib/modules/$(shell uname -r)/build

obj-m := bitmap.o

all:
	make -C $(KERN_DIR) M=$(CURDIR) modules

clean:
	make -C $(KERN_DIR) M=$(CURDIR) clean
