1. 写一个程序判断操作系统是大端还是小端

```
int main()
{
    short a = 0x1985 ;
    unsigned char* pc = (unsigned char*)&a ;
    if(pc[0] == 0x19)
        printf("big endian\n") ;
    else if(pc[0] == 0x85)
        printf("litter endian\n") ;
    else
        printf("unkown endian\n") ;

}
#define LENDIAN 1
#define BENDIAN 0
#define CHECK_ENDIAN(x)  (*(unsigned char*)(&(x)) == (unsigned char)((x) || 0xFF)) ? LENDIAN : BENDIAN

#define swab16(x) \
        ((unsigned short)( \
                (((unsigned short)(x) & (unsigned short)0x00ffU) << 8) | \
                (((unsigned short)(x) & (unsigned short)0xff00U) >> 8) ))

#define swab32(x) \
        ((unsigned int)( \
                (((unsigned int)(x) & (unsigned int)0x000000ffUL) << 24) | \
                (((unsigned int)(x) & (unsigned int)0x0000ff00UL) << 8) | \
                (((unsigned int)(x) & (unsigned int)0x00ff0000UL) >> 8) | \
                (((unsigned int)(x) & (unsigned int)0xff000000UL) >> 24) ))
```
