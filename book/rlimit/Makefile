obj-m += cdev.o

PHONY := open

clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *symvers
	rm -rf *.order
	rm -rf open fork

app:
	arm-linux-gcc -o fork fork.c
	arm-linux-gcc -o open open.c
