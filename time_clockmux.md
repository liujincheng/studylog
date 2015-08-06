摘要：  
MCU上各个器件需要选择合适的clock source。

---

相关的文件：  
mt_clkmgr_legacy.c

#数据结构
以audio audio_clkmux为例，sel_mask和pdn_mask控制clock的flag位。
```c
{
        .name = __stringify(MUX_AUDIO),//19
//        .base_addr = CLK_CFG_4,
        .sel_mask = 0x00000003,    #
        .pdn_mask = 0x00000080,    #mask
        .offset = 0, 
        .nr_inputs = 4, 
        .ops = &audio_clkmux_ops,
}
```

#选择clk source
audio change  clk’s  source from clkmux_sel。clksrc为0和1之类的数值，代表选择哪个clock source。比如0，select 26M。可以参考mt_soc_audio_8163/mt_soc_pcm_voice_md2_bt.c文件中调用clkmux_sel的位置。
```
clkmux_sel(id, clocksrc, name)
    +--> clkmux_sel_locked(mux, clksrc);
        +--> mux->ops->sel(mux, clksrc); 针对audio，为clkmux_sel_op，
            +--> clk_writel(mux->base_addr+8, mux->sel_mask); //clr
            +--> clk_writel(mux->base_addr+4, (clksrc << mux->offset)); //set
            +--> aee_rr_rec_clk(4, clk_readl(mux->base_addr));  //第一个参数，为mux_id/4，每个字节为一个mux
```

enable clk
```c
clkmux_enable_op(mux)
    +--> clk_writel(mux->base_addr+8, mux->pdn_mask);//write clr reg
    +--> aee_rr_rec_clk(4, clk_readl(mux->base_addr));  //第一个参数，为mux_id/4
```

会将clk_data的值记录到RAM_CONSOLE中，当抓到dbg文件时，可以从中读到相应mux的clk_data。比如下面的实例。
```
clk_data: 0x1000101
clk_data: 0x81018180
clk_data: 0x1810100
clk_data: 0x7028202
clk_data: 0x1008180
clk_data: 0x10101
clk_data: 0x1018080
clk_data: 0x0
```
