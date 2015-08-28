# 概念
内存保护单元(ARM体系方面）(MPU,Memory Protection Unit)，MPU中一个域就是一些属性值及其对应的一片内存。这些属性包括：起始地址、长度、读写权限以及缓存等。ARM940具有不同的域来控制指令内存和数据内存。内核可以定义8对区域，分别控制8个指令和数据内存区域。

域和域可以重叠并且可以设置不同的优先级。域的启始地址必须是其大小的整数倍。另外，域的大小可以4K到4G间任意一个2的指数，如4K，8K，16K.....

# code 分析

drivers/misc/mediatek/emi_mpu/mt6735

相关器件：

mediatek,EMI       记录EMI_BASE_ADDR

mediatek,DEVAPC    EMI irq以及各种配置寄存器

#案例
##ALPS02262866, [HIAUR][MT6795] [issue#1551]EMI MPU violation, Kernel API Dump
這個0xf8448050 physical address, 因為不在DRAM上，kernel 不會去建立mapping. 

比較可能是driver owner自己透過ioremap()去mapping了這塊phy address，並存取導致MPU Violation. 

所以接下來要在ioremap 去攔誰mapping了這個address 

1. 在麻煩請客戶加入debug code，去找mapping這塊address的driver . 
2. 這題的覆現率多大? 
3. 这是在什么场景下测出的问题，一般要測試多久? 
4. 這題在麻煩請客戶優先測試。 
5. 是否真的是單一手機才會打出來 

```c
static void __iomem *__ioremap_caller(phys_addr_t phys_addr, size_t size, 
29 pgprot_t prot, void *caller) 
{
[...]
51 if((phys_addr <= vio_start && phy_end >= vio_start) || 
52 (phys_addr >= vio_start && phy_end <= vio_end) || 
53 (phys_addr <= vio_end && phy_end >= vio_end)){ 
54 printk("[MTK DEBUG] Someone mapping violation area\n"); 
55 printk("[MTK DEBUG] phys_addr = 0x%llx, end=0x%llx\n",phys_addr, phy_end); 
56 BUG(); 
}
```

```log
There is a issue about EMI MPU violation, Log detail as below
>> db.04.KernelAPI.dbg.DEC/__exp_main.txt
Defect Class: Kernel API Dump

Detail Info:
</home/aa/Project/HIAURMLTUHL_Generic_WWE_L50_CRC_Sense70_Stable_447073/kernel-3.10/drivers/misc/mediatek/emi_mpu/mt6795/emi_mpu.c:458> EMI MPU violation.
EMI_MPUS = 0x20010960, EMI_MPUT = 0xf8448050, module is unknown.

Backtrace:
[<ffffffc0004046fc>] kernel_reportAPI+0xa0/0x1a4
[<ffffffc0003ff660>] aee_kernel_exception_api+0xc4/0xf0
[<ffffffc0006ebdb8>] mpu_violation_irq+0x2bc/0x2f8
[<ffffffc00012112c>] handle_irq_event_percpu+0x8c/0x26c
[<ffffffc000121354>] handle_irq_event+0x48/0x78
[<ffffffc0001244fc>] handle_fasteoi_irq+0xb0/0x150
[<ffffffc0001207ac>] generic_handle_irq+0x30/0x4c
[<ffffffc0000848b0>] handle_IRQ+0x60/0xd8
[<ffffffc000081660>] gic_handle_irq+0x64/0x160
[<ffffffc000083e3c>] el1_irq+0x7c/0xe4
[<ffffffc000544094>] ccci_alloc_req+0x124/0x3b4
[<ffffffc00054e6cc>] cldma_rx_collect+0x94/0x864
[<ffffffc00054f098>] cldma_rx_done+0x40/0x144
[<ffffffc0000b8798>] process_one_work+0x150/0x480
[<ffffffc0000b9564>] worker_thread+0x138/0x3c0
[<ffffffc0000bf430>] kthread+0xb0/0xbc
[<ffffffc00008440c>] ret_from_fork+0xc/0x40
[<ffffffffffffffff>] 0xffffffffffffffff

>> kernel.log

<6>[17221.985096] (0)[16967:kworker/0:1][K] 1%概念365mobile_log_d 89
<5>[17227.191141]-(0)[22558:mobile_log_d.wr]kernel_reportAPI,EMI MPU,</home/aa/Project/HIAURMLTUHL_Generic_WWE_L50_CRC_Sense70_Stable_447073/kernel-3.10/drivers/misc/mediatek/emi_mpu/mt6795/emi_mpu.c:458> EMI MPU violation.
<5>[17227.191141]EMI_MPUS = 0x20010920, EMI_MPUT = 0xf8448050, module is unknown.
<5>[17227.191141],0x0
<5>[17227.191162]-(0)[22558:mobile_log_d.wr]ke_queue_request: add new ke work, status 0
<3>[17227.191179]-(0)[22558:mobile_log_d.wr][EMI MPU] axi_id = 124, port_id = 0
<3>[17227.191191]-(0)[22558:mobile_log_d.wr][EMI MPU] _id2mst = 53
<6>[17227.191205]-(0)[22558:mobile_log_d.wr]Vio Addr:0x0 , Master ID:0x0 , Dom ID:0x0, R
<5>[17227.196000]-(0)[22558:mobile_log_d.wr]It's a MPU violation.
<6>[17227.196018]-(0)[22558:mobile_log_d.wr]Clear status.
<3>[17227.196029]-(0)[22558:mobile_log_d.wr]EMI MPU violation.
<3>[17227.196039]-(0)[22558:mobile_log_d.wr][EMI MPU] Debug info start ----------------------------------------
<3>[17227.196051]-(0)[22558:mobile_log_d.wr]EMI_MPUS = 20010920, EMI_MPUT = b8448050.
<3>[17227.196063]-(0)[22558:mobile_log_d.wr]Current process is "mobile_log_d.wr " (pid: 22558).
<3>[17227.196074]-(0)[22558:mobile_log_d.wr]Violation address is 0xf8448050.
<3>[17227.196084]-(0)[22558:mobile_log_d.wr]Violation master ID is 0x920.
<3>[17227.196094]-(0)[22558:mobile_log_d.wr][EMI MPU] axi_id = 124, port_id = 0
<3>[17227.196105]-(0)[22558:mobile_log_d.wr]Violation domain ID is 0x0.
<3>[17227.196115]-(0)[22558:mobile_log_d.wr]Read violation.
<3>[17227.196123]-(0)[22558:mobile_log_d.wr]Corrupted region is 0
<3>[17227.196123]
<3>[17227.196134]-(0)[22558:mobile_log_d.wr][EMI MPU] Debug info end------------------------------------------
..
<6>[17252.582773] (0)[23789:kworker/0:0][K] 10%概念365mobile_log_dmobile_log_d 872
<6>[17262.616271] (0)[23789:kworker/0:0][K] 29%概念365mobile_log_dmobile_log_d 2343
```

## 案例2
MT6592需要使用1.5G的DDR, 原本Hardware就不support 非2^n次方的DRAM Size,完全是靠SW去Walk around.

[在TEE Project], 会因为trusted OS额外使用了MPU去保护Secure OS，所以系统就只剩下一个闲置的MPU可以给DRAM Driver使用。因此DRAM Driver总共只会有一个MPU去保护DRAM Gap.所以会有一个DRAM Gap,无法被MPU保护。 因此若有SW 不预期使用0Xe0000000~0xffffffff. 这段区间的话。将会导致系统挂掉。由于系统挂掉的可能有很多种，所以Debug同仁在分析上面，无法很清楚的判断这次系统挂掉是否跟有人存取GAP DRAM有关系。

所以Debug effort会提高很多。

1. CPU Rank

  > 0x80000000 – 0xAFFFFFFF, Rank0可以使用的範圍，size = 0x30000000.  
  > 0xB0000000 – 0xBFFFFFFF, Rank0的gap，size = 0x10000000  
  >   
  > 0xC0000000 – 0xEFFFFFFF, Rank1可以使用的範圍，size = 0x30000000.  
  > 0xF0000000 – 0xFFFFFFFF, Rank1的gap，size = 0x10000000  

2. MPU是怎麼樣的分佈，是一個cpu上有兩個MPU可以用？比如，AP(linux)側有兩個MPU可用，modem側也有兩個MPU可用？還是整個系統只有兩個MPU？

在92上總共有8個MPU, 其中七個都有固定的driver在使用，只有空下一個沒有任何人使用。
 
3.     DRAM的Gap為什麼一定要用MPU保護？linux對這段區間不建頁表可以解決吧？

目前，給客戶的SW對於這段GAP，已經沒有建立頁表。也就是Kernel是看不到這塊區域的。理想上不該有任何SW會去使用這塊區域，但總有一些driver或application或Hardware會因為programming或程式邏輯的問題，在特殊的場景下誤踩這份區域導致系統掛掉。若沒有MPU保護這段區域，就無法明確知道是誰誤踩了這塊區域導致系統掛掉。

--> [Future]既然Gap的地方你們不做頁表mapping，就不會有driver能直接訪問而不abort吧？理論上linux側的code，只要訪問這個沒有頁表mapping的address都會data abort才對，有什麽樣的例外情況？MPU能阻止DMA的非法訪問嗎？



