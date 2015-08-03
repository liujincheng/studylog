#编译命令  
make bootimage showcommands  显示android过程的命令。showcommands对所有的编译选项都有效。  
make ramdisk-nodeps  直接对rootfs打包  
make bootimage-nodeps  直接对bootimage打包  
makekernel 可以编译kernel，但是生成的文件的名称为obj/KERNEL_OBJ/arch/arm64/boot/Image.gz-dtb.bin  

快速编译的原理：  
修改main.mk，将目标添加到dont_bother_goals列表中，可以在make该目标的时候不去收集Android.mk的信息。
  
alias makekernel="make -C kernel-3.10 O=$PRJTOP/out/target/product/$TARGET_PRODUCT/obj/KERNEL_OBJ ARCH=arm64 CROSS_COMPILE=$PRJTOP/prebuilts/gcc/linux-x86/aarch64/cit-aarch64-linux-android-4.9/bin/aarch64-linux-android- ROOTDIR=$PRJTOP "  
将上述命令添加到.bashrc中  
  
makekernel V=1  a50cml_dtul-om_defconfig  
  
make mrproper  
makekernel  V=1 mrproper  
编译过程中会遇到很多Makefile报错，这是因为在drivers目录下下很多代码文件夹不存在，而Makefile又有相关的obj-y选项导致。将相关代码行注释掉即可。  
-obj-$(CONFIG_MTK_CPU_STRESS)   += cpu_stress/  
+#obj-$(CONFIG_MTK_CPU_STRESS)  += cpu_stress/  
  
make vmlinux  
export  CUSTOM_KERNEL_DCT="htc_a50cml_dtul"  
makekernel  V=1 vmlinux  
  
  
生成kernel的文档  
makekernel  V=1 htmldocs  
查看kernel支持的编译选项，比如htmldocs等。但是该功能依赖xmlto。  
  
编译某一个文件  
  dir/            - Build all files in dir and below  
  dir/file.[oisS] - Build specified target only  
  dir/file.lst    - Build specified mixed source/assembly target only  
                    (requires a recent binutils and recent build (System.map))  
  dir/file.ko     - Build module including final link  
比如makekernel fs  可以快速编译fs目录。这里的目录是相对于ROOTDIR的，而不是相对于当前文件夹的。  
  
快速编译
mm   进入到某一个目录下，快速编译。且不用搜索Android.mk，忒爽。

#控制编译过程
控制哪些包将会被安装到system/下，可以搜索下面这个关键字：  ALL_DEFAULT_INSTALLED_MODULES  
如何控制systemimage中是否包含package  
1. device/mediatek/mt6795/device.mk 中使用PRODUCT_PACKAGES += libnbaio
2. 修改vendor/htc/ftm/hosd/AndroidHosd.mk，$(hide) cp -f $(foreach item,$(HOSD_COMMON_PACKAGE) $(HOSD_PRODUCT_PACKAGE)
  
  
#Android.mk编译选项
修改Android.mk，控制生成的可执行文件的存放路径  
LOCAL_MODULE_PATH = $(TARGET_ROOT_OUT_SBIN)  
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)  
还需要让ramdisk目标依赖于该目标，否则该目标将不会编译到。  
  
控制可执行文件的编译选项  
COMMON_CFLAGS += -DIS_MTK_PLATFORM=0  

在Android编译环境中，目录搜素的方法：diag_dir := $(call include-path-for, diag)
