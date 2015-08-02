在I2C总线传输过程中，将两种特定的情况定义为开始和停止条件（见图3）：当SCL保持“高”时，SDA由“高”变为“低”为开始条件；当SCL保持“高”且SDA由“低”变为“高”时为停止条件。开始和停止条件均由主控制器产生。使用硬件接口可以很容易地检测到开始和停止条件，没有这种接口的微机必须以每时钟周期至少两次对SDA取样，以检测这种变化。

开始和停止条件图3



SDA线上的数据在时钟“高”期间必须是稳定的，只有当SCL线上的时钟信号为低时，数据线上的“高”或“低”状态才可以改变。输出到SDA线上的每个字节必须是8位，每次传输的字节不受限制，但每个字节必须要有一个应答ACK。如果一接收器件在完成其他功能（如一内部中断）前不能接收另一数据的完整字节时，它可以保持时钟线SCL为低，以促使发送器进入等待状态；当接收器准备好接受数据的其它字节并释放时钟SCL后，数据传输继续进行。I2C数据总线传送时序如图4所示。

数据传送具有应答是必须的。与应答对应的时钟脉冲由主控制器产生，发送器在应答期间必须下拉SDA线。当寻址的被控器件不能应答时，数据保持为高并使主控器产生停止条件而终止传输。在传输的过程中，在用到主控接收器的情况下，主控接收器必须发出一数据结束信号给被控发送器，从而使被控发送器释放数据线，以允许主控器产生停止条件。合法的数据传输格式如下：

I2C总线在开始条件后的首字节决定哪个被控器将被主控器选择，例外的是“通用访问”地址，它可以在所有期间寻址。当主控器输出一地址时，系统中的每一器件都将开始条件后的前7位地址和自己的地址进行比较。如果相同，该器件即认为自己被主控器寻址，而作为被控接收器或被控发送器则取决于R/W位。


Linux驱动声明了一些宏
MODULE_DEVICE_TABLE(i2c, gpon_i2c_id_table);
#define MODULE_DEVICE_TABLE(type,name) \
  MODULE_GENERIC_TABLE(type##_device,name)
#define MODULE_GENERIC_TABLE(gtype,name) \
extern const struct gtype##_id __mod_##gtype##_table \
  __attribute__ ((unused, alias(__stringify(name))))
展开后，得到如下声明，用于导出变量到符号表中。
extern const i2c_device_id __mod_i2c_device_gpon_i2c_id_table __attribute__ ((unused, alias(__stringify(name))))

i2c_new_device() or i2c_new_probed_device()

两个概念：
i2c_adapter
i2c_adapter is the structure used to identify a physical i2c bus along with the access algorithms necessary to access it.
i2c_algorithm为一类i2c设备实现了如何发送数据的接口，而i2c_adapter则是用来标记这一类i2c设备。

i2c_driver
按照Linux设备模型定义的i2c设备驱动，包含probe、remove、shudown、suspend等函数接口。同时又按照i2c的特性，包含id_table、address_list、attach_adapter等字段。
i2c设备支持自动检测。如果detect这个字段没有定义，那么并不能称之为一个真正意义的i2c设备，它只能处理i2c_smbus_read_byte_data，其他的接口都无法使用。
struct i2c_driver {
 unsigned int class; i2c设备类型
 int (*attach_adapter)(struct i2c_adapter *) __deprecated;
 int (*detach_adapter)(struct i2c_adapter *) __deprecated;

 linux设备驱动接口
 int (*probe)(struct i2c_client *, const struct i2c_device_id *);
 int (*remove)(struct i2c_client *);
 void (*shutdown)(struct i2c_client *);
 int (*suspend)(struct i2c_client *, pm_message_t mesg);
 int (*resume)(struct i2c_client *);

 void (*alert)(struct i2c_client *, unsigned int data); alert接口
 int (*command)(struct i2c_client *client, unsigned int cmd, void *arg); 类ioctl接口，供用户配置设备

 struct device_driver driver; 内嵌通用driver结构体，供驱动模型使用，如sysfs中显示
 const struct i2c_device_id *id_table; 供驱动模型使用

 int (*detect)(struct i2c_client *, struct i2c_board_info *); detect回调，自动创建设备
 const unsigned short *address_list; 驱动支持的地址列表，估计用于自动检测
 struct list_head clients; 客户链表
};
#define to_i2c_driver(d) container_of(d, struct i2c_driver, driver)


新的client发现流程
i2c_detect(struct i2c_adapter *adapter, struct i2c_driver *driver)
    |-->temp_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL); 初始化一个临时client，用于判断是否有client
    |-->for (i = 0; address_list[i] != I2C_CLIENT_END; i += 1) for循环，遍历driver->address_list，检查是否有设备在线
        |-->i2c_detect_address(temp_client, driver);
            |-->i2c_check_addr_validity(addr); 检查地址是否可用，小于0x8，大于0x77都是保留地址
            |-->i2c_check_addr_busy(adapter, addr) 检查地址下是否有client已经存在。首先检查parent上该地址是否有client存在，再检查adapter下设备是否有client存在
            |-->i2c_default_probe(adapter, addr) 通过调用i2c_smbus_xfer向指定地址上读或写数据，通过返回值判断是否地址上是否有设备存在
            |-->driver->detect(temp_client, &info) 调用驱动的detect函数。temp_client是临时的，而info是入参供驱动detect函数填写参数。
            |-->client = i2c_new_device(adapter, &info) 创建一个真实的i2c_client
            |-->list_add_tail(&client->detected, &driver->clients) 将client添加到驱动的设备链表下。

i2c_register_driver(struct module *owner, struct i2c_driver *driver)
    |-->driver_register(&driver->driver) 向通用设备驱动模型注册
    |-->i2c_for_each_dev(driver, __process_new_driver); 调用i2c_bus_type的sysfs下所有adapter，处理driver。
        |-->bus_for_each_dev(&i2c_bus_type, NULL, data, fn);
            |-->klist_iter_init_node(&bus->p->klist_devices, &i,(start ? &start->p->knode_bus : NULL));
            |-->while ((dev = next_device(&i)) && !error)
                |-->fn(dev, data)。也即__process_new_driver(dev,driver)
                    |-->i2c_do_add_adapter(data, to_i2c_adapter(dev)) 添加设备。可以想到，adapter注册到bus，而遍历bus，也可以找到所有的adapter。
                        |-->i2c_detect(adap, driver); 向adapter添加driver
