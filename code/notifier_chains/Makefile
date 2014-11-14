obj-m += cdev.o

clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *symvers
	rm -rf *.order
app:
	arm-linux-gcc test.c -o test
