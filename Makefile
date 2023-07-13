kpath ?= /usr/src/linux
obj-m := edid_fixer.o

all:
	make -C$(kpath) M=$(CURDIR) modules

clean:
	make -C$(kpath) M=$(CURDIR) clean

.PHONY: all clean
