驱动学习总结
刘金成

---
##Linux设备驱动

* NAPI
* DMA

##XTM驱动

PTM设备要正常工作，需要做很多配置工作，包括以下：
* PTM设备的初始化。包括buffer初始化等。
* 创建一个PTM接口，比如ptm0。
* 设置PTM设备的发包DMA队列，供QoS调度使用
 
对于驱动而言，配置的入口点是bcmxtmrt_request。它处理来自bcmxtmcfg0字符设备的各种配置。该字符设备用户态对应的配置文件是xtm.c，内核态对应的是bcmdrivers/broadcom/char/xtmcfg/中的字符设备，该字符设备被编译为bcmxtmcfg.ko，在modem启动时insmod到modem中。本文从bcmxtmrt_request开始分析xtm的配置。
在bcmxtmrt_request加入调试语句，可以获取在modem运行期间对modem做的所有配置。
```c
int bcmxtmrt_request( XTMRT_HANDLE hDev, UINT32 ulCommand, void *pParm )
{
    PBCMXTMRT_DEV_CONTEXT pDevCtx = (PBCMXTMRT_DEV_CONTEXT) hDev;
    int nRet = 0;
printk("command=%d, pParam=%p\n", ulCommand, pParm);
hex_dump((unsigned char *)pParm, 64);
。。。
}
```

###xtmcfg中各个文件的功能说明
* xtmcfgdrv.c中，设置了字符设备bcmxtmcfg0，并设置ioctl函数，用于内核态与用户态交互。XTM驱动中调用DoInitialize函数初始化xtm的processor。
* xtmcfgmain.cpp，设置股BcmXtm_ChipInitialize初始化xtm的各个寄存器，并使用bcmxtmrt驱动的bcmxtmrt_request函数传递给processor的m_pfnXtmrtReq字段（该字段在processor、connection、oamhandler中都有该字段）。此外，为xtm字符设备提供接口函数。
* xtmprocessor.cpp，xtm设备的全局设置设置在此，如TrafficDescrTable、InterfaceCfg等。
* xtmconnection.cpp，在xtmprocessor的createNetworkDevice中，调用m_ConnTable.Get(pConnAddr)获取一个xtm connection，之后设置这个connection。WAN基础接口相关的在此文件中设置，如设置发包队列，配置shaper等在此完成。
* xtmoamhandler.cpp中，发送信元、接收oam信元等。
* xtmsoftsar.cpp中，6338可以使用软sar来组装信元。

###XTM初始化

XTM初始化时，接收的pParm为PXTMRT_GLOBAL_INIT_PARMS。该数据结构用户态对应的是PXTM_INITIALIZATION_PARMS。起始位置为xtm_mgr.c::xtm_handle_oss_start()

图1. XTM驱动初始化配置命令
```c 
typedef struct XtmrtGlobalInitParms
{
    UINT32 ulReceiveQueueSizes[MAX_RECEIVE_QUEUES];  
//该字段表示在PTM驱动收包端的DMA个数、skb buffer个数、skb data buffer的个数。  
都使用该字段。MAX_RECEIVE_QUEUES最多为4，我们使用了前面的两个。如上图所示，第一个队列大小为0x190，第二个队列为0x64。
 
    /* BCM6368 */
    UINT32 *pulMibTxOctetCountBase;
    UINT32 *pulMibRxMatch;
    UINT32 *pulMibRxOctetCount;
    UINT32 *pulMibRxPacketCount;
} XTMRT_GLOBAL_INIT_PARMS, *PXTMRT_GLOBAL_INIT_PARMS;
```
在接收到commad为1的命令后，bcmxtmrt_request会调用DoGlobInitReq()函数初始化XTM接口。它主要初始化XTM的收发包的DMA、skb的BD buffer等。
该函数较长，我们逐段分析之：

###为收包DMA分配内存
```
ulSize = (ulBufsToAlloc * sizeof(struct DmaDesc)) + 0x10;  // ulBufsToAlloc为ulReceiveQueue-Sizes数组各项之和。这里加0x10方便对齐。
    if( nRet == 0 && (pGi->pRxBdMem = (struct DmaDesc *)
        kmalloc(ulSize, GFP_KERNEL)) != NULL )  // pGi->pRxBdMem为XTM收包BD的实际内存起始地址。
    {
        pBdBase = (struct DmaDesc *) (((UINT32) pGi->pRxBdMem + 0x0f) & ~0x0f);  //pBdBase的地址按照16字节对齐，它是DMA环的起始地址。
        p = (UINT8 *) pGi->pRxBdMem;
        cache_wbflush_len(p, ulSize);  //将cache的内容写到内存中，手动保证同步
        pBdBase = (struct DmaDesc *) CACHE_TO_NONCACHE(pBdBase);
    }
```

###为skb buffer和skb data分配内存。
这里用到两个宏：SKB_ALIGNED_SIZE，RXBUF_ALLOC_SIZE。前者表示一个skb所占用的内存空间，后者表示skb->data占用的空间。从这里可以看出，实际分配内存时， skb和skb->data在内存上的位置是相临的。
```c
           const UINT32 ulRxAllocSize = SKB_ALIGNED_SIZE + RXBUF_ALLOC_SIZE;
        const UINT32 ulBlockSize = (64 * 1024);   //以64k字节的块为单位申请内存，以提高申请内存的效率。
        const UINT32 ulBufsPerBlock = ulBlockSize / ulRxAllocSize;  //每一次申请内存所能放置的skb的个数。
//这里额外申请一个buffer，保证pGi->pFreeRxSkbList不为空。
        if( (p = kmalloc(SKB_ALIGNED_SIZE, GFP_KERNEL)) != NULL )
        {
            memset(p, 0x00, SKB_ALIGNED_SIZE);
            ((struct sk_buff *) p)->retfreeq_context = pGi->pFreeRxSkbList;  // retfreeq_context用来表示这个包被free时，应该将它归还到哪里，也就是skb的上下文。这样保证了WAN口收包，但是在LAN口驱动中发送完毕释放的时候，包仍然能够被正确地挂到WAN口的skb buffer下面。
            pGi->pFreeRxSkbList = (struct sk_buff *) p;  //新申请的包被挂到WAN口的skb的free队列下。
        }
        j = 0;
        pBd = pBdBase;   
        while( ulBufsToAlloc )
        {    //这里也就是按照64k的块申请内存
            ulAllocAmt = (ulBufsPerBlock < ulBufsToAlloc) ? ulBufsPerBlock : ulBufsToAlloc;
            ulSize = ulAllocAmt * ulRxAllocSize;
            if( j < MAX_BUFMEM_BLOCKS && (p = kmalloc(ulSize, GFP_KERNEL)) != NULL )
            {  // MAX_BUFMEM_BLOCKS=64，也就是说最多申请64个64k的块
                UINT8 *p2;
                memset(p, 0x00, ulSize);
                pGi->pBufMem[j++] = p;  //pBufMem，该数组记录申请到的内存块的地址
                cache_wbflush_len(p, ulSize);
                p = (UINT8 *) (((UINT32) p + 0x0f) & ~0x0f);
                for( i = 0; i < ulAllocAmt; i++ )
                {
对于收包BD，DMA有几个主要状态：
DMA_OWN表示该bd被驱动所拥有，是空闲的，可以被硬件写数据进去
DMA_DONE 表示该bd被DMA占有了，有数据待发送。
DMA_WRAP表示该已经到了BD环的结尾，需要wrap到头部。
                    pBd->status = DMA_OWN;  //初始化DMA。
                    pBd->length = RXBUF_SIZE;
                    pBd->address = (UINT32)VIRT_TO_PHY(p + RXBUF_HEAD_RESERVE);
                    pBd++;
//这里可以看出，skb->data在skb内存的前面
                    p2 = p + RXBUF_ALLOC_SIZE;
                    ((struct sk_buff *) p2)->retfreeq_context =
                        pGi->pFreeRxSkbList;
                    pGi->pFreeRxSkbList = (struct sk_buff *) p2;
                    p += ulRxAllocSize;
                }
                ulBufsToAlloc -= ulAllocAmt;
            }
//将不够64个块的其他pGi->pBufMem[j]指针清空。代码省略
        }
```
###初始化收包DMA的寄存器

       这里初始化收包和发包的配置寄存器。比如记录收发包的状态，是否有包处于待发送状态。我们在确认DMA HALT故障时，需要读取dumpmem 0xb0005240 64。这里的0xb0005240就是第一个发包DMA队列。
```
dumpmem 0xb0005240 64
b0005240: 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 08    ................
b0005250: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08    ................
b0005260: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08    ................
b0005270: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08    ................
```
上面红色标记的为0xb0005240的第一字节的00比特位，该位为1表示硬件有包待处理，而为0表示包处理完了。在DMA HALT故障时，这个寄存器一直为1，也就是无法发包了。

在实际PTM驱动中，pGi->pDmaRegs->chcfg转化为物理地址0xb0005200，对于收包DMA寄存器，SAR_RX_DMA_BASE_CHAN为0。发包SAR_TX_DMA_BASE_CHAN为4。
```c
    volatile DmaChannelCfg *pRxDma, *pTxDma;
        volatile DmaStateRam *pStRam;
        /* Clear State RAM for receive DMA Channels. */
        pGi->pDmaRegs = (DmaRegs *) SAR_DMA_BASE;
        pStRam = pGi->pDmaRegs->stram.s;
        memset((char *) &pStRam[SAR_RX_DMA_BASE_CHAN], 0x00,
            sizeof(DmaStateRam) * NR_SAR_RX_DMA_CHANS);  //清零所有寄存器
        pGi->pRxDmaBase = pGi->pDmaRegs->chcfg+SAR_RX_DMA_BASE_CHAN;
        pRxDma = pGi->pRxDmaBase;
        pBd = pBdBase;
        for(i=0, pRxDma=pGi->pRxDmaBase; i < MAX_RECEIVE_QUEUES; i++, pRxDma++)
        {
            pRxDma->cfg = 0;
            BcmHalInterruptDisable(SAR_RX_INT_ID_BASE + i);  //停用该队列的中断，不处理软件中断。
            if( pGip->ulReceiveQueueSizes[i] )
            {
                PRXBDINFO pRxBdInfo = &pGi->RxBdInfos[i];
                pRxDma->maxBurst = SAR_DMA_MAX_BURST_LENGTH;   //作用？
                pRxDma->intStat = DMA_DONE | DMA_NO_DESC | DMA_BUFF_DONE;
                pRxDma->intMask = DMA_BUFF_DONE;  //中断掩码。设置该寄存器可以屏蔽中断。
比如在WAN口处于收包处理函数中，来不及处理中断。就设置掩码位，使硬件不触发中断。
相关函数：bcmxtmrt_rxisr()。在这个bcmxtmrt_rxisr()函数中曾经有一个故障。
在收包过程中，先复位了intMask，硬件开始放中断，但是由于硬件和软件没有协调号，还没有打开软件中断，导致中断丢失，从而包没有被处理。
                pRxBdInfo->pBdBase = pBd;  //设置收包的BD环
                pRxBdInfo->pBdHead = pBd;  //当回收BD时，移动head指针
                pRxBdInfo->pBdTail = pBd;   //当收到新的包的时候，移动tail指针
                pRxBdInfo->pDma = pRxDma;  //将BD配置情况pRxDma赋值给该收包队列，该队列保存在pGi全局信息中。
                pStRam[SAR_RX_DMA_BASE_CHAN+i].baseDescPtr = (UINT32)                VIRT_TO_PHY(pRxBdInfo->pBdBase);
  //和上一句话的作用相反，将软件BD队列的地址赋给硬件寄存器。
                pBd += pGip->ulReceiveQueueSizes[i] - 1;
                pBd++->status |= DMA_WRAP;  //为该BD队列的最后一个加上DMA_WRAP标记，从而实现BD环。
                BcmHalMapInterrupt((FN_HANDLER) bcmxtmrt_rxisr, (UINT32)
                    pRxDma, SAR_RX_INT_ID_BASE + i);  //map中断号和中断处理函数。
            }
            else
                memset(&pGi->RxBdInfos[i], 0x00, sizeof(RXBDINFO)); 
 //如果在配置时分配给某一条队列的大小为0，那么清空pGi中记录的该BD队列的数组项。
        }
        pGi->pDmaRegs->controller_cfg |= DMA_MASTER_EN;
        /* Clear State RAM for transmit DMA Channels. */
        memset( (char *) &pStRam[SAR_TX_DMA_BASE_CHAN], 0x00,
            sizeof(DmaStateRam) * NR_SAR_TX_DMA_CHANS );
        pGi->pTxDmaBase = pGi->pDmaRegs->chcfg+SAR_TX_DMA_BASE_CHAN;
        for(i=0, pTxDma=pGi->pTxDmaBase; i < MAX_TRANSMIT_QUEUES; i++, pTxDma++)
        {
            pTxDma->cfg = 0;
            BcmHalInterruptDisable(SAR_TX_INT_ID_BASE + i);  //关掉发包的中断
        }
```
###初始化定时器，定时回收skb buffer

       这里回收的是PTM驱动的发包的buffer。

       启示在每次进入bcmxtmrt_xmit函数时候，都会回收一遍buffer。但是存在一个问题，如果一次放入一批包到buffer中，但之后就再也没有包过来，这个时候buffer就没有回收。所以需要增加一个定时器，定时执行回收操作。
```
        init_timer(&pGi->Timer);
        pGi->Timer.data = (unsigned long) pGi;
        pGi->Timer.function = (void *) bcmxtmrt_timer;
```

定时器函数。
```
static void bcmxtmrt_timer( PBCMXTMRT_GLOBAL_INFO pGi )
{
    UINT32 i;
    /* Free transmitted buffers. */
    for( i = 0; i < MAX_DEV_CTXS; i++ )
        if( pGi->pDevCtxs[i] )
            bcmxtmrt_xmit( NULL, pGi->pDevCtxs[i]->pDev );  //在xmit函数中回收。
    /* Restart the timer. */
    pGi->Timer.expires = jiffies + SAR_TIMEOUT;
    add_timer(&pGi->Timer);
} /* bcmxtmrt_timer */
```
##创建XTM接口

创建WAN接口也就是创建WAN连接的net_device接口，在Modem中这个接口名为ptm0。它接收从用户态的参数数据结构为：PXTMRT_CREATE_NETWORK_DEVICE。

内核函数bcmxtmrt_request()在收到command为2的命令后，开始调用DoCreateDeviceReq函数创建WAN接口。

##为接口分配内存
```c
    if( pGi->ulDrvState != XTMRT_UNINITIALIZED &&
        (dev = alloc_netdev( sizeof(BCMXTMRT_DEV_CONTEXT),
         pCnd->szNetworkDeviceName, ether_setup )) != NULL )
```
       待分配的net_device的内存大小由几部分组成：
* 私有数据结构体的大小。这里私有数据是BCMXTMRT_DEV_CONTEXT。为方便对齐，这里会在这个大小上加上32字节。
* struct net_device的大小。该数值会按32字节对齐。

一般地，使用kmalloc就可以满足内存分配的需要，该函数用于分配的连续的内存块，如果待分配的内存少于128k，一般都可以满足。但如果这里是用alloc_netdev分配的内存大小超过128k，则无法满足分配需求。一个方法是修改alloc_netdev的内存分配方式。

使用alloc_netdev分配的接口内存有两个特点：
* dev地址按32字节对齐
* dev->priv指向的私有数据内存块，起始地址也是按照32字节对齐。

```c
dev_alloc_name(dev, dev->name);
       为设备分配名称。传入参数的格式为ptm%d之类。在运行期间，会创建ptm0，ptm1之类的接口。这里涉及到linux设备管理。
设置设备的私有字段PBCMXTMRT_DEV_CONTEXT。
           pDevCtx = (PBCMXTMRT_DEV_CONTEXT) dev->priv;
        memset(pDevCtx, 0x00, sizeof(BCMXTMRT_DEV_CONTEXT));  //初始化私有数据
        memcpy(&pDevCtx->Addr, &pCnd->ConnAddr, sizeof(XTM_ADDR));  //设置设备的信元地址。对该地址的解释见后文。
/*931系列的Modem支持两种ATM和PTM两种信元。
ATM信元(ATM_ADDR)由32bit的ulPortMask，16bit的vpi，16bit的vci组成。在物理上modem只有一根用户线，但是使用vpi和vci可以在该物理通道上创建多条逻辑通道。在发包的时候dsl线会为包加上信元的头部。而在对端(DSLAM)也会设置某一根用户线支持的vpi和vci。Vpi和vci实现两个层次的逻辑通道，比如一个vpi通道下可以创建多个vci通道。（在配置Modem的一些功能时，比如QoS，需要注意vci和vpi的数量限制。）
PTM信元（PTM_ADDR）由32bit的portMask和32bit的ptmPriority组成。如图2所示，我们的portMask和ptmPriority都是1。
*/
        if( pCnd->ConnAddr.ulTrafficType == TRAFFIC_TYPE_ATM )  /*0x01000a*/
            pDevCtx->ulHdrType = pCnd->ulHeaderType;
        else
            pDevCtx->ulHdrType = HT_PTM;
        pDevCtx->ulFlags = pCnd->ulFlags;  /*0x07*/
        pDevCtx->pDev = dev;
        pDevCtx->ulAdminStatus = ADMSTS_UP;  /* 管理状态，默认为up。可以通过XTMRT_CMD_SET_ADMIN_STATUS设置。如果该字段没有被设置为UP，那么运行期间调用dev->open将不能打开一个接口。*/
        pDevCtx->ucTxVcid = INVALID_VCID;  /*该字段在link up时设置。在查询接口的统计数据时，会检查该字段是否设置。在发送一个包之前，也会使用该字段的值设置BD。pBd->status  = (pBd->status&DMA_WRAP) | DMA_SOP | DMA_EOP | pDevCtx->ucTxVcid;*/

             /*赋MAC地址*/
        /*设置回调函数。 这些函数会在创建设备的一个接口时调用*/
        dev->open               = bcmxtmrt_open;
        dev->stop               = bcmxtmrt_close;
        dev->hard_start_xmit    = bcmxtmrt_xmit;
        dev->tx_timeout         = bcmxtmrt_timeout;
        dev->watchdog_timeo     = SAR_TIMEOUT;
        dev->get_stats          = bcmxtmrt_query;
        dev->set_multicast_list = NULL;
        dev->do_ioctl           = &bcmxtmrt_ioctl;
        dev->poll               = bcmxtmrt_poll;
        dev->weight             = 96;
        dev->priv_flags |= IFF_WANDEV;  /*使用priv_flags字段约定该设备是一个WAN设备*/
        switch( pDevCtx->ulHdrType )
        {    。。。
            pDevCtx->ulEncapType = TYPE_ETH;    /* bridge, MER, PPPoE, PTM 该字段目前只在加速器中使用。*/
            dev->flags = IFF_MULTICAST;
              ｝
```
2.5             向linux注册设备。
```
       nRet = register_netdev(dev); /*向linux内核注册该网络接口。比如设置通知链，设置sysfs，在设备列表上挂接设备等。*/
                  /*将pDevCtx设置给pGi保存。*/
```

###设置发包队列

在XTM初始化时，设置了XTM驱动的收包BD环，但却将发包BD环的设置留到了建链后。这是因为发包的BD环要相对复杂一些。为了实现高优先级的包优先发送，为xtm驱动设置多条发包队列。高优先级队列的包优先发送，之后才会发送低优先级的包，也就是QoS。优先级队列可以在运行期间由用户态配置下来。

和前面的配置类似，都是通过字符设备bcmxtmcfg来配置xtm驱动，对应的函数为xtm.c：：devCtl_xtmSetConnCfg()。配置优先级队列的命令为XTMRT_CMD_UNSET_TX_QUEUE和XTMRT_CMD_SET_TX_QUEUE。如图3所示。

       函数的调用关系为：
```c
//用户态
devCtl_xtmSetConnCfg()
  -> ioctl( g_nXtmFd, XTMCFGIOCTL_SET_CONN_CFG, &Arg );
//内核态
bcmxtmcfg_ioctl()
  -> DoSetConnCfg ()
-> BcmXtm_SetConnCfg ()
  -> g_pXtmProcessor->SetConnCfg( pConnAddr, pConnCfg )  //首先配置连接的拥塞策略
    -> pConn->SetCfg( pConnCfg );  //设置连接
      -> CheckTransmitQueues  //设置发送队列。先删掉现有的DMA队列，然后再根据ulTransmitQParmsSize创建新的DMA队列，会多次调用到下面的函数。
           -> (*m_pfnXtmrtReq) (m_hDev, XTMRT_CMD_SET_TX_QUEUE, //bcmxtmrt_request
                    &m_TxQIds[m_ulTxQIdsSize - 1]);
```

数据结构
```c
typedef struct XtmrtTransmitQueueId
{
    UINT32 ulPortId;       //通常为1，最大为4。
    UINT32 ulPtmPriority;  //通常为1。如果是PTM驱动，该值有意义，可以有PRI和LOW两个优先级
    UINT32 ulSubPriority;  //队列优先级。可以是1,2,3。由用户态配置下来。对应DMA优先级
    UINT32 ulQueueSize;   //队列的大小。
    UINT32 ulQueueIndex;  //对应的dmaIndex。
} XTMRT_TRANSMIT_QUEUE_ID, *PXTMRT_TRANSMIT_QUEUE_ID;
```
 
##设置发包队列
```
/*为队列分配内存，同时初始化一个发送队列*/
static int DoSetTxQueue( PBCMXTMRT_DEV_CONTEXT pDevCtx,
    PXTMRT_TRANSMIT_QUEUE_ID pTxQId )
{
   UINT8 *p;
    PTXQINFO pTxQInfo =  &pDevCtx->TxQInfos[pDevCtx->ulTxQInfosSize++];
    PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
    /* 设置每条发送队列的大小都等于ulNumExtBufs ，至于具体有多少个包在排队，需要由xmit函数中的QueuePacket决定。ulNumExtBufs数值也等于LAN口收包驱动的大小需要分配DMA和skbuff的内存大小。*/
    ulQueueSize = pGi->ulNumExtBufs;
    ulSize = (ulQueueSize * (sizeof(struct sk_buff *) +
        sizeof(struct DmaDesc))) + 0x20; /* 0x20 for alignment */
    if( (pTxQInfo->pMemBuf = kmalloc(ulSize, GFP_ATOMIC)) != NULL )
    {
        memset(pTxQInfo->pMemBuf, 0x00, ulSize);
        cache_wbflush_len(pTxQInfo->pMemBuf, ulSize);
 
        ulPort = PORTID_TO_PORT(pTxQId->ulPortId);
        if( ulPort < MAX_PHY_PORTS &&
            pTxQId->ulSubPriority < MAX_SUB_PRIORITIES )
        {
           /*如果传递先来的物理portid，以及优先级都是合法的，那么就设置pTxQInfo的队列相关信息，设置DMA环和skbuff环。设置队列的DMA配置，DMA的maxburst，DMA的中断状态，DMA状态数据(比如收发包的个数)，将BD地址告诉硬件。所有设置完成后，将该队列放入到全局变量pGi中，并设置pGi的ulNumTxQs。具体代码省略*/
              }
}
```
###清空发包队列
```
/*清空一个队列的内存*/
static int DoUnsetTxQueue( PBCMXTMRT_DEV_CONTEXT pDevCtx,
PXTMRT_TRANSMIT_QUEUE_ID pTxQId )
｛/*从设备的全局数据结构中，找到ulQueueIndex对应的队列*/
    for( i = 0, pTxQInfo = pDevCtx->TxQInfos; i < pDevCtx->ulTxQInfosSize;
        i++, pTxQInfo++ )
    {
       if( pTxQId->ulQueueIndex == pTxQInfo->ulDmaIndex )
        {
                     /*清空寄存器中的状态信息*/
                     /*free队列中供DMA BD和skbuff BD使用的内存*/
                     /*设置pTxPriorities为NULL，也就是说发包函数不能在使用这条队列了*/
              /*修改pDevCtx中的队列数组和队列大小*/
              /*更新pGi->ulNumTxQs */
              ｝
       }
}
```

###DSL链路状态改变

       DSL链路改变，是指插拔用户线后的建链与掉链。XTMRT_CMD_LINK_STATUS_CHANGED处理LinkUp和LinkDown。
```c
typedef struct XtmrtLinkStatusChange
{
    UINT32 ulLinkState;       /*链路状态。该字段的最高bit位用于设置pDevCtx->ulFlags */
    UINT32 ulLinkUsRate;     /*上行建链速率*/
    UINT32 ulLinkDsRate;     /*下行建链速率*/
    UINT32 ulTransmitQueueIdsSize;    /*实际发包队列的个数*/
    XTMRT_TRANSMIT_QUEUE_ID TransitQueueIds[MAX_TRANSMIT_QUEUES]; /*保存实际发包队列的相关参数。用于连接建立后设置发包驱动*/
    UINT8 ucTxVcid;          /*用于记录发包统计数据在pGi->pulMibTxOctetCountBase 中的位置。参见bcmxtmrt_query()。当链路Down下来后，该数值设置为INVALID_VCID */
    UINT32 ulRxVcidsSize;      /*rxvcid数组大小。。用于在pGi->pDevCtxsByMatchId 中通过vcid查找设备上下文数组。*/
    UINT8 ucRxVcids[MAX_VCIDS]; 
} XTMRT_LINK_STATUS_CHANGE, *PXTMRT_LINK_STATUS_CHANGE;
```

####命令处理

由于XTMRT_CMD_LINK_STATUS_CHANGED需要同时处理建链和掉链时的Device Context管理，因此在这个函数中对它
```c
static int DoLinkStsChangedReq( PBCMXTMRT_DEV_CONTEXT pDevCtx,
     PXTMRT_LINK_STATUS_CHANGE pLsc )
{
    local_bh_disable();
    if( pDevCtx )    {
    PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
    for( i = 0; i < MAX_DEV_CTXS; i++ )
        {
        if( pGi->pDevCtxs[i] == pDevCtx )
        {
            pDevCtx->ulFlags |= pLsc->ulLinkState & LSC_RAW_ENET_MODE;  /*最高bit位作它用*/
            pLsc->ulLinkState &= ~LSC_RAW_ENET_MODE;
            pDevCtx->MibInfo.ulIfLastChange = (jiffies * 100) / HZ;
            pDevCtx->MibInfo.ulIfSpeed = pLsc->ulLinkUsRate;
                if( pLsc->ulLinkState == LINK_UP )
                    nRet = DoLinkUp( pDevCtx, pLsc );
                else
                    nRet = DoLinkDownTx( pDevCtx );
                break;
            }        }    }
    else
    {/*在所有的pDevCtx都down掉后，有xtmconnection调用DoLinkStsChangedReq 执行该步骤，处理一些全局的Linkdown事件。比如关收包中断*/
        nRet = DoLinkDownRx( (UINT32) pLsc );
    }
    local_bh_enable();
    return( nRet );
}
```
####Link Up

       以LinkUp而言，主要设置PTM设备的发包队列，以及准备收包中断。

       关键代码：
```c
    for( i = 0; i < pLsc->ulRxVcidsSize; i++ )   /*通过rxvcid查找pDevCtx*/
        pGi->pDevCtxsByMatchId[pLsc->ucRxVcids[i]] = pDevCtx;
/*设置发包队列，在第3小节已经描述过该函数。
如果发包队列设置成功，则做一些收包的准备工作。*/
    for(i = 0,pTxQInfo = pDevCtx->TxQInfos, pTxQId = pLsc->TransitQueueIds;
        i < pLsc->ulTransmitQueueIdsSize && nRet == 0; i++, pTxQInfo++, pTxQId++)
    {        nRet = DoSetTxQueue(pDevCtx, pTxQId );    }
       下面的代码用于准备收包，需要特别关注各个准备工作的先后顺序。曾经有个故障与这里的先后顺序不当有关，在此介绍。
       案例：大流量(100M)下行发包时，拔掉用户线再插上，下行业务不通，Ptm驱动不收包。
       案例分析：当DSL 重新插上后，DMA打开，由于有大流量，所以收包BD 环很快被灌满，并且有中断，但是ifconfig ptm0 up 滞后，这个时候虽然进入了中断，但是dev 的一些标志位没置上，不符合收包条件，中断退出，然后ifconfig ptm0 up ，这个时候中断可以被正常处理了，但是BD环已经满了，DMA往里放不进去包了，再也没中断产生了。形成了一个局面：DMA 放不进去包，没有中断，而收包任务因为没有中断也没法去收包清空BD环，陷入了死锁。
broadcom没有这个问题，原因是应用程序和我们不一样，他的DMA打开和ptm0 up 能收包之间的时间间隔可能很短，所以没这个问题。
我们想了一个办法，将ptm0 up 的位置提到DMA打开之前，现在没有问题了。
        if( pGi->ulDrvState == XTMRT_INITIALIZED )
        {
         if( pDevCtx->ulAdminStatus == ADMSTS_UP )
         {
dev_open(pDevCtx->pDev) ;  //打开dev，设置它的标志位，使设备满足收包条件。该函数会调用一些通用的dev设置操作，并且调用dev->open()函数。也就是bcmxtmrt_open函数中，设置设备状态为XTMRT_DEV_OPENED，以及netif_start_queue()。事实上，open()中的这几个步骤和后面的netif_start_queue()有所重叠。
}
            for( i = 0, pRxDma = pGi->pRxDmaBase; i < MAX_RECEIVE_QUEUES;
                i++, pRxDma++ )
            {
                if( pGi->RxBdInfos[i].pBdBase )
                {
                                 BcmHalInterruptEnable(SAR_RX_INT_ID_BASE + i);  //开中断，中断处理器做好接收中断的准备
                     pRxDma->cfg = DMA_ENABLE;  //打开硬件中断标志位。之后DMA触发中断。此时中断处理器已经准备好，接收中断。然后包收取后交给dev，dev可以处理包。按照此流程，包不会被丢失。
                }
            }
            pGi->Timer.expires = jiffies + SAR_TIMEOUT;
            add_timer(&pGi->Timer);  //设置定时器，用于定时回收发包BD。一般地，BD回收是在发包之前进行，BD都能够回收。定时器主要应对这种情况：大流量发包之后突然停止，不再发送包，此时BD就会被长期占用了。
            if( pDevCtx->ulOpenState == XTMRT_DEV_OPENED )
                netif_start_queue(pDevCtx->pDev);  //netif_start_queue 用来告诉上层网络协定这个驱动程序还有空的缓冲区可用，请把下一个封包送进来
            pGi->ulDrvState = XTMRT_RUNNING;
              }
```
####LinkDown

       掉链时需要停用发包DMA、清空BD环、关闭收包中断等操作。可以分作两个部分：doLinkDownTx和doLinkDownRx两个过程。
1） doLinkDownTx
       案例：带流量的情况下整板插拔用户线，会出现有的ptm0口发包计数器显示包发送出去，但是在对端却收不到包的情况。经过分析发包寄存器，发现PTM发包DMA长期处于DMA_ENABLE状态，导致不能发包。
       下面的这段代码用于检查DMA是否HALT。
```c
          for( j = 0; j < 2000 &&
             (pTxQInfo->pDma->cfg & DMA_ENABLE) == DMA_ENABLE; j++ )
        {
            udelay(500);
        }
        if( (pTxQInfo->pDma->cfg & DMA_ENABLE) == DMA_ENABLE )
        {
            /* This should not happen. */
            printk("**** DMA_PKT_HALT ****\n");
            pTxQInfo->pDma->cfg = DMA_PKT_HALT;
            udelay(500);
            pTxQInfo->pDma->cfg = 0;
//           kerSysMipsSoftReset();        //检测到DMA HALT后可以重启modem
        }
       下面的代码用于设置ptm设备xmit端。
              //清空一些统计数据
        ulIdx = SAR_TX_DMA_BASE_CHAN + pTxQInfo->ulDmaIndex;
        pStRam[ulIdx].baseDescPtr = 0;
        pStRam[ulIdx].state_data = 0;
        pStRam[ulIdx].desc_len_status = 0;
        pStRam[ulIdx].desc_base_bufptr = 0;
        /* 清空发送BD环。这里需要将bd中剩余的包全部释放掉。 */
        for( j = 0; j < pTxQInfo->ulQueueSize; j++ )
            if( pTxQInfo->ppSkbs[j] && pTxQInfo->pBds[j].address )
                dev_kfree_skb_any(pTxQInfo->ppSkbs[j]);
        /* 清空ptm的xmit队列。和DoUnsetTxQueue的作用相当*/
        pTxQInfo->ulFreeBds = pTxQInfo->ulQueueSize = 0;
        pTxQInfo->ulNumTxBufsQdOne = 0;
        if( pTxQInfo->pMemBuf )
        {
            kfree(pTxQInfo->pMemBuf);
            pTxQInfo->pMemBuf = NULL;
        }
    /* 清空发包相关的一些数据结构*/
    pDevCtx->ulTxQInfosSize = 0;
    memset(pDevCtx->TxQInfos, 0x00, sizeof(pDevCtx->TxQInfos));
    memset(pDevCtx->pTxPriorities, 0x00, sizeof(pDevCtx->pTxPriorities));
    pDevCtx->ucTxVcid = INVALID_VCID;
    pGi->ulNumTxBufsQdAll = 0;
2） doLinkDownRx
       当所有的pDevCtx都关闭后，关闭收包中断。
       if( ulStopRunning )   // ulStopRunning是否有pDevCtx仍然在运行。
    {
        /* Disable receive interrupts and stop the timer. */
        for( i = 0, pRxDma = pGi->pRxDmaBase; i < MAX_RECEIVE_QUEUES;
             i++, pRxDma++ )
        {
            if( pGi->RxBdInfos[i].pBdBase ) 
            {
                pRxDma->cfg = 0;  //DMA停止发包
                BcmHalInterruptDisable(SAR_RX_INT_ID_BASE + i); //中断管理器停止接收中断
            }
        }
        del_timer_sync(&pGi->Timer);  //删除发包BD回收定时器
        pGi->ulDrvState = XTMRT_INITIALIZED;  //设置ptm的状态为XTMRT_INITIALIZED，掉链收包设置过程结束。
```
##QoS配置过程
```c
DoSetTrafficDescrTable
       --》BcmXtm_SetTrafficDescrTable
              --》SetTrafficDescrTable
                     --》pConn->SetTdt
                            --》ConfigureShaper  配置单条队列
                                   --》设置寄存器XtmProcessorRegisters-> ulSstCtrl
``` 
 
H168N驱动串口打印
```
3814 XTM Init: Ch:0 - 200 rx BDs at 0xb0a229d0
3816 XTM Init: Ch:1 - 16 rx BDs at 0xb0a23010
4507 XTM Init: Ch:0 - 400 tx BDs at 0xa27ea000
4508 XTM Init: Ch:1 - 400 tx BDs at 0xa274a000
4509 XTM Init: Ch:2 - 400 tx BDs at 0xa26fa000
```
##Enet/Switch驱动

FAP

如下图所示。63168使用一块4ke的MIPS芯片，作为FAP芯片，用于快速转发包。4ke芯片和host芯片之间靠DQM交互。DQM是一些FIFO的队列，方向可配置，可以收发包。在没有FAP的时候，iuDMA直接触发中断将包给HOST芯片。现在iuDMA将包先给FAP，由FAP使用DQM将包推送到HOST。发包的过程与此类似。因此现在不仅仅需要给驱动配置DMA队列，还需要给他们配置DQM队列。
下图中有三条数据包路径：
1. 中断收包进入协议栈，之后包再设备接口出去。
1. 走软加速，学习加速条目，并快速转发，没有条目数量限制。软加速应该把加速条目配置到FAP中。
1. 走FAP，直接由外置的4ke芯片转发数据包。


设置BD大小
```
-------- Packet DMA RxBDs --------Number of RX DMA Channels
ETH[0] # of RxBds=0   bcmPktDma_Bds_p->host.eth_rxbds[chnl]
ETH[1] # of RxBds=0 g_Eth_rx_iudma_ownership[chnl]为HOST_OWN时才有值。此时却分别为FAP_OWN, FAP_OWN1
ETH[0] Rx DQM depth=600   DQM(Dynamic Queue Management)
ETH[1] Rx DQM depth=600
XTM[0] # of RxBds=16
XTM[1] # of RxBds=16
XTM[0] Rx DQM depth=128
XTM[1] Rx DQM depth=16
FAP ETH[0] # of RxBds=600
FAP ETH[1] # of RxBds=600
FAP XTM[0] # of RxBds=200
FAP XTM[1] # of RxBds=16
Total # RxBds=1448
 
-------- Packet DMA TxBDs ---------
ETH[0] # of TxBds =0
ETH[1] # of TxBds =0
ETH[0] Tx DQM depth=100
ETH[1] Tx DQM depth=100
XTM[0] # of TxBds =0
XTM[1] # of TxBds =0
… …
XTM[15] # of TxBds =0
XTM[0] Tx DQM depth=50
XTM[1] Tx DQM depth=50
… …
XTM[15] Tx DQM depth=50
FAP ETH[0] # of TxBds=300
FAP ETH[1] # of TxBds=300
FAP XTM[0] # of TxBds=600   FAP_XTM_NR_TXBDS
FAP XTM[1] # of TxBds=600
… …
FAP XTM[15] # of TxBds=600
DoGlobInitReq[2484], nr_tx_bds=600
fapDrv_psmAlloc: out of memory: fapIdx=1, size=1600, ManagedMemory=b0a208e8, totalSize=11008
ERROR: Out of PSM. Xtm rx BD allocation rejected!!
Unable to allocate memory for Rx Descriptors
```
