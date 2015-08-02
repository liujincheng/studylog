实现了可以动态调整大小的数组

类似于数组，但使用数组之前需要数组的大小，且无法动态调整。虽然可以申请一块内存，强制转化为数组，但仍然面临如何保证动态调整大小的问题。
为解决这个问题，引入IDR数据结构。
```c
/*初始化*/
static DEFINE_IDR(mtd_idr);
#define DEFINE_IDR(name)struct idr name = IDR_INIT(name)
#define IDR_INIT(name) \
{ \
 .top = NULL, \
 .id_free= NULL, \
 .layers = 0, \
 .id_free_cnt= 0, \
 .lock = __SPIN_LOCK_UNLOCKED(name.lock),\
}
/*遍历*/
#define mtd_for_each_device(mtd) \
 for ((mtd) = __mtd_next_device(0); \
      (mtd) != NULL; \
      (mtd) = __mtd_next_device(mtd->index + 1))
/*读取*/
struct mtd_info *__mtd_next_device(int i)
{
 return idr_get_next(&mtd_idr, &i);
}
ret = idr_find(&mtd_idr, num);
/*添加*/
 do {
  if (!idr_pre_get(&mtd_idr, GFP_KERNEL))
   goto fail_locked;
  error = idr_get_new(&mtd_idr, mtd, &i);
 } while (error == -EAGAIN);
/*删除*/
idr_remove(&mtd_idr, mtd->index);
```
