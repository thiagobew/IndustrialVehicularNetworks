#!/bin/bash

KERNEL_VERSION=5.15.6
BUSYBOX_VERSION=1.34.1


mkdir -p src
cd src
	#Kernel
	if [ -f "linux-$KERNEL_VERSION.tar.xz" ]; then	
		echo "File is found"
		KERNEL_MAJOR=$(echo $KERNEL_VERSION | sed 's/\([0-9]*\)[^0-9].*/\1/')
		#wget https://mirrors.edge.kernel.org/pub/linux/kernel/v$KERNEL_MAJOR.x/linux-$KERNEL_VERSION.tar.xz	
		#tar -xf linux-$KERNEL_VERSION.tar.xz
		cd linux-$KERNEL_VERSION
		make defconfig
		make -j8 || exit
	
	
	else 
		echo "File is not found"
		KERNEL_MAJOR=$(echo $KERNEL_VERSION | sed 's/\([0-9]*\)[^0-9].*/\1/')
		wget https://mirrors.edge.kernel.org/pub/linux/kernel/v$KERNEL_MAJOR.x/linux-$KERNEL_VERSION.tar.xz	
		tar -xf linux-$KERNEL_VERSION.tar.xz
		cd linux-$KERNEL_VERSION
		make defconfig
		make -j8 || exit
	fi
	
		cd ..
		
	
	# Busybox
	if [ -f "busybox-$BUSYBOX_VERSION.tar.bz2" ]; then
		echo "File is found"
		#wget https://www.busybox.net/downloads/busybox-$BUSYBOX_VERSION.tar.bz2
		#tar -xf busybox-$BUSYBOX_VERSION.tar.bz2
		cd busybox-$BUSYBOX_VERSION
		#make defconfig
		sed 's/^.*CONFIG_STATIC[^_].*$/CONFIG_STATIC=y/g' -i .config
		make  -j8 busybox || exit
	else
		echo "File is not found"
		wget https://www.busybox.net/downloads/busybox-$BUSYBOX_VERSION.tar.bz2
		tar -xf busybox-$BUSYBOX_VERSION.tar.bz2
		cd busybox-$BUSYBOX_VERSION
		make defconfig
		sed 's/^.*CONFIG_STATIC[^_].*$/CONFIG_STATIC=y/g' -i .config
		make  -j8 busybox || exit
	
	fi
		
	
		cd ..
	gcc --static -o sender sender.c
	gcc --static -o receiver receiver.c
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
			ln -s /bin/busybox ./$prog
		done
		
		cp ../../src/sender ./
		ln -s ./sender
		
		
	cd ..
	
	echo '#!bin/sh' > init
	echo 'mount -t sysfs sysfs /sys' >> init
	echo 'mount -t proc proc /proc' >> init
	echo 'mount -t devtmpfs udev /dev' >> init
	echo 'sysctl -w kernel.printk=" 2 4  1 7"' >> init 
	echo '/bin/sender' >> init	
	echo '/bin/sh' >> init	
        echo 'poweroff -f' >>init
	
	
	chmod -R 777 .
	
	find . | cpio -o -H newc > ../initrd_sender.img

cd ..


cd initrd

	mkdir -p bin dev proc sys
	cd bin
		cp ../../src/busybox-$BUSYBOX_VERSION/busybox ./
		
		for prog in $(./busybox  --list); do
			ln -s /bin/busybox ./$prog
		done
		
		cp ../../src/receiver ./
		ln -s ./receiver
		
		
	cd ..
	
	echo '#!bin/sh' > init
	echo 'mount -t sysfs sysfs /sys' >> init
	echo 'mount -t proc proc /proc' >> init
	echo 'mount -t devtmpfs udev /dev' >> init
	echo 'sysctl -w kernel.printk=" 2 4  1 7"' >> init 
	echo '/bin/receiver' >> init	
	echo '/bin/sh' >> init	
	echo 'poweroff -f' >>init
	
	
	chmod -R 777 .
	
	find . | cpio -o -H newc > ../initrd_receiver.img

cd ..



xterm -e qemu-system-x86_64 \
	-m 512 \
	-smp 1 \
	-kernel bzImage \
	-append "console=ttyS0" \
	-initrd initrd_sender.img \
	-device e1000,netdev=n3,mac=56:34:12:00:54:08 \
	-netdev socket,id=n3,mcast=230.0.0.1:1234 \
	-nographic \
	-object filter-dump,id=f3,netdev=n3,file=dump1.dat &
	

xterm -e qemu-system-x86_64 \
	-m 512 \
	-smp 1 \
	-kernel bzImage \
	-append "console=ttyS0 " \
	-initrd initrd_receiver.img \
	-device e1000,netdev=n2,mac=56:34:12:00:54:09 \
	-netdev socket,id=n2,mcast=230.0.0.1:1234 \
	-nographic \
	-object filter-dump,id=f2,netdev=n2,file=dump2.dat &
