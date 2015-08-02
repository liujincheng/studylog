wd_common_drv.c::wk_cpu_callback() 函数中更新checkbit，并踢狗。
有“cpu5: down”，但是没有更新checkbit。说明有执行__cpu_die, 并cpu_notify_nofail(CPU_DEAD | mod, hcpu);，但是cpu5的wk_cpu_callback却没有被执行到，可能被block在什么位置了。以前解决过一个类似的问题，当时把priority设置为6就解决了。

kwdt_thread()  watchdog线程，每个cpu有一个
{
printk_deferred("[WDK], local_bit:0x%x, cpu:%d, check bit0x:%x,RT[%lld]\n", 
   local_bit, cpu, wk_check_kick_bit(), sched_clock());
如果checkbit和local bit相等的时候，就会踢一次狗。
}



