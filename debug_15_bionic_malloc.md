# 在userdebug版本上enable coredump的方法
## Step 1： load libudf.so
在vendor/mediatek/proprietary/external/aee/config_external/init.aee.customer.rc添加:

```
on init
export LD_PRELOAD libsigchain.so:libudf.so
```

## Step 2： prepare to rehook malloc function
remove static before __libc_malloc_dispatch in bionic\libc\bionic\malloc_debug_common.cpp , debug 15 need rehook this variable, eng load can skip this step.

```c
#if 1//def _MTK_MALLOC_DEBUG_
const MallocDebug* __libc_malloc_dispatch = &__libc_malloc_default_dispatch;
#else
static const MallocDebug* __libc_malloc_dispatch = &__libc_malloc_default_dispatch;
#endif
```

## step 3: check preload libudf.so success
  build load and download, after phone reboot, double check libudf.so add to LD_PRELOAD:

```
adb shell “echo $LD_PRELOAD”
```

## step 4:  enable debug 15 through property

```
adb shell setprop persist.libc.debug.malloc 15
adb shell setprop persist.libc.debug15.prog app_process64 (根据发生问题的进程来定)
adb shell setprop persist.debug15.config 0x44020040
adb reboot
```

## step 5： check mmap "malloc debug" success
  After reboot you can check the target process debug 15 enable by check “malloc debug” exist or not, if exist debug15 enabled, this feature will mmap 32M memory for debug.

```
 cat /proc/xxx/maps | grep "malloc debug"
```

# 生成coredump的方法

如果已经是userdebug版本了，手动敲命令把这些属性设置一遍，也是可以的。

1. 在init.mt6735.rc去掉import init.aee.rc前的注释

init.mt6735.rc:6:#import init.aee.rc

2. 修改init.aee.rc，增加user mode下做coredump
那在init.aee.customer.rc增加这个
on property:ro.build.type=user
    write /proc/sys/fs/suid_dumpable 2   
    write /proc/sys/kernel/core_pattern "|/system/bin/aee_core_forwarder /data/core/ %p %s UID=%u GID=%g"
on init
    write /proc/self/coredump_filter 39

3. 修改init.rc放开resource限制。
或使用ulimit命令修改。这个改动放在init.aee.rc中应该也可以，不过我没有试过。
on boot
    # test for coredump
    setrlimit 4 -1 -1


----------------------------------------------------------

	关于debug15无法使能，有几个疑点想请你帮忙确认一下：
1.Step在bionic\libc\bionic\malloc_debug_common.cpp这个文件中，有读取libc.debug.malloc这个property到g_malloc_debug_level，而MTK文档中设置的是persist.libc.debug.malloc，这两者是否等同？
2.Step在malloc_init_impl函数中，有根据g_malloc_debug_level，设定不同的malloc dispatch函数。但是我们没有看到针对该值为15的case。想找你确认，MTK的debug 15是在哪里判断的？
3.Step在libudf.so中，有检查persist.libc.debug.malloc。
4.Step当该值为1时，会fopen libc_malloc_debug_leak.so。我查询了昨天晚上Reynold发给你的system_server的map文件，也没有加载这个库。

查看libudf.so，在该文件中有检查persist.libc.debug.malloc是否为bug 15，并重新rehook __libc_malloc_dispatch. 分析该文件，该可以进行fdleak，mmap检查。

persist.libc.debug15.prog^@^@^@^@^@^@^@persist.debug15.config^@^@%s: pthread_atfork fail。该函数在fork前后打桩。应该与该功能无关。

LD_PRELOAD为事先加载的so。疑问，如果这个so最先加载，那么它怎么去rehook libc.so中的变量呢？此时的log要怎么打印呢？

用gdb连接上去，查看__libc_malloc_dispatch有没有rehook成功。

_MTK_MALLOC_DEBUG_这个宏只有在eng build 时才会被设置。该宏对编译libc.a时用到。

Userdebug和eng build下，编译libc_malloc_debug_leak.so。user build下不会编译到它。
MALLOC_LEAK_CHECK，该选项会设置在编译libc_malloc_debug_leak.so时工具链的编译参数。如果没有设置，会在编译libc时编译错误。
如果使用RCMS ROM，不会包含这个lib。

MTK_USER_SPACE_DEBUG_FW会控制libc的debug function，该flag在htc_a50cml_dtul/ProjectConfig.mk中有设置MTK_USER_SPACE_DEBUG_FW = yes。

libsigchain.so

libudf.so, userspace debug firmware

------------------------------------------------------------------
使用android原生的memory leak方法
1. 设置property
$ adb shell “echo ‘libc.debug.malloc=1’ > /data/local.prop”
$ adb shell chmod 644 /data/local.prop
$ adb reboot
2. 检查libc_malloc_debug_leak.so有没有被映射到
2.1 检查/proc/xxx/maps
2.2 使用gdb连接上去，检查__libc_malloc_dispatch函数
native process有这个进程，比如zygote64

==debug1打开后，无法开进Home
1. 正常mode下，ps查看，system_server没有父进程。log为logcat.txt

2. 只设置libc.debug.malloc，通过logcat | grep leak查看debug1有生效。但是此种情况下，system_server半个小时都无法启动。分析logcat的log，主要是Surface和art的log。log为logcat_leak.txt
在异常的log中，大量下面这种的log。从字面意思看，art在逐个函数确认，切每个函数都有100多ms。该行log由433进程，也即zygote64打印，它占用了大量的时间。
W/art     (  433): Verification of java.lang.String java.util.Scanner.findWithinHorizon(java.util.regex.Pattern, int) took 114.677ms
W/dex2oat ( 1709): Verification of void android.os.Looper.loop() took 110.014ms
nobody    433   1     1974004 93636 ffffffff 929e9400 S zygote64
root      1709  1677  320096 88848 ffffffff f6dc9f84 S /system/bin/dex2oat

art的打印发生位置
art/runtime/verifier/method_verifier.cc  MethodVerifier::VerifyMethod()


系统大概运行了一段时间之后，zygote64发生了一次重启。
root      1677  1     62544  16068 000a2028 8405f984 S zygote64
root      1709  1677  320096 88848 ffffffff f6dc9f84 S /system/bin/dex2oat

app_progress64启动zygote64，zygote启动system_server和各种类。app_process64在init.zygote64_32.rc中启动，如下是对应的命令。
命令为service zygote /system/bin/app_process64 -Xzygote /system/bin --zygote --start-system-server --socket-name=zygote

zygote的主要流程 [http://blog.sina.com.cn/s/blog_60862cad0100wk8j.html]
2.1 首先建立虚拟机启动runtime，初始化Zygote，通过AndroidRuntime对象启动SystemServer：
   runtime.start("com.android.internal.os.ZygoteInit",startSystemServer);
 AndroidRuntime  @frameworks\base\core\jni\AndroidRuntime.cpp
 ZygoteInit  @frameworks\base\core\java\com\android\internal\os\zygoteinit.java
2.2 其次ZygoteInit的main函数中
 SamplingProfilerIntegration.start()装载Zygote;
 registerZygoteSocket()注册ZygoteSocket;
 preloadClasses()装载preloaded classes;           
 preloadResources()装载 preload资源;  
 startSystemServer()调用ForkSystemServer来fork一个systemserver新进程;
 closeServerSocket()关闭ZygoteSocket;
 通过上面的步骤Zygote机制建立起来，用socket通讯方式与ActivityManagerService进行通讯，
 接受ActivityManagerService的命令开始fork应用程序，如下：
      Log.i(TAG, "Accepting command socket connections");
            if (ZYGOTE_FORK_MODE) {
                runForkMode();  //runForkMode()这个函数里面的Zygote.fork()映射到linux进程得到一个pid
            } else {
                runSelectLoopMode();
            }
2.3 SystemServer
system_server不是native的binary，而是java的class。甚至在/system/bin下都没有system_server这样的程序。
frameworks/base/services/core/jni/com_android_server_SystemServer.cpp
 执行startSystemServer()之后，在Zygote上fork了一个进程com.android.server.SystemServer，如下代码：
     String args[] = {
            "--setuid=1000",
            "--setgid=1000",
            "--setgroups=1001,1002,1003,1004,1005,1006,1007,1008,1009,1010,1018,3001,3002,3003",
            "--capabilities=130104352,130104352",
            "--runtime-init",
            "--nice-name=system_server",
            "com.android.server.SystemServer",
        };
        parsedArgs = new ZygoteConnection.Arguments(args);
     pid = Zygote.forkSystemServer(
                    parsedArgs.uid, parsedArgs.gid,
                    parsedArgs.gids, debugFlags, null,
                    parsedArgs.permittedCapabilities,
                    parsedArgs.effectiveCapabilities);
 pid = Zygote.forkSystemServer具体实现  @dalvik/vm/native/dalvik_system_Zygote.c。

2.4 android服务
Android所有服务都是建立在SystemServer之上。SystemServer  @frameworks\base\service\java\com\android\server\systemserver.java
init1这个方法时被Zygote调用来初始化系统的，init1会启动native的服务如SurfaceFlinger,AudioFlinger等等，这些工作做完以后会回调init2来启动Android的service。

2.5 system_server的加载流程
 97     public static void execApplication(String invokeWith, String niceName,
 98             int targetSdkVersion, FileDescriptor pipeFd, String[] args) {
 99         StringBuilder command = new StringBuilder(invokeWith);
100         command.append(" /system/bin/app_process /system/bin --application");
101         if (niceName != null) {
102             command.append(" '--nice-name=").append(niceName).append("'");
103         }
104         command.append(" com.android.internal.os.WrapperInit ");
105         command.append(pipeFd != null ? pipeFd.getInt$() : 0);
106         command.append(' ');
107         command.append(targetSdkVersion);
108         Zygote.appendQuotedShellArgs(command, args);
109         Zygote.execShell(command.toString());

Zygote.java
167     public static void execShell(String command) {
168         String[] args = { "/system/bin/sh", "-c", command };
169         try {
170             Os.execv(args[0], args);
171         } catch (ErrnoException e) {
172             throw new RuntimeException(e);
173         }
174     } 
如果按照这个流程下来，最终还是会使用app_process初始化run_time，加载system_server。

3. 如果设置libc.debug.malloc.program为app_process，system_server使用debug1，但是仍然无法进到home中。

4. 检查app_process的参数，只针对某些app使用debug1。
经过实验，可以通过envrion找到进程的启动参数，检查是否出现system-server关键字。
此种情况下，system_server使用debug1，其他进程，zygnote没有使用debug1，而zygnote使用了debug1。即使这样，仍然无法开进Home。
因为实际实验中，发现libc.so中的很多log都印不出来，不方便判断究竟哪些进程会使用debug1.


1. app_process只有前面3次使用libc malloc。-->限制次数后，可以开到android优化界面。
2. pid在某个范围内时做libc malloc
3. 检查app_process的参数
3.1 


--------------------------------------------------------------------
android原生memory leak工具调试方法学习
malloc_init_impl
    +--> 找libc.debug.malloc这个property
    +--> dlopen(so_name, RTLD_LAZY)
    +--> malloc_debug_initialize = reinterpret_cast<MallocDebugInit>(dlsym(malloc_impl_handle, "malloc_debug_initialize"));
    +--> InitMalloc(malloc_impl_handle, &malloc_dispatch_table, "leak");  初始化malloc_impl_handle
    +--> __libc_malloc_dispatch = &malloc_dispatch_table;

libc.debug.malloc=1
libc.debug.malloc.program=zygote
