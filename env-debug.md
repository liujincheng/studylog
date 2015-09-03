% 调试环境搭建及使用说明

# gdb
## 使用方法
1. 使用gdb加载core文件。如下所示。可以看到动态链接库的符号没有加载。

  > aarch64-linux-android-gdb system/bin/app_process64 PROCESS_COREDUMP

2. 设置动态链接库的符号

  > set solib-search-path <path to system/lib>
  > set sysroot out/target/product/htc_a50ml/symbols/

第一个命令设置之后，会重新搜索动态链接库。另外一个设置符号表的路径，但是这个命令设置完后不会重新去搜索符号表。

3. 设置代码搜索路径

  > set directories ~/code2/share/MT6753_SHEP/   设置代码搜索路径，默认搜索当前目录和编译目录
  > show directories          Source directories searched: /home/liu/code2/share/MT6753_SHEP:$cdir:$cwd

但是如果vmlinux或符号文件不是local编译，而是从服务器上download下来，符号中使用完整路径，使用set directories不好使。此时可使用如下目录，将符号中的完整路径替换为本地的路径信息。

  > set substitute-path FROM TO

4. 查看现场

  > bt  查看backtrace
  > info thread   查看thread信息。对于KE，查看cpu信息。
  > info registers  查看寄存器信息
  > info frame [frame-number]  可以查看每一层函数调用时的寄存器地址
  > frame <frame-number>   查看某一个frame对应的source code

## 应用场景1，使用gdb查看watchdog time out issue

## 应用场景2，使用gdb查看memory corruption issue

## 常用命令
### 查看数组
*pointer@number_of_elements

## android中生成coredump
1. 在init.mt6735.rc去掉import init.aee.rc前的注释

init.mt6735.rc:6:#import init.aee.rc

2. 修改init.aee.rc，增加user mode下做coredump

```
on property:ro.build.type=user
    write /proc/sys/fs/suid_dumpable 2
    write /proc/sys/kernel/core_pattern "|/system/bin/aee_core_forwarder /data/core/ %p %s UID=%u GID=%g"
on init
    write /proc/self/coredump_filter 39
```

3. 修改init.rc放开resource限制。
或使用ulimit命令修改。这个改动放在init.aee.rc中应该也可以，不过我没有试过。

```
on boot
+    # test for coredump
+    setrlimit 4 -1 -1
+
```

### 效果
实际测试下来，eng mode下只有system_server和surface flinger可以产生dbg文件。user mode下，debuggerd无法产生dbg文件。

 aee mode  process          command   db file         size
 --------  --------------   --------  --------------  ----
 4         system_server    kill -31  db.fatal.00.NE  21423448
 4         surface_flinger  kill -31  db.fatal.01.NE  1412823
 4         drmserver        kill -31  N/A             N/A
 3         drmserver        kill -31  db.02.NE        18450705
 3         debuggerd        kill -31  N/A             N/A

Table: coredump test result

## remote gdb
1. 设置adb，将手机的1234端口号映射到host的1234号。

  > adb forward tcp:1233 tcp:1234

2. 在target上启动gdbserver调试进程

  > gdbserver64 :1234 /system/bin/ljctest

3. 在host上使用gdb连接gdb server

  > aarch64-linux-android-gdb
  > file xxx/ljctest
  > set sysroot out/target/product/htc_a50ml/symbols/
  > set solib-search-path <path to system/lib>   # For A50ML out/target/product/htc_a50ml/system/lib
  > target remote localhost:1234
  > b main
  > continue

# crash
## 安装流程
需要根据target的arch重新编译crash。
```
\\10.33.8.3\rd\Documents\SSD1\SSD1_BSP_Team\7. TOOLS\Crash-tool\termcap-1.3.1.tar.gz
\\10.33.8.3\rd\Documents\SSD1\SSD1_BSP_Team\7. TOOLS\Crash-tool\crash-7.1.0.tar.gz
    1. Build termcap and install
    2. Build crash and install
make target=ARM64 CROSS_COMPILE=aarch64-linux-android-
sudo make install
```

## 命令分类整理：
1. 系统整机信息命令

* dev   打印打开的设备文件
* files  打印所有进程的文件
* fuser  和files配合，打印打开某一个文件的所有进程
* ipcs   打印System IPC信息
* irq     打印各个cpu上的irq信息
* kmem  打印slabinfo、free、vm_info等各种内存信息
* log   相当于dmesg，打印内核log
* mach 当前cpu的arch信息
* mod    加载的模块
* mount   加载的partition
* net  ifconfig接口
* ps   进程列表
* pte    page table entry
* sig  打印各个进程的所有信号处理函数
* sys  打印一些设备信息，用途好像不是太大。
* task 打印某一个进程的状态信息
* vm   打印进程的虚拟地址空间vm_struct信息。
* runq  打印cpu上的run queue
* waitq  打印等待队列上的进程
* whatis  打印某一个变量的类型

2. 堆栈分析

* bt   打印堆栈
* dis  反汇编函数
* p   打印变量的值，比如jiffies
* rd   内存读取，比如读取某一个寄存器所指内存附近的内存
* wr  写内存，比如修改某个地址。但觉得在实时crash上用途要大一些，对ramdump意义不大。
* search  在用户态虚拟地址空间、内核态虚拟地址空间、物理地址空间中搜索变量
* sym  打印符号表system.map
* vtop  将虚拟地址转化为物理地址
* ptov  将物理地址转化为内核虚拟地址

3. 辅助命令

* alias   用于自定义命令
* ascii   按照ascii打印一段内存
* btop  This command translates a hexadecimal address to its page number.
* eval  求表达式的值，相当于计算器。支持“+  -  &  |  ^  *  %  /  <<  >>”
* extend   扩展crash的命令
* foreach  在所有进程上执行某个命令
* repeat   反复执行一个命令
* gdb  调用gdb的命令，实际上，很多命令用不了。
* list    遍历list_head结构体
* struct  结构体辅助函数
* union  枚举辅助函数
* tree   树状结构体辅助函数

# bionic debug
Android提供的原生内存调试工具，可分析进程的内存泄漏。原理，在内存分配与释放的时候打桩。开debug 15复现，抓取system server 的core dump，分析native heap占用情况

## 开启debug 15
需要重新编译bootimg，参考步骤如下：请使用eng版本, 在vendor/mediatek/proprietary/external/aee/config_external/init.aee.customer.rc添加:

```
on init
export LD_PRELOAD libsigchain.so:libudf.so
重新打包bootimage并下载, 开机后用adb输入:
adb shell setprop persist.libc.debug.malloc 15
adb shell setprop persist.libc.debug15.prog app_process
adb shell setprop persist.debug15.config 0x2a003024
adb reboot
```

这时会重启手机，开机后就进入malloc debug方式，此时overhead较重，可能会有一些不预期的ANR(出现ANR时请点击等待)，但这不影响测试

然后观察system_server memory占用情况 ，可透过adb shell showmap [system_server_pid] 确认里面的[heap]占用情况，当出现占用比较大时，如下，
            138632   138564   132179        0     6784        0   131780    2 [heap]

准备向system_server 发送sig 31：adb shell kill -31 [system_server_pid]， 会生成 DB文件，里面包含coredump，请把DB文件和相关的 log(main/kernel/bugreport)一起给我们。（实际验证，需要多次发送31信号才能讲system_server杀死。）

PS： 为确保能生成coredump，建议贵司编译好bootimg，并下完debug 15相关adb命令之后，先尝试发sig31，确保能抓到coredump之后，再去复现问题

# Kernel debug flag
整理kernel中debug相关的宏
```
+CONFIG_IKCONFIG=y
+CONFIG_IKCONFIG_PROC=y
+CONFIG_PANIC_TIMEOUT=5
+CONFIG_KALLSYMS_ALL=y
+CONFIG_SLUB_DEBUG=y
+CONFIG_MTK_MEMCFG=y
+CONFIG_MTK_SCHED_TRACERS=y
+CONFIG_MTK_FTRACE_DEFAULT_ENABLE=y
+CONFIG_MT_DEBUG_MUTEXES=y
+CONFIG_ISR_MONITOR=y
+CONFIG_MT_SCHED_MONITOR=y
+CONFIG_SLUB_DEBUG_ON=y
+CONFIG_DEBUG_SPINLOCK=y
+CONFIG_DEBUG_MUTEXES=y
+CONFIG_PROVE_LOCKING=y
+CONFIG_LOCKDEP=y
+CONFIG_DEBUG_LOCKDEP=y
+CONFIG_TRACE_IRQFLAGS=y
+CONFIG_DEBUG_LIST=y
+CONFIG_DEBUG_PAGEALLOC=y
```

# tombstone

Android debuggerd 源码分析 http://www.tuicool.com/articles/zIBvUjr, debuggerd是android的一个daemon进程，负责在进程异常出错时，将进程的运行时信息dump出来供分析。

有三种方式触发打印：
1. 异常的C/C++程序 。这种程序由bionic的linker自动安装异常信号的处理函数，当程序产生异常信号时，进入信号处理函数，与debuggerd建立。
2. debuggerd。由debuggerd进程与debuggerd daemon建立连接。命令debuggerd -b [<tid>]，在控制台打印tid进程实时堆栈。不加-b参数，会将堆栈打印到/data/tombstones/tombstone_XX文件中。

dumpstate/dumpsys。 收集系统运行状态，打印到控制台或文件中。

# xlog
interface

  > /proc/xlog/setfil
  > /proc/xlog/filters
  > /dev/xlog

可执行程序：

  > /system/bin/xlog
  > ioctl    --create module and set level

support 1024 modules

# Ftrace [待验证]
linux提供的性能分析工具。工具考虑了中断、进程切换对性能的影响。

# KDB [待验证]
可检查内存、进程列表、kmsg，在特定情况下也可以打断电。

# kmemleak [待验证]
对应的接口是/sys/kernel/debug/kmemleak，可通过该命令配置、查看内存泄漏情况。

# Oprofile [待验证]
原生内核支持的性能分析工具，可从内核的所有方面，如中断、进程等分析系统性能热点，也可以分析应用程序的性能。

# promem [待验证]
一个查看各个进程的内存占用情况的命令行工具，如vss、pss、heap。

# systrace [待验证]
用图形化的方式展现ftrace的分析结果。android4.1开始支持。

#T32
1. 新建Trace32的文件夹
  > /home/jincheng/trace
2. 文件拷贝
  > cd /home/jincheng/trace
  > unzip Trace32_SW_2012_Feb.zip
  > mv  Trace32_SW_2012_Feb/files/* .
  > mv bin/pc_linux/config.t32 .
  > chmod -R u+w *
  > chmod a+x bin/pc_linux64/filecvt
  > bin/pc_linux64/filecvt .
3. 设置环境变量
  > export T32SYS=/home/liu/trace
  > export T32TMP=/tmp
  > export T32ID=T32
4. 设置字体
修改config.t32，设置字体
  > SCREEN=
  > FONTMODE=3

  > cd /opt/t32/fonts
  > mkfontdir .
  > xset +fp /home/liu/trace/fonts
  > xset fp rehash

5. 安装依赖库
  > sudo apt-get install libjpeg62

6. 安装arm64补丁  trace32_arm64_patch
* demo/arm64
* bin/pc_linux64/t32marm*


