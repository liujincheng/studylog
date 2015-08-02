摘要：  
本文介绍中断从entry.S，到通用处理函数，再到具体的handle的详细流程。

##中断的注册

##中断处理
arm和arm64架构分别有自己的entry.S文件，用于处理同步和异步的异常。

```c
.macro irq_handle
    +--> handle_arch_irq(sp)
        +--> gic_handle_irq
```

##中断分发流程
```c
static asmlinkage void __exception_irq_entry gic_handle_irq(struct pt_regs *regs)
{
	u32 irqstat, irqnr;
	struct gic_chip_data *gic = &gic_data[0];
	void __iomem *cpu_base = gic_data_cpu_base(gic);
	//gic寄存器的base address
	do
	{
		irqstat = readl_relaxed(cpu_base + GIC_CPU_INTACK);
		//读取GIC器件寄存器中的irqstat
		irqnr = irqstat & ~0x1c00;
		//一共可以处理1024种中断，其中15~1021号为外部中断，0~15号为CPU间通信的中断。
		if (likely(irqnr > 15 && irqnr < 1021))
		{
			irqnr = irq_find_mapping(gic->domain, irqnr);
			//中断号的转换，这里将外部中断映射为内部实际处理的中断。
			handle_IRQ(irqnr, regs);
			continue;
		}
		if (irqnr < 16)
		{
			writel_relaxed(irqstat, cpu_base + GIC_CPU_EOI);
			#ifdef CONFIG_SMP
			handle_IPI(irqnr, regs);
			#endif
			//直到从GIC寄存器中读不到irqnr时，才会break。
			continue;
		}
		break;
	}
	while (1);
}
```
