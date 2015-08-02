##data structure
```c
struct sighand_struct {
	atomic_t count;
	struct k_sigaction action[_NSIG];  kernel sig action is 64
	spinlock_t siglock;
	wait_queue_head_t signalfd_wqh;
};
```

##main flow 
###sysrq send sig
```c
send_sig_all -> do_send_sig_info(sig, SEND_SIG_FORCED, p, true) -> send_signal(sig, info, p, group);
```

详细函数流程
```c
static void send_sig_all(int sig)
{
	struct task_struct *p;

	read_lock(&tasklist_lock);
	for_each_process(p) {
		if (p->flags & PF_KTHREAD)  /*don't signal kernel thread*/
			continue;
		if (is_global_init(p)) /*init process, pid is 1*/
			continue;

		do_send_sig_info(sig, SEND_SIG_FORCED, p, true); 
		/*second arg is siginfo, which include signo, errno, si_code, _sifields
		 *_sifields can tell where this signal is come from, such as, userspace kill,
		 *timer, rt process, SIGCHLD, exception signal, sigpoll, sigsys*/
		/*SEND_SIG_FORCED don't have explicit source*/

	}
	read_unlock(&tasklist_lock);
}

int do_send_sig_info(int sig, struct siginfo *info, struct task_struct *p,
			bool group)
{
	unsigned long flags;
	int ret = -ESRCH;

	if (lock_task_sighand(p, &flags)) {  //get task sighand, and disable interrupt, spin_lock(sighand->siglock)
		ret = send_signal(sig, info, p, group);
		unlock_task_sighand(p, &flags);  //spin_unlock_irqsave(sighand->siglock, *flags)
	}

	return ret;
}

static inline struct sighand_struct *lock_task_sighand(struct task_struct *tsk,
						       unsigned long *flags)
{
	struct sighand_struct *ret;

	ret = __lock_task_sighand(tsk, flags);  //rcu defer task->sighand, disable interrupts and preempt, spin_lock
	(void)__cond_lock(&tsk->sighand->siglock, ret); //compier option. add "+1" context marker for success one
	return ret;
}
```

###exception sig
cpu运行期间出现同步异常，比如缺页，段错误等，需要使用异常打断当前的执行流程，并发送信号。
```c
do_undefinstr() -> arm64_notify_die() send SIGILL -> when user mode: force_sig_info(info->si_signo, info, current); kernel mode: die()
SIGSEGV/SIGBUS: do_bad_area()/do_translation_fault() -> __do_user_fault(tsk, addr, esr, sig, code, regs); -> force_sig_info(sig, &si, tsk);
```

###kill system call
有三个与信号相关的系统调用，分别用于向适当的目标发送信号。  
他们分别是： kill() tkill, tgkill。以下是这些系统调用的主要函数流程。  
```c
sys_kill (si_code is SI_USER) -> kill_something_info(sig, &info, pid) 
    |-->if pid > 0, kill_pid_info(sig, info, find_vpid(pid));
		|-->group_send_sig_info(sig, info, p); {default send signal to all task's membor(thread) }
			|-->do_send_sig_info(sig, info, p, true); {the last parameter true means send sig to group}
				|-->send_signal(sig, info, p, group);
    |-->pid=0(current tsk group), or pid < -1(-pid tsk group), __kill_pgrp_info(sig, info, pid ? find_vpid(-pid) : task_pgrp(current));
		|-->^C/^Z in this flow
    |-->pid=-1(send to all tsk(not current)) group_send_sig_info(sig, info, p);

int __kill_pgrp_info(int sig, struct siginfo *info, struct pid *pgrp)
{
	struct task_struct *p = NULL;
	int retval, success;

	success = 0;
	retval = -ESRCH;
	do_each_pid_task(pgrp, PIDTYPE_PGID, p) {
		int err = group_send_sig_info(sig, info, p);
		success |= !err;
		retval = err;
	} while_each_pid_task(pgrp, PIDTYPE_PGID, p);
	return success ? 0 : retval;
}
```

以下是发送信号的流程
```c
group_send_sig_info() -> do_send_sig_info() -> send_signal() -> __send_signal()
send_sig() -> send_sig_info() -> do_send_sig_info() -> send_signal() -> __send_signal()
oom_kill_process() -> do_send_sig_info() -> send_signal() -> __send_signal()
``` 

##linux study
###linux static check tool
use same symbol to mark code attribute, such as __user, __kernel, __iomem, __bitwise, __aquire, __release
	http://yarchive.net/comp/linux/sparse.html
	# define __acquires(x)rcu_read_lock__attribute__((context(x,0,1)))   before using x, it should be 0, and after using, it should be 1
	# define __releases(x)rcu_read_lock__attribute__((context(x,1,0)))
	# define __acquire(x)rcu_read_lock__context__(x,1) x ref count "+1"
	# define __release(x)rcu_read_lock__context__(x,-1)  x ref count "-1"
	# define __cond_lock(x,c)rcu_read_lock((c) ? ({ __acquire(x); 1; }) : 0)


