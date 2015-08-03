#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*for 64bit device*/
typedef unsigned long __uint64_t;
typedef __uint64_t    uint64_t;
typedef __uint32_t    uint32_t;
typedef unsigned int __uint32_t;

struct msm_rtb_layout {
  unsigned char sentinel[11];
  unsigned char log_type;
  uint32_t idx;
  uint64_t caller;
  uint64_t data;
} __attribute__ ((__packed__));

enum logk_event_type {
  LOGK_NONE = 0,
  LOGK_READL = 1,
  LOGK_WRITEL = 2,
  LOGK_LOGBUF = 3,
  LOGK_HOTPLUG = 4,
  LOGK_CTXID = 5,
  LOGK_TIMESTAMP = 6,
  LOGK_L2CPREAD = 7,
  LOGK_L2CPWRITE = 8,
  /* HTC DEFINE: START FROM 10 */
  LOGK_IRQ = 10,
  LOGK_DIE = 11,
};

char *type2string[] = {
  "NONE",
  "READL",
  "WRITEL",
  "LOGBUF",
  "HOTPLUG",
  "CTXID",
  "TIMESTAMP",
  "L2CPREAD",
  "L2CWRITE",
  "UNUSED",
  "IRQ",
  "DIE"
} ;

#define MSM_RTB_SIZE (1024*1024)
#define CPU_NUM 8
#define ARRAY_SIZE ((MSM_RTB_SIZE)/(CPU_NUM * sizeof(struct msm_rtb_layout)))

struct msm_rtb_layout rtb_entries[CPU_NUM][ARRAY_SIZE] ;

void main(int argc, char* argv[])
{
  FILE* fp ;
  struct msm_rtb_layout buff ;
  int cpu = 0 ;
  int index = 0 ;

  if(argc != 2)
  {
    printf("%s <file>\n", argv[0]) ;
    exit(-1) ;
  }

  fp = fopen(argv[1], "rb") ;
  if(!fp)
  {
    printf("open file errorn \n") ;
    exit(-1) ;
  }

  while(fread(&buff, sizeof(struct msm_rtb_layout), 1, fp) > 0)
  {
    int current_cpu = cpu++%CPU_NUM ;
    if(index >= ARRAY_SIZE*CPU_NUM)
      break ;
    memcpy(&rtb_entries[current_cpu][index++/CPU_NUM], &buff, sizeof(struct msm_rtb_layout)) ;
  }
  fclose(fp) ;

  for(cpu=0; cpu<CPU_NUM; cpu++)
  {
    int i = 0 ;
    int start = 0 ;
    printf(">>>>>>CPU %d last RTB<<<<<<<<\n", cpu) ;

    for(i=0; i<ARRAY_SIZE; i++)
    {
      struct msm_rtb_layout *rtb = &rtb_entries[cpu][i] ;
      struct msm_rtb_layout *rtb_next = &rtb_entries[cpu][(i+1)%ARRAY_SIZE] ;
      if(rtb->idx > rtb_next->idx)
      {
        start = (i+1)%ARRAY_SIZE ;
        break ;
      }
    }

    for(i=0; i<ARRAY_SIZE; i++)
    {
      struct msm_rtb_layout *rtb = &rtb_entries[cpu][(i+start)%ARRAY_SIZE] ;
      printf("type:%10s, idx:%d, caller:0x%16lx, data:0x%16lx\n", type2string[rtb->log_type & 0xF], rtb->idx, rtb->caller, rtb->data) ;
      if(rtb->caller > 0xffffffc000000000)
        printf("PC: 0x%16lx\n", rtb->caller) ;
    }
  }
}


