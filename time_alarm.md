摘要：  
本文主要总结Android Alarm，以及相关的power的知识点。

=模块框图
-- alarm
-- alarmtimer   hrtimer
-- rtc interface
-- rtc chip 

=关键数据结构
enum android_alarm_type {
  /* return code bit numbers or set alarm arg */
  ANDROID_ALARM_RTC_WAKEUP,
  ANDROID_ALARM_RTC,
  ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
  ANDROID_ALARM_ELAPSED_REALTIME,
  ANDROID_ALARM_SYSTEMTIME,
  ANDROID_ALARM_TYPE_COUNT,

  ANDROID_ALARM_POWER_ON = 6,      --> 开机状态下的alarm
  ANDROID_ALARM_POWER_ON_LOGO = 7, --> 是否带有logo
};

=关键函数
android alarm在hrtimer的基础上包装，考虑suspend后如何唤醒device。
考虑alarm的两种场景，系统在整个alarm的过程中，一直处于活跃状态，此种模式下，alarm实质就是hrtimer。如果系统在alarm过程中，device要发生susped，此时alar device的suspend函数就会被调用到，在此过程中检索最近需要唤醒的时刻，设置到外部唤醒rtc器件。如果在到达预定时间之前，系统一直处于suspend状态，rtc会通过pmic触发EINT，external interrupt给cpu，cpu resume回来，hrtimer重新开始工作。
==alarm设置
alarm_set(alarm_type, ts)  alarm_dev.c
    +--> alarm_enabled |= alarm_type_mask; 
    +--> devalarm_start(&alarms[alarm_type], timespec_to_ktime(*ts));  
        +--> is_wakeup(type) alarm_start(&alrm->u.alrm, exp); 需要在suspend时唤醒
            +--> alarmtimer_enqueue(base, alarm);
                +--> timerqueue_add(&base->timerqueue, &alarm->node); 向timerqueue中添加alarm node
            +--> hrtimer_start(&alarm->timer, alarm->node.expires, HRTIMER_MODE_ABS);  同时还设置hrtimer
        +--> else            hrtimer_start(&alrm->u.hrt, exp, HRTIMER_MODE_ABS);

==alarm的suspend
alarmtimer_suspend(*dev)，遍历alarm_base下的链表。包括ALARM_BOOTTIME和ALARM_REALTIME两种。具体alarm对应的type，在alarm_dev_init中创建alarm时指定
    +--> rtc = alarmtimer_get_rtcdev(); 获取系统使用的rtc device
    +--> 检查rtc时间，如果在2s之后，就suspend失败，返回-EBUSY。
    +--> rtc_timer_cancel(rtc, &rtctimer); setup局部变量rtctimer。函数的名称有误导
    +--> rtc_timer_start(rtc, &rtctimer, now, ktime_set(0, 0)); 设置alarm
        +--> if (timer->enabled)  rtc_timer_enqueue(rtc, timer);   如果timer已经enable了，那么尝试去删除它。
        +--> rtc_timer_enqueue(rtc, timer);  向rtc queue中添加这个timer。
            +--> timerqueue_add(&rtc->timerqueue, &timer->node);  首先将这个timer添加到timerqueue中
            +--> timerqueue_getnext(&rtc->timerqueue) 获取最近的timer
            +--> __rtc_set_alarm(rtc, &alarm);  如果待向rtc配置的timer是最近的一个，那么向rtc配置
上述函数中，rtc__前缀的函数都是在drivers/rtc/interface.c中定义，也就是属于rtc的驱动

关键打印：在alarmtimer_suspend中，执行rtc_timer_start之前，会打印待配置的rtc timer的时间信息，关键字为"min.tv64"

suspend的方法
其实很简单，就是注册一个platform的device，并定义它的suspend函数，这样就可以在suspend时被调用到。
static const struct dev_pm_ops alarmtimer_pm_ops = {
  .suspend = alarmtimer_suspend,
};

static struct platform_driver alarmtimer_driver = {
  .shutdown = alarm_shutdown,
  .driver = {
    .name = "alarmtimer",
    .pm = &alarmtimer_pm_ops,
  }
};

==alarm resume的流程
alarmtimer_fired  alarm_init()函数中初始化一个hrtimer，设置alarm->timer.function
    +--> alarm_dbg(INT, "alarmtimer_fired \n");
    +--> alarmtimer_dequeue(base, alarm);  base是根据alarm->type得到
    +--> restart = alarm->function(alarm, base->gettime()); 执行alarm的handle
        +--> devalarm_alarmhandler  call到alarm-dev.c中
            +--> devalarm_triggered(devalrm);
                +--> wake_up(&alarm_wait_queue); 唤醒work queue。它会一步步向上报这条alarm消息，直到userspace。
    +--> alarmtimer_enqueue(base, alarm);  如果返回值为restart，还需要重新添加这个alarmtimer

关键打印：alarm_dbg(INT, "alarmtimer_fired \n"); 可以用wake_up搜索
由于alarm_timer的默认打印级别为ANDROID_ALARM_PRINT_INFO，所以这里INT类型的log默认不会打印。
<6>[40834.601909]<1>.(0)[1490:kworker/u16:7]PM: suspend exit 2015-07-15 13:37:39.162452307 UTC

==timer_create系统调用
传统上，应该使用posix系统调用timer_create创建timer。
timer_create(clockid, timer_event_spec, created_timer_id)


==alarm的初始化
alarm_dev_init()
该函数负责定义新的clock，并向系统注册。
    +--> alarmtimer_rtc_timer_init(); 
    +--> 使用posix_timers_register_clock向系统注册CLOCK_REALTIME_ALARM和CLOCK_BOOTTIME_ALARM两种时钟。时钟处理handle均为alarm_clock。
    +--> 初始化alarm_bases，使用CLOCK_REALTIME和CLOCK_BOOTTIME作为上述两种alarm clock的base clock。
    +--> alarmtimer_rtc_interface_setup()按照驱动框架添加alarm timer class
    +--> platform_driver_register(&alarmtimer_driver);   按照驱动框架注册驱动
    +--> platform_device_register_simple("alarmtimer", -1, NULL, 0)   注册一个简单的device
    +--> ws = wakeup_source_register("alarmtimer"); 模块内部的wakeup，用于一些短时间的wakeup。suspend时若低于2s，就使用这个wakeup_source

#define CLOCK_REALTIME      0
#define CLOCK_MONOTONIC     1
#define CLOCK_PROCESS_CPUTIME_ID  2
#define CLOCK_THREAD_CPUTIME_ID   3
#define CLOCK_MONOTONIC_RAW   4
#define CLOCK_REALTIME_COARSE   5
#define CLOCK_MONOTONIC_COARSE    6
#define CLOCK_BOOTTIME      7
#define CLOCK_REALTIME_ALARM    8
#define CLOCK_BOOTTIME_ALARM    9
#define CLOCK_SGI_CYCLE     10  /* Hardware specific */
#define CLOCK_TAI     11



从alarmtimer的初始化函数中可以看出，其本质上是一个hrtimer，只是在此基础上进行包装。RTC负责唤醒设备，此时hrtimer会检测到相应的timer到期，然后调用alarmtimer_fired()函数做alarmtimer相关的到期操作，最后肯定会调用再上一层alarm的handle处理。
void alarm_init(struct alarm *alarm, enum alarmtimer_type type,
    enum alarmtimer_restart (*function)(struct alarm *, ktime_t))
{
  timerqueue_init(&alarm->node);
  hrtimer_init(&alarm->timer, alarm_bases[type].base_clockid,HRTIMER_MODE_ABS);
  alarm->timer.function = alarmtimer_fired;
  alarm->function = function;
  alarm->type = type;
  alarm->state = ALARMTIMER_STATE_INACTIVE;
}



"EINT 206 is pending, total 409"这句打印，在drivers/irqchip/irq-mt-eic.c:1895中由mt_eint_print_status() 函数印出，作用是使用`mt_eint_get_status`打印eint的寄存器，每一个bit位代表一个eint的状态，一共213个eint（在mt6573.dts中的max_eint_num中定义）。在这里，206代表PMIC芯片，CUST_EINT_MT_PMIC_MT6325_NUM=206，在pmic.c的PMIC_EINT_SETTING()中定义。
```
spm_output_wake_reason()  在spm_go_to_dpidle函数中，wake up后会被调用到。
    +--> __spm_output_wake_reason(wakesta, pcmdesc, true, src);  
    +--> if (wakesta->r12 & WAKE_SRC_EINT) mt_eint_print_status();  
```
从这段代码可以看到，spm有众多wake source。这些source在在mt_spm.h中有SPM_WAKE_SRC_LIST定义。无法直接搜索到，是因为所有的WAKE_SOURCE_xxx都是由SPM_WAKE_SRC宏拼装而成。在分析log时，观察r12寄存器，可以得知从dpidle中出来的原因。

mtk_rtc_common: rtc_tasklet_handler start
mtk_rtc_common: alarm time is up
mtk_rtc_common: set al time = 2063/02/04 02:00:59 (1)

```
43976 <6>[34106.509176]<1>.(1)[13563:kworker/u16:0]PM: suspend entry 2015-07-15 08:02:59.114579385 UTC
43994 <6>[34106.619610]<1>.(2)[13563:kworker/u16:0]active wakeup source: PowerManagerService.WakeLocks 
43996 <3>[34106.619643]<1>.(2)[13563:kworker/u16:0]Freezing of tasks aborted after 0.001 seconds
44006 <6>[34106.620940]<1>.(2)[13563:kworker/u16:0]PM: suspend exit 2015-07-15 08:02:59.226343539 UTC
```
在kernel/power/suspend.c中由pm_suspend_marker(annotation)打印，参数表示entry或exit。UTC时间间隔0.11s，jiffes时间间隔也是0.11s，也就是说手机在这两段log之间其实没有睡下去。分析log，suspend时需要freezing task，此时检测到PowerManagerService持有wakeLocks，所以suspend失败，退出suspend。
```
try_to_suspend
    +--> pm_suspend(autosleep_state);
        +--> pm_suspend_marker("entry");
        +--> enter_state(state);   
            +--> PM_SUSPEND_FREEZE => freeze_begin(); 
            +--> suspend_syssync_enqueue(); 
            +--> suspend_prepare(state);
                +--> pm_prepare_console(); 
                +--> pm_notifier_call_chain(PM_SUSPEND_PREPARE); 
                +--> suspend_freeze_processes();
                +--> pm_notifier_call_chain(PM_POST_SUSPEND);
                +--> pm_restore_console(); 
            +--> suspend_finish();  
        +--> pm_suspend_marker("exit");
```
enter_state是suspend的实现函数，有的时候很快就跳出，但如果suspend成功，则会出现大段log。

hibernate/suspend

























