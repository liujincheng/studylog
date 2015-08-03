lastpc是mtk用于记录手机最后的pc地址。代码分为两个层次，通用框架层和芯片层。

#芯片
MT6753在device tree中配置了MCUCFG的基地址。
    MCUSYS_MCUCFG@0x10200600 {  
      compatible = "mediatek,MCUSYS_MCUCFG";  
      reg = <0x10200600 0xa00>;  
    };

#驱动

/sys/bus/platform/drivers/lastpc

platform_driver_register(&lastpc_drv.plt_drv); 
driver_create_file(&lastpc_drv.plt_drv.driver, &driver_attr_lastpc_dump);
driver_create_file(&lastpc_drv.plt_drv.driver, &driver_attr_lastpc_reboot_test);


[LAST PC] CORE_0 PC = 0x41e0c29e( + 0x0), FP = 0x41e74400, SP = 0x0
[LAST PC] CORE_1 PC = 0x0( + 0x0), FP = 0x0, SP = 0x0
[LAST PC] CORE_2 PC = 0x0( + 0x0), FP = 0x0, SP = 0x0
[LAST PC] CORE_3 PC = 0x0( + 0x0), FP = 0x0, SP = 0x0
[LAST PC] CORE_4 PC = 0x0( + 0x0), FP = 0x0, SP = 0x0
[LAST PC] CORE_5 PC = 0x0( + 0x0), FP = 0x0, SP = 0x0
[LAST PC] CORE_6 PC = 0x0( + 0x0), FP = 0x0, SP = 0x0
[LAST PC] CORE_7 PC = 0x0( + 0x0), FP = 0x0, SP = 0x0

#last pc in lk
64位系统中，pc地址为64bit，需要使用64位的数据类型来存储。
typedef unsigned int  __u32;
typedef unsigned long long __u64

首先，看以下lk中读写寄存器的操作函数。不是直接使用读取内存的命令读取的。参考了UART寄存器的读写过程。
mt_typedefs.h
#define DRV_Reg32(addr)             INREG32(addr)
#define DRV_WriteReg32(addr, data)  OUTREG32(addr, data)
#define DRV_SetReg32(addr, data)    SETREG32(addr, data)
#define DRV_ClrReg32(addr, data)    CLRREG32(addr, data)
#define WRITE_REGISTER_UINT32(reg, val) \
    (*(volatile unsigned int * const)(reg)) = (val)

int g_is_64bit_kernel = 0;






