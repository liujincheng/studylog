在http://git.htc.com:8081/#/c/545664/1中，使用register_reboot_notifier(&htc_reboot_info_reboot_notifier)，在reboot的时候被通知到。

kernel_reboot() -> kernel_restart() -> machine_restart() -> arch_reset()

1. kernel_restart会调用kernel_restart_prepare通知reboot_notifier_list链表下的各个模块执行准备工作，比如备份数据。之后migrate_to_reboot_cpu即CPU 0。再执行所有已注册到system core关机callback。最终调用machine_restart
1. machine_restart首先关闭fiq和irq。由于multi core之间不可能co-work，所以还应该使用machine_shutdown关闭多核。之后调用arch_reset去重置系统。
3. arch_reset更新memory中的reboot_reason，准备ramdump，之后通过wd_sw_reset重置系统。
