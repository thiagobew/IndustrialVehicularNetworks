#!/bin/bash

KERNEL_VERSION=5.10.54
BUSYBOX_VERSION=1.36.0

cd src
	gcc --static -o sendMAC sendMAC.c
	gcc --static -o receiveMAC receiveMAC.c
cd ..

cp src/linux-$KERNEL_VERSION/arch/x86_64/boot/bzImage ./


#initrd
rm -rf initrd
mkdir initrd
cd initrd

	mkdir -p bin dev proc sys
	cd bin
		cp ../../src/busybox-$BUSYBOX_VERSION/busybox ./
		
		for prog in $(./busybox  --list); do
			ln -s ./busybox ./$prog
		done
		
		cp ../../src/sendMAC ./
		ln -s ./sendMAC
		
		
	cd ..
	
	echo '#!bin/sh' > init
	echo 'mount -t sysfs sysfs /sys' >> init
	echo 'mount -t proc proc /proc' >> init
	echo 'mount -t devtmpfs udev /dev' >> init
	echo 'ip link set eth0 up' >> init
	#echo 'sysctl -w kernel.printk=" 2 4  1 7"' >> init 
	echo '/bin/sendMAC 4 &' >> init	
	echo '/bin/sh' >> init	
        echo 'poweroff -f' >>init
	
	
	chmod -R 777 .
	
	find . | cpio -o -H newc > ../initramfs1.img

cd ..

#VM1
xterm -e qemu-system-x86_64 \
	-m 1024 \
	-smp 1 \
	-kernel ./src/linux-$KERNEL_VERSION/arch/x86/boot/bzImage \
        -append "console=ttyS0 root=/dev/ram init=/init earlyprintk=serial net.ifnames=0 nokaslr" \
        -initrd initramfs1.img  \
        -device e1000,id=eth1,netdev=n1,mac=52:54:00:12:34:56 \
        -netdev socket,id=n1,mcast=231.0.0.1:1234 \
        -object filter-dump,id=f1,netdev=n1,file=dump1.dat \
        -nographic &

cd initrd

	mkdir -p bin dev proc sys
	cd bin
		cp ../../src/busybox-$BUSYBOX_VERSION/busybox ./
		
		for prog in $(./busybox  --list); do
			ln -s /bin/busybox ./$prog
		done
		
		cp ../../src/sendMAC ./
		ln -s ./sendMAC
		
		
	cd ..
	
	echo '#!bin/sh' > init
	echo 'mount -t sysfs sysfs /sys' >> init
	echo 'mount -t proc proc /proc' >> init
	echo 'mount -t devtmpfs udev /dev' >> init
	echo 'ip link set eth0 up' >> init
	#echo 'sysctl -w kernel.printk=" 2 4  1 7"' >> init 
	echo '/bin/sendMAC 2 &' >> init	
	echo '/bin/sh' >> init	
        echo 'poweroff -f' >>init
	
	
	chmod -R 777 .
	
	find . | cpio -o -H newc > ../initramfs2.img

cd ..

#VM2
xterm -e qemu-system-x86_64 \
	-m 1024 \
	-smp 1 \
	-kernel ./src/linux-$KERNEL_VERSION/arch/x86/boot/bzImage \
        -append "console=ttyS0 root=/dev/ram init=/init earlyprintk=serial net.ifnames=0 nokaslr" \
        -initrd initramfs2.img  \
        -device e1000,id=eth2,netdev=n2,mac=52:54:00:12:34:57 \
        -netdev socket,id=n2,mcast=231.0.0.1:1234 \
        -object filter-dump,id=f2,netdev=n2,file=dump2.dat \
        -nographic &

cd initrd

	mkdir -p bin dev proc sys
	cd bin
		cp ../../src/busybox-$BUSYBOX_VERSION/busybox ./
		
		for prog in $(./busybox  --list); do
			ln -s /bin/busybox ./$prog
		done
		
		cp ../../src/receiveMAC ./
		ln -s ./receiveMAC
		
		
	cd ..
	
	echo '#!bin/sh' > init
	echo 'mount -t sysfs sysfs /sys' >> init
	echo 'mount -t proc proc /proc' >> init
	echo 'mount -t devtmpfs udev /dev' >> init
	echo 'ip link set eth0 up' >> init
	#echo 'sysctl -w kernel.printk=" 2 4  1 7"' >> init 
	echo '/bin/receiveMAC 1 &' >> init	
	echo '/bin/sh' >> init	
        echo 'poweroff -f' >>init
	
	
	chmod -R 777 .
	
	find . | cpio -o -H newc > ../initramfs3.img

cd ..

#VM3
xterm -e qemu-system-x86_64 \
	-m 1024 \
	-smp 1 \
	-kernel ./src/linux-$KERNEL_VERSION/arch/x86/boot/bzImage \
        -append "console=ttyS0 root=/dev/ram init=/init earlyprintk=serial net.ifnames=0 nokaslr" \
        -initrd initramfs3.img  \
        -device e1000,id=eth3,netdev=n3,mac=52:54:00:12:34:58 \
        -netdev socket,id=n3,mcast=231.0.0.1:1234 \
        -object filter-dump,id=f3,netdev=n3,file=dump3.dat \
        -nographic
