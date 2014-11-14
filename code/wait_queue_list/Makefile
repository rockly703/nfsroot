obj-m += cdev.o

app:
	arm-linux-gcc read.c -o read
	arm-linux-gcc write.c -o write

clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *symvers
	rm -rf *.order
	rm -rf read
	rm -rf write
