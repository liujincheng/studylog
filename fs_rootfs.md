##RootFS

流程分析：
使用ubuntu的安装CD安装linux时，会从光盘启动，加载一个完全在内存中运行的linux，可以和正常的linux一样工作。但是在文件系统中创建的所有文件，重启后都丢失。在安装过程中，会检测硬件的类型，选择语言，硬盘分区，然后将系统从光盘上拷贝到硬盘上，设置/bin/init启动文件。

initramfs就是这种用于启动的内存文件系统。

我们之前的Modem都是从flash启动，由于flash烧写很慢，因此希望能够从内存启动，就使用了initramfs。

设置boot从网络启动，先从tftp服务器拷贝内核文件vmlinux，再拷贝文件系统文件fs.cpio到内存的8M开始地址，然后CFE按照正常流程加载vmlinux。修改内核代码，尝试去8M开始的位置去读内存，看是否存在文件系统，如果存在的话，就实用populate_rootfs将initramfs加载为根文件系统，并在roofs中查找/bin/init文件，使用该文件启动linux的第一个进程。我们使用了busybox的init程序作为启动程序。


##initramfs
###配置从initrams的hello world
在内核配置文件中设置INITRAMFS_SOURCE为96328GW/fs时，Modem使用tftp从远程获取到镜像文件，之后就停留不动了。

创建一个helloworld的应用程序，重命令名为init，将之放入到某文件夹，并用mknod在该文件夹中创建/dev/console，将INITRAMFS_SOURCE指向该文件夹。Modem能够启动，且能够运行helloworld的内容。

在上面的文件夹中放入一些小的文件，Modem仍然能够启动，但是如果文件夹稍大，则现象同1.

###CFE中tftp远程加载镜像时解压流程
在readprogsegment()函数中出错。CFE只能读取6M左右的文件，之后就没有内存可用，程序陷入等待状态。整个CFE能够使用的内存是8M（cfe_sdramsize），cfe_sdramsize在运行期间会被修改为64M，但是解压缩后的vmlinux大于8M。

这里的8M是寻址空间的限制，还是软件限制？由于改写cfe_sdramsize不会涉及到对flash的操作，因此尝试将cfe_sdramsize改为16M。

为什么64M内存仍然不能加载整个文件？是内存耗尽吗？还是版本有问题？问什么每次都是停留在6346752（0x60D800）偏移？
```c
 if (buf) {
     xprintf("buf=0x%x, &(info->tftp_data[info->tftp_blkoffset])=0x%x, amtcopy=0x%x\n", buf,&(info->tftp_data[info->tftp_blkoffset]),amtcopy) ;
     memcpy(buf,&(info->tftp_data[info->tftp_blkoffset]),amtcopy);
     xprintf("%s[%d]\n", __FUNCTION__, __LINE__) ;
     buf += amtcopy;
     }
res = readprogsegment(fsctx,ref,(void *)(intptr_t)(signed)ph->p_vaddr, ph->p_filesz,flags); //注意到，memcpy向buf拷贝的时候，buf并不是预先分配，而是直接使用的elf头部中的p_vadrr。
buf=0x8061d600, &(info->tftp_data[info->tftp_blkoffset])=0x80648144, amtcopy=0x200
tftp_fileop_read[645]
buf=0x8061d800, &(info->tftp_data[info->tftp_blkoffset])=0x80648144, amtcopy=0x200
```
通过查看cfe的反汇编程序cfe/build/broadcom/bcm63xx_ram/cfe6328.dis，发现cfe的代码区间为0x80601000-0x806202f0，大小为127728字节。
```
8040a000 T __initramfs_start
80770892 T __initramfs_end
80010000 A _text
807c4c50 A _end
```
##qemu模拟Linux启动过程

##远程加载fs.cpio.gz
修改populate_rootfs，使之能够在此阶段从tftp下载文件。
*  如果在populate_rootfs中读取，那么该怎么调用tftp的接口函数呢？
或者在cfe阶段预先把fs下载下来，保存在一个内存中。然后由initramfs读取。
* 如果由cfe加载文件，那么如何保证存储空间足够，如何保证vmlinux在解压的时候不会把cpio文件覆盖。内核在解压后，使用的地址空间在System.map中都会指出。
* cfe阶段，还不能调用内核的kmalloc，那么rootfs在系统启动后存放的位置是否固定呢？能否计算出来？

通过实验，在cfe阶段，以0x80800000开始的大约7M内存，在内核启动后，仍然保留。因此，大约可以将这7M的空间用来保存fs.cpio.gz，并在boot和vmlinuz之间共享。

但直接调用loadRaw()函数时，可能是有一些校验，导致加载失败。

外部生成cpio.gz
```
.PHONY:ramfs
ramfs:
 mkdir -p $(TARGETS_DIR)/$(PROFILE)/fs.cpio
 cp -a $(TARGETS_DIR)/$(PROFILE)/fs/* $(TARGETS_DIR)/$(PROFILE)/fs.cpio
 cd $(LINUXDIR)
 /bin/sh $(LINUXDIR)/scripts/gen_initramfs_list.sh -o $(TARGETS_DIR)/$(PROFILE)/fs.cpio.gz -u 0 -g 0 $(TARGETS_DIR)/$(PROFILE)/fs.cpio/
```

修改loadramfs，在cfe中下载文件，在populate_rootfs中加载。但是提示下面的错误。将cpio.gz中的init用之前的helloworld替代，没有打印，Modem也没有重启。说明是文件系统做得有问题。
```
VFS: Cannot open root device "31:0" or unknown-block(31,0)
Please append a correct "root=" boot option; here are the available partitions:
Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(31,0)
Rebooting in 1 seconds..HELO
populate_rootfs和最终出错的位置，代码间隔较远。
```
重新ln -s /bin/busybox init，结果Modem可以启动了。但是Modem的网页可以访问，数据库无法创建，串口无法输入。

在关于early_userspace的 中，启动linuxrootfs的方式有三种：1. 所有的驱动和文件系统都built-in在内核中，使用prepare_namespace()挂在内核，2. initrd方式，使用linuxrc加载驱动。3. initramfs，有/init完成prepare_namespace()的事情。通过阅读代码，发现代码/init程序并没有正确的执行。

在boot和kernel中加入crc校验机制，crc正确。

检查Makefile时，把targets/H108N30/fs.cpio/init用ll查看了一把，发现居然改文件指向的是PC的一个绝对路径文件。才发现是Makefile的ln -s写错了。应该是ln -s /bin/busybox $(xxx)的。而且，这里的/bin/busybox写成bin/busybox也会出错。panic()报错：试图杀死init进程。

串口无法输入，可以输出
怀疑是/dev/console没有初始化好。
发现targets/fs/dev/下的所有的字符文件，都被初始化为普通文本文件了。但是flash模式下启动是，这些文件居然也可以工作。
在buildFS2中，使用fakeroot来模拟root权限。把fakeroot去掉后，OK，创建的是字符设备了。fakeroot还有问题

1. 各个文件夹仍然不可以写，生成squashfs时参数需要修改 文件夹可写！ftp/telnet等功能正常
2. 对配置的修改不能保存。比如删除一条WAN连接，当时生效，但是重启后该WAN连接仍然存在。 OK
3. 还有一些iptables方面的错误，不知道是怎么引起的。 没有了
4. 需要优化Makefile。支持同时生成普通的kernel_fs，以及cpio。 OK
5. 制作使用文档。例如如何修改虚拟机，使Modem可以访问虚拟机中的文件。
6. 学习squashfs、ramfs、rootfs的加载等。

从flash启动
```
VFS: Mounted root (squashfs filesystem) readonly on device 31:0.
Freeing unused kernel memory: 168k freed
message received before monitor task is initialized kerSysSendtoMonitorTask
init started: BusyBox v1.01 (2011.03.02-03:08+0000) multi-call binary
Starting pid 225, console /dev/ttyS0: '/etc/init.d/rcS'
Starting pid 229, console /dev/ttyS0: '/bin/sh'
BusyBox v1.01 (2011.03.02-03:08+0000) Built-in shell (ash)
```

参考文档：
1. 从内存启动Modem
2. 修改Modem的PATH的搜索路径，让它首先从/var/bin目录下面开始搜索。方便ftp上传文件。

```
hImage:
 cp -a release/9806h/9806HV2/vmlinux /mnt/proj/vmlinux
 cp -a release/9806h/9806HV2/System.map /mnt/proj/System.map
 cd /mnt/proj/ ; /opt/zte/mips_gcc4.1.2_uClibc0.9.30/bin/mips-linux-uclibc-objcopy -O binary -R .note -R .comment -S vmlinux piggy
 cd /mnt/proj/ ; gzip -9 < piggy > piggy.gz

hfs:
 cp -a release/userapp/fs/MIPS/* /mnt/proj/mipso/fs.cpio/
 /bin/sh tools/gen_initramfs_list.sh -o /mnt/proj/fs.cpio.gz -u 0 -g 0 /mnt/proj/mipso/fs.cpio/

hmulti: hImage hfs
 cd /mnt/proj ; $(curdir)/tools/mkimage -A mips -O linux -T multi -C gzip -a 0x80100000 -e 0x`grep kernel_entry System.map | awk '{ print $$1 }'` -n 'MIPS Linux-2.6.21.7' -d piggy.gz:fs.cpio.gz zImage
```

grep kernel_entry System.map | awk '{ print $1 }' //这里只有一个$

kernel_entry。linux内核启动的第一个阶段是从/arch/mips/kernel/head.s文件开始的。而此处正是内核入口函数kernel_entry()。该函数首先初始化内核堆栈，来为创建系统的第一个进程准备，接着用一段循环将内核镜像的为初始化数据（bss段，在edata和_end之间）清零，然后跳转到/init/main.c中的start_kernel()初始化平台硬件相关的代码。

 
