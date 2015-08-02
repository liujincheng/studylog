#printk学习笔记


struct console *console_drivers;   链表保存所有的console

printk可能丢log
比如last_kmsg_shutdown_1.txt中的下面这句log，在kmsg中不存在。log太多，来不及打印，被丢掉。
[  384.271628]<0> (0)[107:hps_main][HPS] (0200)(1)action end(22)(15)(0)(0) 

last kmsg中只能记录级别为4和5的log。


printk
    +--> vprintk_emit(0, -1, NULL, 0, fmt, args);
        +--> text_len = vscnprintf(text, sizeof(textbuf), fmt, args);
        +--> printascii(text);
        +--> 整理debug level
        +--> if (console_trylock_for_printk(this_cpu))   console_unlock(); 
            +--> 

printk的打印级别怎么控制？

为尽早可以看到printk的打印，需要尽早调用串口的初始化函数，便于printk写数据。
console_initcall(bcm63xx_console_init);    --> 调用console_initcall宏声明，创建一个函数指针，指向bcm63xx_console_init。该函数向printk模块注册一个console。
init.h中声明了console_init宏。
#define console_initcall(fn) \
 static initcall_t __initcall_##fn \                 --> 创建静态函数指针__initcall_bcm63xx_console_init。并将该指针放入到.con_initcall.init段。
 __used __section(.con_initcall.init) = fn

linux中串口驱动包括两个主要的结构体：uart_driver和uart_port。
uart_driver是串口驱动的入口，需要注册到linux内核驱动框架中。包括驱动名称，导出到用户态时设备的名称ttyS，主设备号为4，从设备号为64，支持两个端口(nr)。另外，还需要指定该驱动使用的struct console对象。该对象主要供内核printk打印时输出用。
一个驱动可以对应多个端口，用uart_port来表示，用于存储端口的配置信息，如fifo大小，uartclk、寄存器的地址membase。
static struct uart_driver bcm63xx_reg = {
    .owner = THIS_MODULE,
    .driver_name = "bcmserial",
    .dev_name = "ttyS",
    .major = TTY_MAJOR,        
    .minor = 64,
    .nr = UART_NR,
    .cons = BCM63XX_CONSOLE,
};
使用uart_register_driver注册驱动，使用uart_add_one_port为驱动添加端口。


