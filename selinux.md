#原理学习
##概念：DAC & MAC
* DAC，大粒度，进程享有创建者的全部权限。比如以root打开factory进程，那么该厂测进程拥有对系统的所有权限。
* MAC，精细粒度，需要控制进程能够做的每一个事情，比如能否打开摄像头，能否控制马达等等。

觉得selinux能够起到的作用，并没有那么大。手机安全，一个很重要的点，是被人拆包后塞进木马。

selinux的优势，在于限制root的权限，并load很多超级deamon，各自负责自己的模块。比如，上网，从sepolicy来定义，只有某一个deamon有权限去控制，以及只有它有权限去分配哪些app能够控制。

##怎么绕过seAndroid
由于SELinux引入android不久，还有很多不完善的地方。在DEFCON 21上，来自viaForensics的Pau Oliva就演示了几个方法来绕过SEAndroid(https://viaforensics.com/mobile-security/implementing-seandroid-defcon-21-presentation.html)：
1. 用恢复模式（recovery）刷回permissive模式的镜像
2. Su超级用户没有设置SELinux的模式，但是system user系统用户可以。
3. Android通过/system/app/SEAndroidManager.apk来设置SELinux模式，所以只要在recovery模式下将其删除就可以绕过
4. 在Android启动时直接操作内核内存，通过将内核里的unix_ioctl符号改写成reset_security_ops重置LSM（Linux Security Modules）

##调试接口
ps -Z  查看进程的security context，用于身份识别，角色控制等。  
ls -Z 查看文件的security context  
chcon 修改文件的secontext  
restorecon  
seclabel  在selinux中用  

格式为：rule_name source_type target_type : class perm_set

模块说明（用户侧）  
source_type(domain)相关

###target type相关
* file_contexts             restorecon使用它修改文件的label。本质上，是遍历指定文件夹，使用setcon设置文件label
* service_contexts       和file_contexts类似。给service_manager class用。
* property_contexts    更细微粒度，控制每一个property的调用流程。给class property_service用。
* initial_sids                 定义初始sid
* initial_sid_contexts   定义初始sid的上下文。使用场景未知。
* fs_use                        fs支持，挂载时label。语法（fs_use_xattr、fs_use_task、fs_use_trans）
* genfs_contexts         fs不支持，为rootfs/proc/vfat/selinuxfs等。genfscon <fstype><path><label>
* port_contexts           Android未使用，网络相关，如portcon tcp 80 u:object_r:http_port:s0。portcon、netifcon、nodecon。

###class相关
security_classes        定义114种object class，其中5个是android独有。

###perm_set相关
access_vectors          定义每一类class能够执行的permission，如对于file，有open、ioctl等。

###基础规则
type
exec_no_trans

###宏规则
global_macros          定义了一些全局宏缩写。
mls_macros               和global的宏类似
attributes                   type定义，如fs_type、domain、exec_type
type trans

##domain tanslate
\# domain_trans(olddomain, type, newdomain)
\# Allow a transition from olddomain to newdomain   upon executing a file labeled with type. 
\# This only allows the transition; it does not  cause it to occur automatically - use domain_auto_trans 
define(`domain_trans', ` 
allow $1 $2:file { getattr open read execute };  # Old domain may exec the file and transition to the new domain.
allow $1 $3:process transition;
allow $3 $2:file { entrypoint open read execute getattr };  # New domain is entered by executing the file.
allow $3 $1:process sigchld;  # New domain can send SIGCHLD to its caller.
dontaudit $1 $3:process noatsecure;  # Enable AT_SECURE, i.e. libc secure mode.
allow $1 $3:process { siginh rlimitinh };  # XXX dontaudit candidate but requires further study.
')

# domain_auto_trans(olddomain, type, newdomain)
\# Automatically transition from olddomain to newdomain  upon executing a file labeled with type.
define(`domain_auto_trans', `
domain_trans($1,$2,$3)  # Allow the necessary permissions.
type_transition $1 $2:process $3;  # Make the transition occur by default.  This is a selinux standard command
')

some examples
external/sepolicy/adbd.te:domain_auto_trans(adbd, shell_exec, shell)
external/sepolicy/su.te:  domain_auto_trans(shell, su_exec, su)

###file type translate
\# file_type_trans(domain, dir_type, file_type)
\# Allow domain to create a file labeled file_type in a  directory labeled dir_type. 
define(`file_type_trans', `
allow $1 $2:dir ra_dir_perms;  # Allow the domain to add entries to the directory.
allow $1 $3:notdevfile_class_set create_file_perms;  # Allow the domain to create the file.
allow $1 $3:dir create_dir_perms;
')
\# file_type_auto_trans(domain, dir_type, file_type)
\# Automatically label new files with file_type when  they are created by domain in directories labeled dir_type. 
define(`file_type_auto_trans', `
file_type_trans($1, $2, $3)  # Allow the necessary permissions.
type_transition $1 $2:dir $3;  # Make the transition occur by default.
type_transition $1 $2:notdevfile_class_set $3;
')

###SID
external/sepolicy/initial_sids
external/sepolicy/initial_sid_contexts

SD card mount
external/sepolicy/sdcardd.te

external/sepolicy/genfs_contexts
genfscon vfat / u:object_r:vfat:s0
genfscon debugfs / u:object_r:debugfs:s0



##步骤：
1. 首先，需要在file_contexts中定义执行一个文件所需要的权限。
	/system/bin/factory u:object_r:factory_exec:s0

2. 然后，定义type。以及type所具备的权限。
	type diagloader_exec , exec_type, file_type;
	type diagloader ,domain;
	init_daemon_domain(diagloader)
	allow diagloader media_rw_data_file:dir { read open execute_no_trans execute };
	allow diagloader mmcblk_device:blk_file { read open execute_no_trans execute };


#MT6795中的selinux
##关闭selinux的方法
1. 在LK的mt_boot.c中，可以控制是否使能selinux。
	1095 if (g_boot_mode == HTC_DOWNLOAD || g_boot_mode == HTC_DOWNLOAD_RUU){
	1096 sprintf(commanline, "%s androidboot.selinux=disabled", commanline);
	1097 } else {
	1098 sprintf(comman line, "%s androidboot.selinux=permissive", commanline);
	1099 }
	最根本的，还是应该看init.c中，对selinux的配置。
	[    6.045011].(0)[1:init]init: SELinux: security_setenforce(0)
2. 修改init的编译宏，
	cd system/core/init
	export HTC_DASHBOARD_BUILD="true"    #使能该开关后，会security_setenforce(0)
	touch init.c; mm
	m ramdisk-nodeps
	m bootimage-nodeps   #得到一个去掉selinux的boot.img
	要使能selinux，只需要去掉HTC_DASHBOARD_BUILD宏，其余步骤类似。
	unset HTC_DASHBOARD_BUILD   #去掉该编译选项，会security_setenforce(1)

##sepolicy生成过程
m4 -D mls_num_sens=1 -D mls_num_cats=1024 \
                -D target_build_variant=userdebug \
                -D force_permissive_to_unconfined=true \
                -s external/sepolicy/security_classes external/sepolicy/initial_sids external/sepolicy/access_vectors external/sepolicy/global_macros external/sepolicy/mls_macros external/sepolicy/mls external/sepolicy/policy_capabilities external/sepolicy/te_macros device/mediatek/common/sepolicy/te_macros external/sepolicy/attributes external/sepolicy/adbd.te external/sepolicy/app.te 
... ... device/mediatek/common/sepolicy/zygote.te external/sepolicy/roles external/sepolicy/users external/sepolicy/initial_sid_contexts external/sepolicy/fs_use device/mediatek/common/sepolicy/fs_use external/sepolicy/genfs_contexts device/mediatek/common/sepolicy/genfs_contexts external/sepolicy/port_contexts > out/target/product/htc_a55ml_dtul/obj/ETC/sepolicy_intermediates/policy.conf
sed '/dontaudit/d' out/target/product/htc_a55ml_dtul/obj/ETC/sepolicy_intermediates/policy.conf > out/target/product/htc_a55ml_dtul/obj/ETC/sepolicy_intermediates/policy.conf.dontaudit
out/host/linux-x86/bin/checkpolicy -M -c 26 -o out/target/product/htc_a55ml_dtul/obj/ETC/sepolicy_intermediates/sepolicy out/target/product/htc_a55ml_dtul/obj/ETC/sepolicy_intermediates/policy.conf

http://blog.csdn.net/innost/article/details/19299937

##判断selinux是否生效的方法
/sys/fs/selinux
[ro.boot.selinux]: [disabled] 
[selinux.reload_policy]: [1]

###在selinux中控制某一个文件的方法
1. 修改权限控制文件，tee分区。这部分数据将会被压到独立的tee分区。
mediatek/common/sepolicy  diagloader、factory、meta_tst
2. 使用seclabel去控制某一个service

这两种方式有什么区别？没有用在什么场景下？
如何对一个进程的身份进行识别？依据进程的名称，进程的域？

如果selinux启用，那是否意味着，安装app，都需要向系统安装selinux policy文件？如何控制可插拔器件上程序的执行？

Practice
target:
1. When init can load diagloader，try to use diagloader to load /system/bin/factory。
2. try to load factory in sdcard.  （key point: how to label factory in sd card with u:object_r:factory type）
log analysis
注意需要在permissive mode下获取全部log。否则会log不全。
156 diagloader.txt|14489 col 111| ^M[   10.326450].(0)[185:logd.auditd]type=1400 audit(1262304551.560:8): avc: denied { read } for pid=195 comm="factory" name="/"
    dev="mmcblk1p1" ino=1 scontext=u:r:init:s0 tcontext=u:object_r:vfat:s0 tclass=dir permissive=1
157 diagloader.txt|14490 col 111| ^M[   10.328968].(0)[185:logd.auditd]type=1400 audit(1262304551.560:9): avc: denie` { open } for pid=!95 comm="factory" path="/    storage/ext_sd" dev="mmcblk1p1" ino=1 scontext=u:r:init:s0 tcontext=u:object_r:vfat:s0 tclass=dir permissive=1
init domain缺少对vfat type的read和open权限。

注意到，在init可以执行diagloader，而不经过domain translate
external/sepolicy/init.te:allow init rootfs:file execute_no_trans;
external/sepolicy/init.te:allow init system_file:file execute_no_trans;

Target 1, init load diagloader process
Solution 1, add new diagloader.te
type diagloader_exec , exec_type, file_type;
type diagloader ,domain;
allow diagloader media_rw_data_file:dir { read open execute_no_trans execute };
allow diagloader mmcblk_device:blk_file { read open execute_no_trans execute };
Solution 2，因为init执行rootfs中的diagloader没有经过domain translate。diagloader在运行时，它的scontex仍然是init type。因此尝试添加init可执行打开和执行sd卡文件的权限。
allow init media_rw_data_file:dir { read open };
allow init mmcblk_device:blk_file rwx_file_perms;
但blk_file没有定义execute_no_trans，Fail。



Target 2,  label /mnt/media_rw/ext_sd/factory with  u:object_r:factory
Solution 1，label factory bin file with factory_exec type when diagloader mount sdcard system。
Solution 2，label factory bin file in file_contexts。

-rwxr-xr-x root     shell             u:object_r:factory_exec:s0 factory.bak
-rwxr-xr-x root     shell             u:object_r:meta_tst_exec:s0 meta_tst
11      chcon u:object_r:factory_exec:s0 /mnt/media_rw/ext_sd/factory 
12      cp /mnt/media_rw/ext_sd/factory /data/
13      chcon u:object_r:factory_exec:s0 /data/factory 
命令11，chcon:  Could not label /mnt/media_rw/ext_sd/factory with --type=factory_exec:  Operation not supported on transport endpoint
命令11，Fail
命令13，OK

Root cause: exvfat file system not support xattr handle in kernel.  
In MTK init.te, there are two special rules:
allow init rootfs:file execute_no_trans;                       #file_contexts:/sbin(/.*)?           u:object_r:rootfs:s0
allow init system_file:file execute_no_trans;             # file_contexts:   /system(/.*)?         u:object_r:system_file:s0
That means, init process has right to execute any process on rootfs sbin and system directory without domain translate.
This is the reason why init can load diagloader process  even when we don’t define diagloader.te.
 
For MTK factory and meta_tst bin file, they have special selinux context label. Once init try to load them, it should translate its domain to corresponding one.
file_contexts:/system/bin/meta_tst u:object_r:meta_tst_exec:s0
file_contexts:/system/bin/factory u:object_r:factory_exec:s0
So we should restore secontext first, this is ok on ext4 filesystem.
 
Currently MFG request us load factory/meta_tst from sd.  Fail when selinux enforcing.
Because vfat filesystem don’t support xattr(extend attr), which is foundation of selinux label.
I think we can read/write/execute any file on sdcard when we don’t try to change its secontext label.
 
Here is my previous analyze result.

chcon:  Could not label /mnt/media_rw/ext_sd/factory with u:object_r:factory_exec:s0:  Operation not supported on transport endpoint.
 
user mode
chcon -> setfilecon -> setxattr [Fail, errno=95 EOPNOTSUPP] -> strerror(errno) [Operation not supported on transport endpoint]
 
kernel mode
for ex4 fs super block, sb->s_xattr = ext4_xattr_handlers;
for vfat filesystem, can not find the place to set sb->s_xattr
[kernel-3.10/fs/ext4/file.c]  ext4 inode support setxattr also, while exfat not support. 

RAW infomation

[external/libselinux/src/setfilecon.c]
 12     return setxattr(path, XATTR_NAME_SELINUX, context, strlen(context) + 1,  0); 
bionic/libc/include/sys/_errdefs.h|132| __BIONIC_ERRDEF( EOPNOTSUPP     ,  95, "Operation not supported on transport endpoint" )  

[kernel-3.10/fs/xattr.c ]
 802 int    
 803 generic_setxattr(struct dentry *dentry, const char *name, const void *value, size_t size, int flags)
 804 {
... ...
 809     handler = xattr_resolve_name(dentry->d_sb->s_xattr, &name);
 810     if (!handler)
 811         return -EOPNOTSUPP;
 812     return handler->set(dentry, name, value, size, flags, handler->flags);
 813 } 
for ex4 filesystem
sb->s_xattr = ext4_xattr_handlers;  (ext4_mount->mount_bdev->ext4_fill_super)
for vfat filesystem
can not find the place to set sb->s_xattr

[kernel-3.10/fs/ext4/file.c]  ext4 inode support setxattr also, while exfat not support. 
645 const struct inode_operations ext4_file_inode_operations = {
... ...
648     .setxattr   = generic_setxattr,
649     .getxattr   = generic_getxattr,
... ...
654 };   

最后一种方法：
init execute mmcblk_device:blk_file 时，直接切换到factory_exec domain。
但问题是，在meta mode下，需要切换到meta_tst domain下。

try to check whether file secontext can be modified on tool_diag partition. OK
6       cp /system/bin/factory.bak /data/tool_diag/factory
7       ls -Z /data/tool_diag/factory
-rwxr-xr-x root     root              u:object_r:unlabeled:s0 factory
9       chcon u:object_r:factory_exec:s0 /data/tool_diag/factory
chcon u:object_r:system_file:s0 /data/tool_diag/    # not necessary
10      ls -Z /data/tool_diag/factory 
-rwxr-xr-x root     root              u:object_r:factory_exec:s0 factory

just use chcon label factory in tool_diag partition, check sylinux in enforce mode, and then reboot ftm, Pass, FTM test UI appear on screen, and auto test can go on.
If I label this file in file_contexts, it should reach a same result.

domain必须切换，因为blk_file不支持exec_no_trans
切换domain时需要指定filetype。
有factory和meta_tst两个domain需要切换，所以需要至少两个type。不能之用一个vfat type。所以需要re-label type。
验证修改file_contexts和直接chcon两种方式修改文件secontext，均Fail。

分析factory.te，发现allow factory system_file:file execute_no_trans; 而blk_file不支持该操作。Maybe


type格式：
type name attribute system mlstructedsubject
device/htc/common/sepolicy/factory.te:type factory_exec , exec_type, file_type;
external/sepolicy/file.te:type vfat, sdcard_type, fs_type, mlstrustedobject;
但是却没有定义vfat_exec type。

还有一个方法。。。
不是在tool_diag partition下可以修改secontext么。那好。系统启动后，我就把sd卡中的factory和meta_tst copy到tool_diag partition。效果就是，表面上看，是从sd卡中load的，但事实上，是从tool_diag partition load的。
表面上从sd卡load，其实我只是把bin file 拷贝到tool_diag partition，从那里来load的。

很难判断什么时候run完。因为很有可能run完就掉电重启了。
format tool_diag为ext4，目前我是通过download tool_diag.img来做的。
我没有试过不做download，tool_diag partition是否ext4呢。因为我看到当时partition的excel文件中，有定义这个partition为ext4。
嚯嚯，mke2fs -t ext4 /dev/block/platform/mtk-msdc.0/by-name/tool_diag
生效了。后果是tool_diag partition下的文件都丢掉了。可以理解~~~

可以写一个shell script，在diagloader之前运行，将sd card中的factory和meta_tst同步到tool_diag。然后再执行diagloader。
我当时试过从init.rc中load shell script的。成功。
不过那个时候没有加selinux。估计还得做实验了。

factory.te中定义了init执行factory_exec type的文件时，可以自动切换domain到factory。
init_daemon_domain(factory)
define(`init_daemon_domain', `
domain_auto_trans(init, $1_exec, $1)
tmpfs_domain($1)
')

sepolicy中，有type、init_daemon_domain等语句，是在什么地方定义的，是怎么生效的？

255|root@htc_a55ml:/ # mount -t ext4 /dev/block/platform/mtk-msdc.0/by-name/tool_diag /data/tool_diag/
mount: Invalid argument
所以，必须mke2fs的。
/dev/block/platform/mtk-msdc.0/by-name/

调试方法
参考其他的te文件怎么写的。
grep "sepolicy" tags/filelist | xargs grep "x_file_perms" | grep macro
❶ 快速编译sepolicy -> ❷adb push到/data/sepolicy/current目录 ->❸setprop selinux.reload_policy 1 ->❹重新mount sd卡 -> ❺查看label是否正常。
在❸阶段，可通过dmesg | grep -i selinux确认修改是否生效。
启动阶段，不会到data目录下去加载sepolicy。实际验证，启动后也不会从这里加载。需要分析原因。

单独修改genfs_context，没有能够为fat32赋予不同的权限。
. 在genfs_context中，factory和meta_tst的label语句，不生效。
· 把genfs_context中的u:object_r:vfat:s0修改为u:object_r:factory_exec:s0，vfat无法挂载。
· 在genfs_context中单独去掉，该目录的特殊权限u:object_r:proc_cpuinfo:s0已经被去掉。
· 在genfs_context中，/proc/cpuinfo的权限设置为u:object_r:factory_exec:s0，OK。似乎factory_exec没有限制。
· u:object_r:vfat_exec:s0，修改为这个type居然可以挂载上，但挂载之后看，type是u:object_r:vfat:s0
挂载vfat和挂载proc的权限不一样。打开UART中，看下拒绝挂载vfat的audit log。
分析sdcardd，发现由它负责

#参考资料
1.       Verify modified relate domain .te file for your functionality.
1)      After modify .te file then generate new sepolicy at your build environment root path by this command
“make sepolicy”
2)      Copy new sepolicy from your build folder out/target/product/m7/root/ into your PC
3)      Create /data/security/current folder at you device by adb shell mkdir command.
(Please make sure your device have this folder)
4)      Push new sepolicy file into device by this command
“adb push sepolicy /data/security/current”
5)      Reload new sepolicy rule by this command
 “adb shell su 0 setprop selinux.reload_policy 1”
6)      Test your functionality and clean all avc denied log for your parts then include security team review before your check in .te file.
你可以参考一下
不知道MTK是不是这样的

Use audit2allow tool to generate .te policy file
•echo "type=1400 msg=audit(1381419041.046:660117): avc: denied { open } for pid=26237 comm="d.soundrecorder" name="kgdev="tmpfs" ino=2725 scontext=u:r:release_app 0 tcontext=u bject_r evice 0 tclass=chr_file" > release_app.log
•$ audit2allow -i denial.log > release_app.te

