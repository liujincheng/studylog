#Requirement
打开CONFIG_DEBUG_LL会报编译错误，是因为mach-mt6592中定义的addruart_current macro有误，一般是三个参数，但是mtk的实现中却只有一个参数。

#study code
##uart寄存器
查看lk中的uart初始化流程，找到对应的寄存器地址. UART的编号，来源于g_boot_arg->log_port; 普通印log的UART为CFG_UART_LOG :=UART1
```c
uart_init_early()   
    +-> mtk_set_current_uart(UART1);   
       +--> g_uart = uart_base;  uart的base address
```

在uart.c中可以看到各个寄存器的偏移。MT6592支持4个UART，在mt_reg_base.h中可以看到这四个寄存器的基地址。实际上使用哪个uart，取决于preloader的配置。可以在preloader中搜索log_port的关键字。能够看到，根据系统工作在不同的模式下，log_port会有不一样。比如当在META mode下时候，会使用UART4。此外，据了解，MTK会将某个UART作为内部调试使用。如果log_port配置错误，将无法向UART上输出log。
```c
\#define UART_THR(uart)                    (UART_BASE(uart)+0x0)  /* Write only */
\#define UART1_BASE        (IO_PHYS + 0x01002000) 
\#define IO_PHYS             0x10000000
```

##lk中使用uart
uart中的打印语句，最终call到_dputc, 也就是直接操作uart。  
```
_dputc(c)  
    +--> uart_putc(c)  
        +--> mt65xx_reg_sync_writel((unsigned int)c, UART_THR(g_uart));  
```

##kernel中使用uart
主要需要弄清楚uart寄存器的physical address和virtual address
\#define IO_VIRT_TO_PHYS(v) (0x10000000 | ((v) & 0x0fffffff))  
\#define IO_PHYS_TO_VIRT(p) (0xf0000000 | ((p) & 0x0fffffff))  

在kernel的arch/arm/mach-mt6592/include/mach/memory.h中，有定义了phy address和virtual address的转换关系。

我们在iomem中找到这几个uart的物理地址。
```c
cat /proc/iomem                                        
11002000-110020ff : mtk-uart.0
11003000-110030ff : mtk-uart.1
11004000-110040ff : mtk-uart.2
11005000-110050ff : mtk-uart.3
```
##uart在kernel中的初始化
在mtk_uart_init_ports()函数中配置了uart的port。比如
```c
         base = uart->setting->uart_base;
         uart->port.mapbase  = IO_VIRT_TO_PHYS(base);   /* for ioremap */
         uart->port.membase  = (unsigned char __iomem *)base;
```
uart_base来源于在drivers/misc/mediatek/uart/mt6592/98:platform_uart.c文件中定义的全局变量
```c
static struct mtk_uart_setting mtk_uart_default_settings[] =
{
    {          
        //.tx_mode = UART_NON_DMA, .rx_mode = UART_RX_VFIFO_DMA, .dma_mode = UART_DMA_MODE_0,
        .tx_mode = UART_TX_VFIFO_DMA, .rx_mode = UART_RX_VFIFO_DMA, .dma_mode = UART_DMA_MODE_0,
        //.tx_mode = UART_NON_DMA, .rx_mode = UART_NON_DMA, .dma_mode = UART_DMA_MODE_0, 
        .tx_trig = UART_FCR_TXFIFO_1B_TRI, .rx_trig = UART_FCR_RXFIFO_12B_TRI,

        .uart_base = UART1_BASE, .irq_num = UART1_IRQ_ID, .irq_sen = MT_LEVEL_SENSITIVE,
        .set_bit = PDN_FOR_UART1,  .clr_bit = PDN_FOR_UART1,  .pll_id = PDN_FOR_UART1,
        .sysrq = FALSE, .hw_flow = TRUE, .vff = TRUE,
    },
... ...
}
```
其中：  
\#define UART1_BASE (0xF1002000)   可以看出这里定义的是虚拟地址  
\#define UART1_IRQ_ID  MT_UART1_IRQ_ID   顺便看下IRQ的地址  
\#define MT_UART1_IRQ_ID                     (GIC_PRIVATE_SIGNALS + 83)  

##uart write函数
```c
void mtk_uart_write_byte(struct mtk_uart *uart, unsigned int byte)
{
    u32 base = uart->base;              
    reg_sync_writel(byte, UART_THR);
}
```


#验证
1. 修改kernel-3.10/arch/arm/mach-mt6592/include/mach/debug-macro.S
	```c
	+        .macro addruart, rp, rv, tmp
	+        ldr    \rp, =0x11002000                        @ physical
	+        ldr    \rv, =0xF1002000        @ virtual
	+        .endm
	```
2. 使用printascii打印字符串，pass。
只有当遇到'\n'字符，才会停下来。也即待打印的字符串中一定要有'\n'，否则就会一直print下去。

#参考资料：
1. kernel-3.10/arch/arm/kernel/debug.S   common code, 定义了printascii，printhex等函数
2. kernel-3.10/arch/arm/mach-mt6592/include/mach/debug-macro.S   芯片特有uart相关函数，比如addruart/senduart/waituart/busyuart
3. bootable/bootloader/lk/platform/mt6592/uart.c   lk中的uart驱动，可以很方便查到对应的寄存器地址
4. kernel-3.10/drivers/misc/mediatek/uart/uart.c   kernel的uart驱动

