#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*for 64bit device*/
typedef unsigned long __uint64_t;
typedef __uint64_t    uint64_t;
typedef __uint32_t    uint32_t;
typedef unsigned int __uint32_t;

void main(int argc, char* argv[])
{
  FILE* fp ;
  int cpu = 0 ;
  int offset = 0 ;
  char buff[8] ;
  unsigned long base ;

  if(argc != 3)
  {
    printf("%s <base addr> <file>\n", argv[0]) ;
    exit(-1) ;
  }

  base = strtoul(argv[1], NULL, 16) ;

  fp = fopen(argv[2], "rb") ;
  if(!fp)
  {
    printf("open file errorn \n") ;
    exit(-1) ;
  }

  while(fread(&buff, 8, 1, fp) > 0)
  {
	printf("0x%016lx: 0x%016lx\n", base + offset, *((unsigned long*)buff)) ; 
	offset +=8 ;
  }
  fclose(fp) ;
  
}


