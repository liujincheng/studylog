摘要：  
本文总结cpu hotplug相关知识。  

v0.1  2015-8-4 理清hotplug的大致流程，细节待完善  

---

系统支持的option放在全局变量supported_cpu_ops中，系统初始化阶段，在device tree中查找相应cpu的enable-method，然后根据该值从全局变量supported_cpu_ops中找到该cpu使用的operation。比如在MT6753中，使用的是如下的结构。
```c
const struct cpu_operations mt_cpu_psci_ops = {
	.name		= "mt-boot",    //和device tree中的enable-method匹配
	.cpu_init	= mt_psci_cpu_init,  //从device tree中读取参数准备
	.cpu_prepare	= mt_psci_cpu_prepare,  //一次性初始化
	.cpu_boot	= mt_psci_cpu_boot,  //启动cpu
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_disable	= mt_psci_cpu_disable,  //由将die的cpu准备
	.cpu_die	= mt_psci_cpu_die,  //将die的cpu自杀
	.cpu_kill	= mt_psci_cpu_kill,  //杀死某个cpu，他杀
#endif
#ifdef CONFIG_ARM64_CPU_SUSPEND
	.cpu_suspend = mt_psci_cpu_suspend,  //自我休眠
#endif
};
```
事实上，mtk的operations_ops的handle，大多数是cpupsci_ops的一个简单包装。psci的全称为Power State Coordination Interface。

##请求cpu上线
```c
__cpu_up(cpu)
    +--> 为seconday cpu上线准备pgd，flush dcache  
    +--> boot_secondary(cpu, idle);
        +--> cpu_ops[cpu]->cpu_boot(cpu); 也即mt_cpu_psci_ops->cpu_boot
    +--> wait_for_completion_timeout(&cpu_running, msecs_to_jiffies(1000));  等1s的时间
```

cpu的hotplug，可以分为两个层次，一个是框架层面，当检测到cpu为idle时，就会尝试去shutdown cpu。shutdown过程中，需要做很多的工作，有arch层面的，也有芯片层面的。芯片层面的工作，委托给cpu_operations回调。

##准备cpu上线
分析cpu的上线，需要分析两个层次的code。一个是向上分析，是场景，也即哪些情况下会触发cpu的online动作，另外一个是向下分析，即实现，分析cpu上线时要做哪些事情，有哪些关键动作。
```c
mt_psci_cpu_boot(cpu)  
    +--> cpu_psci_ops.cpu_boot(cpu);标准psci的cpu上线的实现流程。  
        +--> cpu_psci_cpu_boot  
            +--> psci_ops.cpu_on(cpu_logical_map(cpu), __pa(secondary_entry));  
                +--> psci_cpu_on (cpu_logical_map(cpu), __pa(secondary_entry));  
                    +--> fn = psci_function_id[PSCI_FN_CPU_ON];也即PSCI_0_2_FN64_CPU_ON  
                    +--> invoke_psci_fn(fn, cpuid, entry_point, 0);  
                        +--> device tree中，定义psci为smc，故__invoke_psci_fn_smc  
                            +--> 进入security mode，出口为head.S中定义的secondary_entry  
    +--> spm_mtcmos_ctrl_cpu(cpu, STA_POWER_ON, 1);  mtk增加的spm相关的控制流程  
```

在这段流程中，涉及到几个关键的技术，psci，smc，hvp。

psci，smc, 全称security monitor call，进入security monitor mode执行，也即trustzone mode下。hvc，也就是hypervisor virtualation call。

##cpu切换
接下来看当cpu进入EL2 mode下，启动secondary cpu时需要做的事情。将这段代码看懂，将可以极大加深对armv8架构的理解。  
```c
head.S::secondary_entry()  
    +--> __calc_phys_offset  计算physical address offset  
    +--> el2_setup  为进入el2 mode做准备，比如coprocesser traps，配置spsr寄存器等。  
    +--> secondary_startup 包含启动cpu的各种关键动作，比如mmu，pgtable等。此处仅摘取部分。  
        +--> 将secondary cpu的bring up的栈地址=secondary_dat的地址加载到x21  
        +--> 将mmu加载后，切换到secondary cpu的地址=__secondary_switched加载到x27  
        +--> __enable_mmu  
            +--> 加载向量表的地址vectors到x5，并保存到vbar_el1寄存器。  
            +--> load TTBR0和TTBR1  
            +--> __turn_mmu_on  
                +--> b x27，也即__secondary_switched  
                    +--> secondary_start_kernel  之后使用C代码完成secondary Cpu的kernel启动    
```

##为该cpu初始化kernel
```c
secondary_start_kernel
    +--> 切换cpu的init_mm.pgd，并刷新tlb, bp
    +--> cpumask_set_cpu(cpu, mm_cpumask(mm));   I am coming
    +--> cpu_init(); 设置cpu的栈，per-cpu变量等。
    +--> smp_ops.smp_secondary_init(cpu); 完成平台相关的kernel初始化
    +--> notify_cpu_starting(cpu); I am ready, 广而告之
    +--> calibrate_delay();   cpu calibration？
    +--> smp_store_cpu_info(cpu); per-cpu数据迁移
    +--> set_cpu_online(cpu, true);   come on, I can work now.
    +--> complete(&cpu_running);  通知唤醒该cpu的cpu
    +--> percpu_timer_setup();  setup percpu的timer，local timer
    +--> local_irq_enable/local_fiq_enable
    +--> cpu_startup_entry(CPUHP_ONLINE);  进入idle状态，等待被调度走，或处理irq
        +--> cpu_idle_loop(); 进入idle状态
```

cpu上线的起点为secondary_start_kernel


