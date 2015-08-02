
##这里是一个RCU的例子：
```c
rcu_read_lock();  // read lock, when try to write, wait for it
sighand = rcu_dereference(tsk->sighand); //rcu api to defer pointer
rcu_read_unlock();
```

