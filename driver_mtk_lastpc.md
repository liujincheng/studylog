lastpc是mtk用于记录手机最后的pc地址。代码分为两个层次，通用框架层和芯片层。

#芯片
MT6753在device tree中配置了MCUCFG的基地址。
    MCUSYS_MCUCFG@0x10200600 {  
      compatible = "mediatek,MCUSYS_MCUCFG";  
      reg = <0x10200600 0xa00>;  
    };

而last pc的基地址，则存放在MCUCFG base + 0x410的位置，也就是0x10200410。在该地址中，依次存放8个cpu的pc/sp/lr寄存器。

#驱动

/sys/bus/platform/drivers/lastpc

platform_driver_register(&lastpc_drv.plt_drv); 
driver_create_file(&lastpc_drv.plt_drv.driver, &driver_attr_lastpc_dump);
driver_create_file(&lastpc_drv.plt_drv.driver, &driver_attr_lastpc_reboot_test);


#last pc in lk
64位系统中，pc地址为64bit，需要使用64位的数据类型来存储。在打印pc地址的时候，也需要使用%llx格式，否则会形成混乱。
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



