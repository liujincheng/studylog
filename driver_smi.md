分析了68380上的SMI接口，68380将phy寄存器映射到内存空间中中，可通过寄存器去读写。但注意到，这里的寄存器地址只有16个字节，也即并不是完全映射到phy的寄存器的。没有8313模拟SMI操作那么直观。或者，这里的80个bit，只是用于读写外部寄存器，并不是映射关系。
  #define MDIO_EXT_BASE 0xb4e00600 /* External MDIO Registers PER*/
  #define MDIO_EGPHY_BASE 0xb4e00610 /* EGPHY MDIO Registers PER*/
  
#pragma pack(push, 1)
typedef struct
{
  uint32_t reserved:2;// for ECO Reserved bits must be written with 0. A read returns an unknown value.
  uint32_t mdio_busy:1;// CPU writes this bit to 1 in order to initiate MCIO transaction. When transaction completes hardware will clear this bit. Reset value is 0x0.
  uint32_t fail:1;// This bit is set when PHY does not reply to READ command (PHY does not drive 0 on bus turnaround). Reset value is 0x0.
  uint32_t op_code:2;// MDIO command that is OP[1:0]: 00 - Address for clause 45 01 - Write 10 - Read increment for clause 45 11 - Read for clause 45 10 - Read for clause 22 Reset value is 0x0.
  uint32_t phy_prt_addr:5;// PHY address[4:0] for clause 22, Port address[4:0] for Clause 45. Reset value is 0x0.
  uint32_t reg_dec_addr:5;// Register address[4:0] for clause 22, Device address[4:0] for Clause 45. Reset value is 0x0.
  uint32_t data_addr:16;// " MDIO Read/Write data[15:0], clause 22 and 45 or MDIO address[15:0] for clause 45". Reset value is 0x0.

}MDIO_CMD_REG;
#pragma pack(pop)
/****************************************************************************
 * structure describing the CONFIG register of the MDIO block *
 ***************************************************************************/
#pragma pack(push, 1)
typedef struct
{
 uint32_t reserved2:18;// Reserved bits must be written with 0. A read returns an unknown value.
 uint32_t supress_preamble:1;// When this bit set, preamble (32 consectutive 1's) is suppressed for MDIO transaction that is MDIO transaction starts with ST.Reset value is 0x0.
 uint32_t reserved1:1;// Reserved bit must be written with 0. A read returns an unknown value.
 uint32_t mdio_clk_divider:7;// MDIO clock divider[6:0], system clock is divided by 2x(MDIO_CLK_DIVIDER+1) to generate MDIO clock(MDC); system clock = 200Mhz.Minimum supported value: 1 Reset value is 0x7.
 uint32_t reserved0:4;// bits must be written with 0. A read returns an unknown value.
 uint32_t mdio_clause:1;// 0: Clause 45 1: Clause 22 Reset value is 0x1.

}MDIO_CFG_REG;
#pragma pack(pop)
函数说明：
int32_t mdioIsReady(MDIO_TYPE type) type可以为MDIO_EXT_BASE或者MDIO_EGPHY_BASE。该函数读取这个寄存器的CMD区域，检测mdio_busy和fail。
