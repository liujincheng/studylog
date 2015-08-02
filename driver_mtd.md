static struct class mtd_class = {
 .name = "mtd",
 .owner = THIS_MODULE,
 .suspend = mtd_cls_suspend,
 .resume = mtd_cls_resume,
};

idr Small id to pointer translation service avoiding fixed sized tables.

一个实际的mtd设备注册流程
brcmnanddrv_probe(struct platform_device *pdev)
    |-->brcmnanddrv_setup_mtd_partitions(info) info存有mtd_info, brcmnand等信息，据此根据nvram填入bcm63XX_nand_parts各个分区的信息。
    |-->mtd_device_register(&info->mtd, info->parts, info->nr_parts) 注册mtd设备。info->mtd为master设备，下面可以有多个分区
        |-->mtd_device_parse_register(master, NULL, NULL, parts, nr_parts) parts为分区信息指针，nr_parts为分区个数
            |-->parse_mtd_partitions(mtd, types, &real_parts, parser_data); 申请real_parts数组，保存name、size、offset、mask_flags(如读写控制位)、ecclayout等
            |-->add_mtd_partitions(mtd, real_parts, err) 一个mtd设备有多个分区的情况，每个分区都是一个设备。
                |-->for (i = 0; i < nbparts; i++)
                    |-->list_add(&slave->list, &mtd_partitions);slave为mtd_part，存有分区信息。将分区添加到全局链表mtd_partitions中。
                    |-->add_mtd_device(&slave->mtd); 添加mtd设备
            |-->add_mtd_device(mtd); mtd设备没有分区，初始化mtd设备
                |-->idr_get_new(&mtd_idr, mtd, &i) 从mtd_idr数组中新申请一个，可通过idr_get_next(&mtd_idr, &i)遍历所有mtd设备
                |-->初始化erasesize_shift、writesize_shift、erasesize_mask、writesize_mask、mtd->dev.type、mtd->dev.class等
                |-->device_register(&mtd->dev) 向驱动模型中添加该设备
    |-->dev_set_drvdata(&pdev->dev, info); 初始化设备的私有数据
