完成量

应用场景：
完成某一类任务

原理分析：

api接口（以kthread.c中创建内核线程为例）：
```c
init_completion(&create.done);
wait_for_completion(&create.done);
complete(&create->done);
```
