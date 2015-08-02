##禁用中断
主要包括以下函数：
    local_irq_save(*flags); //进入临界区时禁止中断, 保存中断状态
    spin_unlock_restore(lock, *flags)
	local_irq_restore(*flags)

spin_trylock()试图获得某个特定的自旋锁，如果该锁已经被争用，那么立刻返回非0值，而不会自旋等待锁被释放；如果获得这个自旋锁，返回0。
	#define raw_spin_trylock(lock) __cond_lock(lock, _raw_spin_trylock(lock))
	# define __cond_lock(x,c)   ((c) ? ({ __acquire(x); 1; }) : 0)


