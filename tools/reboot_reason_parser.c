#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*for 64bit device*/
typedef unsigned long __uint64_t;
typedef __uint64_t    uint64_t;
typedef __uint32_t    uint32_t;
typedef unsigned int __uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define NR_CPUS 8
#define TASK_COMM_LEN 16
#define REBOOT_REASON_SIG (0x43474244)  /* DBRR */

/*
   This group of API call by sub-driver module to report reboot reasons
   aee_rr_* stand for previous reboot reason
 */
struct last_reboot_reason {
  uint32_t fiq_step;
  uint32_t exp_type; /* 0xaeedeadX: X=1 (HWT), X=2 (KE), X=3 (nested panic) */
  uint32_t reboot_mode;

  uint32_t last_irq_enter[NR_CPUS];
  uint64_t jiffies_last_irq_enter[NR_CPUS];

  uint32_t last_irq_exit[NR_CPUS];
  uint64_t jiffies_last_irq_exit[NR_CPUS];

  uint64_t jiffies_last_sched[NR_CPUS];
  char last_sched_comm[NR_CPUS][TASK_COMM_LEN];

  uint8_t hotplug_data1[NR_CPUS];
    uint8_t hotplug_data2;
  uint64_t hotplug_data3;

  uint32_t mcdi_wfi;
  uint32_t mcdi_r15;
  uint32_t deepidle_data;
  uint32_t sodi_data;

  uint32_t spm_suspend_data;
  uint64_t cpu_dormant[NR_CPUS];
  uint32_t clk_data[8];
  uint32_t suspend_debug_flag;

  uint8_t cpu_dvfs_vproc_big;
  uint8_t cpu_dvfs_vproc_little;
  uint8_t cpu_dvfs_oppidx;
  uint8_t cpu_dvfs_status;

  uint8_t gpu_dvfs_vgpu;
  uint8_t gpu_dvfs_oppidx;
  uint8_t gpu_dvfs_status;

  uint64_t ptp_cpu_big_volt;
  uint64_t ptp_cpu_little_volt;
  uint64_t ptp_gpu_volt;
  uint64_t ptp_temp;
  uint8_t ptp_status;


  uint8_t thermal_temp1;
  uint8_t thermal_temp2;
  uint8_t thermal_temp3;
  uint8_t thermal_temp4;
  uint8_t thermal_temp5;
  uint8_t thermal_status;


  void *kparams;
};


struct ram_console_buffer {
  uint32_t sig; 
  /* for size comptible */
  uint32_t off_pl;
  uint32_t off_lpl; /* last preloader: struct reboot_reason_pl*/
  uint32_t sz_pl;
  uint32_t off_lk;
  uint32_t off_llk; /* last lk: struct reboot_reason_lk */
  uint32_t sz_lk;
  uint32_t padding[3];
  uint32_t sz_buffer; 
  uint32_t off_linux; /* struct last_reboot_reason */
  uint32_t off_console;

  /* console buffer*/
  uint32_t log_start;
  uint32_t log_size;
  uint32_t sz_console;
};

static struct ram_console_buffer *ram_console_buffer;
static struct ram_console_buffer *ram_console_old ;

#define LAST_RR_SEC_VAL(header, sect, type, item) \
	header->off_##sect ? ((type*)((void*)header + header->off_##sect))->item : 0
#define LAST_RRR_BUF_VAL(buf, rr_item) LAST_RR_SEC_VAL(buf, linux, struct last_reboot_reason, rr_item)
#define LAST_RRR_VAL(rr_item)  LAST_RR_SEC_VAL(ram_console_old, linux, struct last_reboot_reason, rr_item)



typedef void (*last_rr_show_t)(char *m);
typedef void (*last_rr_show_cpu_t)(char *m, int cpu);

void aee_rr_show_wdt_status(char *m)
{
#if 0
	unsigned int wdt_status;
	struct ram_console_buffer *buffer = ram_console_old;
	if (buffer->off_pl == 0 || buffer->off_pl + ALIGN(buffer->sz_pl, 64) != buffer->off_lpl) {
		/* workaround for compatiblity to old preloader & lk (OTA) */
		wdt_status = *((unsigned char*)buffer + 12);
	} else
		wdt_status = LAST_RRPL_VAL(wdt_status);
	printf( "WDT status: %d", wdt_status);
#endif
}

void aee_rr_show_fiq_step(char *m)
{
	printf( " fiq step: %u ", LAST_RRR_VAL(fiq_step));
}

void aee_rr_show_exp_type(char *m)
{
	unsigned int exp_type = LAST_RRR_VAL(exp_type);
	printf( " exception type: %u\n", (exp_type ^ 0xaeedead0) < 16 ? exp_type ^ 0xaeedead0 : exp_type);
}

void aee_rr_show_last_irq_enter(char *m, int cpu)
{
	printf( "  irq: enter(%d, ", LAST_RRR_VAL(last_irq_enter[cpu]));
}

void aee_rr_show_jiffies_last_irq_enter(char *m, int cpu)
{
	printf( "%lu) ", LAST_RRR_VAL(jiffies_last_irq_enter[cpu]));
}

void aee_rr_show_last_irq_exit(char *m, int cpu)
{
	printf( "quit(%d, ", LAST_RRR_VAL(last_irq_exit[cpu]));
}

void aee_rr_show_jiffies_last_irq_exit(char *m, int cpu)
{
	printf( "%lu)\n", LAST_RRR_VAL(jiffies_last_irq_exit[cpu]));
}

void aee_rr_show_hotplug_data1(char *m, int cpu)
{
	printf( "  hotplug: %d, ", LAST_RRR_VAL(hotplug_data1[cpu]));
}

void aee_rr_show_hotplug_data2(char *m, int cpu)
{
	if (cpu == 0)
		printf( "%d, ", LAST_RRR_VAL(hotplug_data2));
}
void aee_rr_show_hotplug_data3(char *m, int cpu)
{
	if (cpu == 0)
		printf( "0x%lx", LAST_RRR_VAL(hotplug_data3));
	printf( "\n");
}

void aee_rr_show_mcdi(char *m)
{
	printf( "mcdi_wfi: 0x%x\n", LAST_RRR_VAL(mcdi_wfi));
}

void aee_rr_show_mcdi_r15(char *m)
{
	printf( "mcdi_r15: 0x%x\n", LAST_RRR_VAL(mcdi_r15));
}

void aee_rr_show_deepidle(char *m)
{
	printf( "deepidle: 0x%x\n", LAST_RRR_VAL(deepidle_data));
}

void aee_rr_show_sodi(char *m)
{
	printf( "sodi: 0x%x\n", LAST_RRR_VAL(sodi_data));
}

void aee_rr_show_spm_suspend(char *m)
{
	printf( "spm_suspend: 0x%x\n", LAST_RRR_VAL(spm_suspend_data));
}

void aee_rr_show_cpu_dormant(char *m, int cpu)
{
	printf( "  cpu_dormant: 0x%lx\n", LAST_RRR_VAL(cpu_dormant[cpu]));
}

void aee_rr_show_clk(char *m)
{
	int i=0;
	for(i=0; i<8; i++)
		printf( "clk_data: 0x%x\n", LAST_RRR_VAL(clk_data[i]));
}

void aee_rr_show_cpu_dvfs_vproc_big(char *m)
{
	printf( "cpu_dvfs_vproc_big: 0x%x\n", LAST_RRR_VAL(cpu_dvfs_vproc_big));
}

void aee_rr_show_cpu_dvfs_vproc_little(char *m)
{
	printf( "cpu_dvfs_vproc_little: 0x%x\n", LAST_RRR_VAL(cpu_dvfs_vproc_little));
}

void aee_rr_show_cpu_dvfs_oppidx(char *m)
{
	printf( "cpu_dvfs_oppidx: little = 0x%x\n", LAST_RRR_VAL(cpu_dvfs_oppidx) & 0xF);
	printf( "cpu_dvfs_oppidx: big = 0x%x\n", (LAST_RRR_VAL(cpu_dvfs_oppidx) >> 4) & 0xF);
}

void aee_rr_show_cpu_dvfs_status(char *m)
{
	printf( "cpu_dvfs_status: 0x%x\n", LAST_RRR_VAL(cpu_dvfs_status));
}

void aee_rr_show_gpu_dvfs_vgpu(char *m)
{
	printf( "gpu_dvfs_vgpu: 0x%x\n", LAST_RRR_VAL(gpu_dvfs_vgpu));
}

void aee_rr_show_gpu_dvfs_oppidx(char *m)
{
	printf( "gpu_dvfs_oppidx: 0x%x\n", LAST_RRR_VAL(gpu_dvfs_oppidx));
}

void aee_rr_show_gpu_dvfs_status(char *m)
{
	printf( "gpu_dvfs_status: 0x%x\n", LAST_RRR_VAL(gpu_dvfs_status));
}

void aee_rr_show_ptp_cpu_big_volt(char *m)
{
	int i;
	for (i = 0; i < 8; i++)
		printf( "ptp_cpu_big_volt[%d] = %lx\n", i, (LAST_RRR_VAL(ptp_cpu_big_volt) >> (i*8)) & 0xFF);
}

void aee_rr_show_ptp_cpu_little_volt(char *m)
{
	int i;
	for (i = 0; i < 8; i++)
		printf( "ptp_cpu_little_volt[%d] = %lx\n", i, (LAST_RRR_VAL(ptp_cpu_little_volt) >> (i*8)) & 0xFF);
}

void aee_rr_show_ptp_gpu_volt(char *m)
{
	int i;
	for (i = 0; i < 8; i++) 
		printf( "ptp_gpu_volt[%d] = %lx\n", i, (LAST_RRR_VAL(ptp_gpu_volt) >> (i*8)) & 0xFF);
}

void aee_rr_show_ptp_temp(char *m)
{
	printf( "ptp_temp: little = %lx\n", LAST_RRR_VAL(ptp_temp) & 0xFF);
	printf( "ptp_temp: big = %lx\n", (LAST_RRR_VAL(ptp_temp) >> 8) & 0xFF);
	printf( "ptp_temp: GPU = %lx\n", (LAST_RRR_VAL(ptp_temp) >> 16) & 0xFF);
}

void aee_rr_show_thermal_temp(char *m)
{
	printf( "thermal_temp1 = %d\n", LAST_RRR_VAL(thermal_temp1));
	printf( "thermal_temp2 = %d\n", LAST_RRR_VAL(thermal_temp2));
	printf( "thermal_temp3 = %d\n", LAST_RRR_VAL(thermal_temp3));
	printf( "thermal_temp4 = %d\n", LAST_RRR_VAL(thermal_temp4));
	printf( "thermal_temp5 = %d\n", LAST_RRR_VAL(thermal_temp5));
}

void aee_rr_show_ptp_status(char *m)
{
	printf( "ptp_status: 0x%x\n", LAST_RRR_VAL(ptp_status));
}

void aee_rr_show_thermal_status(char *m)
{
	printf( "thermal_status: %d\n", LAST_RRR_VAL(thermal_status));
}

uint32_t get_suspend_debug_flag(void)
{
#if 0
	LAST_RR_GET(suspend_debug_flag);
#endif
}
void aee_rr_show_suspend_debug_flag(char *m)
{
#if 0
    uint32_t flag = get_suspend_debug_flag();
    printf( "SPM Suspend debug = 0x%x\n", flag );
#endif
}

void aee_rr_show_last_pc(char *m)
{
#if 0
	char *reg_buf = kmalloc(4096, GFP_KERNEL);
	if (reg_buf && mt_reg_dump(reg_buf) == 0) {
		printf( "%s\n", reg_buf);
		kfree(reg_buf);
	}
#endif
}

last_rr_show_t aee_rr_show[] = {
	aee_rr_show_wdt_status,
	aee_rr_show_fiq_step,
	aee_rr_show_exp_type,
	aee_rr_show_last_pc,
	aee_rr_show_mcdi,
	aee_rr_show_mcdi_r15,
	aee_rr_show_suspend_debug_flag,
	aee_rr_show_deepidle,
	aee_rr_show_sodi,
	aee_rr_show_spm_suspend,
	aee_rr_show_clk,
	aee_rr_show_cpu_dvfs_vproc_big,
	aee_rr_show_cpu_dvfs_vproc_little,
	aee_rr_show_cpu_dvfs_oppidx,
	aee_rr_show_cpu_dvfs_status,
	aee_rr_show_gpu_dvfs_vgpu,
	aee_rr_show_gpu_dvfs_oppidx,
	aee_rr_show_gpu_dvfs_status,
	aee_rr_show_ptp_cpu_big_volt,
	aee_rr_show_ptp_cpu_little_volt,
	aee_rr_show_ptp_gpu_volt,
	aee_rr_show_ptp_temp,
	aee_rr_show_ptp_status,
	aee_rr_show_thermal_temp,
	aee_rr_show_thermal_status
};

last_rr_show_cpu_t aee_rr_show_cpu[] = {
	aee_rr_show_last_irq_enter,
	aee_rr_show_jiffies_last_irq_enter,
	aee_rr_show_last_irq_exit,
	aee_rr_show_jiffies_last_irq_exit,
	aee_rr_show_hotplug_data1,
	aee_rr_show_hotplug_data2,
	aee_rr_show_hotplug_data3,
	aee_rr_show_cpu_dormant,
};

#define array_size(x) (sizeof(x) / sizeof((x)[0]))
int aee_rr_reboot_reason_show(char *m)
{
	int i, cpu;
#if 0
	if (ram_console_check_header(ram_console_old)) {
		printf( "NO VALID DATA.\n");
		return 0;
	}
#endif
	for (i = 0; i < array_size(aee_rr_show); i++)
	{
		aee_rr_show[i](m);
	}
	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		printf( "CPU %d\n", cpu);
		for (i = 0; i < array_size(aee_rr_show_cpu); i++)
			aee_rr_show_cpu[i](m, cpu);
	}
	return 0;
}

void main(int argc, char* argv[])
{
  FILE* fp ;
  int cpu = 0 ;
  int index = 0 ;
  struct ram_console_buffer ram_console ;
  struct last_reboot_reason reboot_reason ;

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

  fread(&ram_console, sizeof(struct ram_console_buffer), 1, fp) ;
  printf("off_linux=0x%x\n", ram_console.off_linux)  ;
  printf("off_console=0x%x\n", ram_console.off_console)  ;

  fseek(fp, 0, SEEK_SET) ;
  ram_console_old = (struct ram_console_buffer*)malloc(ram_console.off_console) ; 
  fread(ram_console_old, ram_console.off_console, 1, fp) ;
  
  aee_rr_reboot_reason_show(NULL) ;

#define offsetof(s,m) ((unsigned long)(&(((s *)0)->m)))
  printf("offset_of exp_type=%lu\n", offsetof(struct last_reboot_reason, exp_type)) ;

  {
    int i = 0 ;
	printf("\n\n------reboot reason raw data-------\n") ;
    for(; i<sizeof(struct last_reboot_reason); i++)
    {
      if(i%16 == 0)
		printf("%04x: ", i) ;
      printf("%02x ", *((unsigned char*)ram_console_old + ram_console.off_linux + i)) ;
      if((i+1)%16 == 0)
		printf("\n") ;
    }  
  }

  fclose(fp) ;
  free(ram_console_old) ;

}


