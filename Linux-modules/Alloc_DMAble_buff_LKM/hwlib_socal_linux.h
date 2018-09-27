#ifndef _HWLIB_SOCAL_
#define _HWLIB_SOCAL_

//-------define the chip used. Needed for some options in hwlib library-------//
#ifndef soc_cv_av
  #define soc_cv_av
#endif

#include <linux/kernel.h>    // Contains types, macros, functions for the kernel

//-----------------------------------------------------------------//
//-------------------------code from hwlib.h-----------------------//
//-----------------------------------------------------------------//
#define ALT_MIN(a, b) ((a) > (b) ? (b) : (a))
#define ALT_MAX(a, b) ((a) > (b) ? (a) : (b))

typedef int32_t             ALT_STATUS_CODE;

/*! Definitions of status codes returned by the HWLIB. */

/*! The operation was successful. */
#define ALT_E_SUCCESS               0

/*! The operation failed. */
#define ALT_E_ERROR                 (-1)
/*! FPGA configuration error detected.*/
#define ALT_E_FPGA_CFG              (-2)
/*! FPGA CRC error detected. */
#define ALT_E_FPGA_CRC              (-3)
/*! An error occurred on the FPGA configuration bitstream input source. */
#define ALT_E_FPGA_CFG_STM          (-4)
/*! The FPGA is powered off. */
#define ALT_E_FPGA_PWR_OFF          (-5)
/*! The SoC does not currently control the FPGA. */
#define ALT_E_FPGA_NO_SOC_CTRL      (-6)
/*! The FPGA is not in USER mode. */
#define ALT_E_FPGA_NOT_USER_MODE    (-7)
/*! An argument violates a range constraint. */
#define ALT_E_ARG_RANGE             (-8)
/*! A bad argument value was passed. */
#define ALT_E_BAD_ARG               (-9)
/*! The operation is invalid or illegal. */
#define ALT_E_BAD_OPERATION         (-10)
/*! An invalid option was selected. */
#define ALT_E_INV_OPTION            (-11)
/*! An operation or response timeout period expired. */
#define ALT_E_TMO                   (-12)
/*! The argument value is reserved or unavailable. */
#define ALT_E_RESERVED              (-13)
/*! A clock is not enabled or violates an operational constraint. */
#define ALT_E_BAD_CLK               (-14)
/*! The version ID is invalid. */
#define ALT_E_BAD_VERSION           (-15)
/*! The buffer does not contain enough free space for the operation. */
#define ALT_E_BUF_OVF               (-20)

/*!
 * Provide base address of MPU address space
 */

#ifndef ALT_HPS_ADDR
#define ALT_HPS_ADDR            0
#endif


//-----------------------------------------------------------------//
//---------------code from socal.h (for cyclone V)-----------------//
//-----------------------------------------------------------------//
#define ALT_CAST(type, ptr)  ((type) (ptr))

/*! Write the 32 bit word to the destination address in device memory.
 *  \param dest - Write destination pointer address
 *  \param src  - 32 bit data word to write to memory
 */
#define alt_write_word(dest, src)      iowrite32((u32) src, (void *) dest)

/*! Read and return the 32 bit word from the source address in device memory.
 *  \param src    Read source pointer address
 *  \returns      32 bit data word value
 */
#define alt_read_word(src)        ioread32(src)

/*! Set selected bits in the 32 bit word at the destination address in device memory.
 *  \param dest - Destination pointer address
 *  \param bits - Bits to set in destination word
 */
#define     alt_setbits_word(dest, bits)        (alt_write_word(dest, alt_read_word(dest) | (bits)))

/*! Clear selected bits in the 32 bit word at the destination address in device memory.
 *  \param dest - Destination pointer address
 *  \param bits - Bits to clear in destination word
 */
#define     alt_clrbits_word(dest, bits)        (alt_write_word(dest, alt_read_word(dest) & ~(bits)))

//-----------------------------------------------------------------//
//-------------code from alt_rstmgr.h (for cyclone V)--------------//
//-----------------------------------------------------------------//
/* The byte offset of the ALT_RSTMGR_PERMODRST register from the beginning of the component. */
#define ALT_RSTMGR_PERMODRST_OFST        0x14

/*
 * Field : DMA Controller - dma
 * 
 * Resets DMA controller
 * 
 * Field Access Macros:
 * 
 */
/* The Least Significant Bit (LSB) position of the ALT_RSTMGR_PERMODRST_DMA register field. */
#define ALT_RSTMGR_PERMODRST_DMA_LSB        28
/* The Most Significant Bit (MSB) position of the ALT_RSTMGR_PERMODRST_DMA register field. */
#define ALT_RSTMGR_PERMODRST_DMA_MSB        28
/* The width in bits of the ALT_RSTMGR_PERMODRST_DMA register field. */
#define ALT_RSTMGR_PERMODRST_DMA_WIDTH      1
/* The mask used to set the ALT_RSTMGR_PERMODRST_DMA register field value. */
#define ALT_RSTMGR_PERMODRST_DMA_SET_MSK    0x10000000
/* The mask used to clear the ALT_RSTMGR_PERMODRST_DMA register field value. */
#define ALT_RSTMGR_PERMODRST_DMA_CLR_MSK    0xefffffff
/* The reset value of the ALT_RSTMGR_PERMODRST_DMA register field. */
#define ALT_RSTMGR_PERMODRST_DMA_RESET      0x1
/* Extracts the ALT_RSTMGR_PERMODRST_DMA field value from a register. */
#define ALT_RSTMGR_PERMODRST_DMA_GET(value) (((value) & 0x10000000) >> 28)
/* Produces a ALT_RSTMGR_PERMODRST_DMA register field value suitable for setting the register. */
#define ALT_RSTMGR_PERMODRST_DMA_SET(value) (((value) << 28) & 0x10000000)


//-----------------------------------------------------------------//
//-------------code from socal/hps.h (for cyclone V)---------------//
//-----------------------------------------------------------------//
/*
 * Component Instance : rstmgr
 * 
 * Instance rstmgr of component ALT_RSTMGR. 
 */
/* The address of the ALT_RSTMGR_STAT register for the ALT_RSTMGR instance. */
#define ALT_RSTMGR_STAT_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_RSTMGR_ADDR) + ALT_RSTMGR_STAT_OFST))
/* The address of the ALT_RSTMGR_CTL register for the ALT_RSTMGR instance. */
#define ALT_RSTMGR_CTL_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_RSTMGR_ADDR) + ALT_RSTMGR_CTL_OFST))
/* The address of the ALT_RSTMGR_COUNTS register for the ALT_RSTMGR instance. */
#define ALT_RSTMGR_COUNTS_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_RSTMGR_ADDR) + ALT_RSTMGR_COUNTS_OFST))
/* The address of the ALT_RSTMGR_MPUMODRST register for the ALT_RSTMGR instance. */
#define ALT_RSTMGR_MPUMODRST_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_RSTMGR_ADDR) + ALT_RSTMGR_MPUMODRST_OFST))
/* The address of the ALT_RSTMGR_PERMODRST register for the ALT_RSTMGR instance. */
#define ALT_RSTMGR_PERMODRST_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_RSTMGR_ADDR) + ALT_RSTMGR_PERMODRST_OFST))
/* The address of the ALT_RSTMGR_PER2MODRST register for the ALT_RSTMGR instance. */
#define ALT_RSTMGR_PER2MODRST_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_RSTMGR_ADDR) + ALT_RSTMGR_PER2MODRST_OFST))
/* The address of the ALT_RSTMGR_BRGMODRST register for the ALT_RSTMGR instance. */
#define ALT_RSTMGR_BRGMODRST_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_RSTMGR_ADDR) + ALT_RSTMGR_BRGMODRST_OFST))
/* The address of the ALT_RSTMGR_MISCMODRST register for the ALT_RSTMGR instance. */
#define ALT_RSTMGR_MISCMODRST_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_RSTMGR_ADDR) + ALT_RSTMGR_MISCMODRST_OFST))
/* The base address byte offset for the start of the ALT_RSTMGR component. */
//#define ALT_RSTMGR_OFST        0xffd05000
/* The start address of the ALT_RSTMGR component. */
//#define ALT_RSTMGR_ADDR       rstmgr_vaddress // ALT_CAST(void *, (ALT_CAST(char *, ALT_HPS_ADDR) + ALT_RSTMGR_OFST))
/* The lower bound address range of the ALT_RSTMGR component. */
#define ALT_RSTMGR_LB_ADDR     ALT_RSTMGR_ADDR
/* The upper bound address range of the ALT_RSTMGR component. */
#define ALT_RSTMGR_UB_ADDR     ALT_CAST(void *, ((ALT_CAST(char *, ALT_RSTMGR_ADDR) + 0x100) - 1))



/*
 * Component Instance : dmasecure
 * 
 * Instance dmasecure of component ALT_DMASECURE.
 * 
 * 
 */
/* The address of the ALT_DMASECURE_REG register for the ALT_DMASECURE instance. */
#define ALT_DMASECURE_REG_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_DMASECURE_ADDR) + ALT_DMASECURE_REG_OFST))
/* The base address byte offset for the start of the ALT_DMASECURE component. */
//#define ALT_DMASECURE_OFST        0xffe01000
/* The start address of the ALT_DMASECURE component. */
//#define ALT_DMASECURE_ADDR    pl330_vaddress //    ALT_CAST(void *, (ALT_CAST(char *, ALT_HPS_ADDR) + ALT_DMASECURE_OFST))
/* The lower bound address range of the ALT_DMASECURE component. */
#define ALT_DMASECURE_LB_ADDR     ALT_DMASECURE_ADDR
/* The upper bound address range of the ALT_DMASECURE component. */
#define ALT_DMASECURE_UB_ADDR     ALT_CAST(void *, ((ALT_CAST(char *, ALT_DMASECURE_ADDR) + 0x4) - 1))




/*
 * Component Instance : sysmgr
 * 
 * Instance sysmgr of component ALT_SYSMGR.
 * 
 * 
 */
/* The base address byte offset for the start of the ALT_SYSMGR component. */
//#define ALT_SYSMGR_OFST        0xffd08000
/* The start address of the ALT_SYSMGR component. */
//#define ALT_SYSMGR_ADDR        ALT_CAST(void *, (ALT_CAST(char *, ALT_HPS_ADDR) + ALT_SYSMGR_OFST))
/* The lower bound address range of the ALT_SYSMGR component. */
#define ALT_SYSMGR_LB_ADDR     ALT_SYSMGR_ADDR
/* The upper bound address range of the ALT_SYSMGR component. */
#define ALT_SYSMGR_UB_ADDR     ALT_CAST(void *, ((ALT_CAST(char *, ALT_SYSMGR_ADDR) + 0x4000) - 1))

/* The address of the ALT_SYSMGR_SILICONID1 register for the ALT_SYSMGR instance. */
#define ALT_SYSMGR_SILICONID1_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_SYSMGR_ADDR) + ALT_SYSMGR_SILICONID1_OFST))
/* The address of the ALT_SYSMGR_SILICONID2 register for the ALT_SYSMGR instance. */
#define ALT_SYSMGR_SILICONID2_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_SYSMGR_ADDR) + ALT_SYSMGR_SILICONID2_OFST))
/* The address of the ALT_SYSMGR_WDDBG register for the ALT_SYSMGR instance. */
#define ALT_SYSMGR_WDDBG_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_SYSMGR_ADDR) + ALT_SYSMGR_WDDBG_OFST))
/* The address of the ALT_SYSMGR_BOOT register for the ALT_SYSMGR instance. */
#define ALT_SYSMGR_BOOT_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_SYSMGR_ADDR) + ALT_SYSMGR_BOOT_OFST))
/* The address of the ALT_SYSMGR_HPSINFO register for the ALT_SYSMGR instance. */
#define ALT_SYSMGR_HPSINFO_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_SYSMGR_ADDR) + ALT_SYSMGR_HPSINFO_OFST))
/* The address of the ALT_SYSMGR_PARITYINJ register for the ALT_SYSMGR instance. */
#define ALT_SYSMGR_PARITYINJ_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_SYSMGR_ADDR) + ALT_SYSMGR_PARITYINJ_OFST))

/*
 * Register Group Instance : dmagrp
 * 
 * Instance dmagrp of register group ALT_SYSMGR_DMA.
 * 
 * 
 */
/* The address of the ALT_SYSMGR_DMA_CTL register for the ALT_SYSMGR_DMA instance. */
#define ALT_SYSMGR_DMA_CTL_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_SYSMGR_DMA_ADDR) + ALT_SYSMGR_DMA_CTL_OFST))
/* The address of the ALT_SYSMGR_DMA_PERSECURITY register for the ALT_SYSMGR_DMA instance. */
#define ALT_SYSMGR_DMA_PERSECURITY_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_SYSMGR_DMA_ADDR) + ALT_SYSMGR_DMA_PERSECURITY_OFST))
/* The base address byte offset for the start of the ALT_SYSMGR_DMA component. */
#define ALT_SYSMGR_DMA_OFST        0x70
/* The start address of the ALT_SYSMGR_DMA component. */
#define ALT_SYSMGR_DMA_ADDR        ALT_CAST(void *, (ALT_CAST(char *, ALT_SYSMGR_ADDR) + ALT_SYSMGR_DMA_OFST))
/* The lower bound address range of the ALT_SYSMGR_DMA component. */
#define ALT_SYSMGR_DMA_LB_ADDR     ALT_SYSMGR_DMA_ADDR
/* The upper bound address range of the ALT_SYSMGR_DMA component. */
#define ALT_SYSMGR_DMA_UB_ADDR     ALT_CAST(void *, ((ALT_CAST(char *, ALT_SYSMGR_DMA_ADDR) + 0x8) - 1))


/*
 * Component Instance : acpidmap
 * 
 * Instance acpidmap of component ALT_ACPIDMAP.
 * 
 * 
 */
/* The address of the ALT_ACPIDMAP_VID2RD register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID2RD_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID2RD_OFST))
/* The address of the ALT_ACPIDMAP_VID2WR register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID2WR_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID2WR_OFST))
/* The address of the ALT_ACPIDMAP_VID3RD register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID3RD_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID3RD_OFST))
/* The address of the ALT_ACPIDMAP_VID3WR register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID3WR_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID3WR_OFST))
/* The address of the ALT_ACPIDMAP_VID4RD register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID4RD_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID4RD_OFST))
/* The address of the ALT_ACPIDMAP_VID4WR register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID4WR_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID4WR_OFST))
/* The address of the ALT_ACPIDMAP_VID5RD register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID5RD_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID5RD_OFST))
/* The address of the ALT_ACPIDMAP_VID5WR register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID5WR_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID5WR_OFST))
/* The address of the ALT_ACPIDMAP_VID6RD register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID6RD_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID6RD_OFST))
/* The address of the ALT_ACPIDMAP_VID6WR register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID6WR_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID6WR_OFST))
/* The address of the ALT_ACPIDMAP_DYNRD register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_DYNRD_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_DYNRD_OFST))
/* The address of the ALT_ACPIDMAP_DYNWR register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_DYNWR_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_DYNWR_OFST))
/* The address of the ALT_ACPIDMAP_VID2RD_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID2RD_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID2RD_S_OFST))
/* The address of the ALT_ACPIDMAP_VID2WR_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID2WR_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID2WR_S_OFST))
/* The address of the ALT_ACPIDMAP_VID3RD_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID3RD_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID3RD_S_OFST))
/* The address of the ALT_ACPIDMAP_VID3WR_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID3WR_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID3WR_S_OFST))
/* The address of the ALT_ACPIDMAP_VID4RD_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID4RD_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID4RD_S_OFST))
/* The address of the ALT_ACPIDMAP_VID4WR_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID4WR_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID4WR_S_OFST))
/* The address of the ALT_ACPIDMAP_VID5RD_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID5RD_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID5RD_S_OFST))
/* The address of the ALT_ACPIDMAP_VID5WR_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID5WR_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID5WR_S_OFST))
/* The address of the ALT_ACPIDMAP_VID6RD_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID6RD_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID6RD_S_OFST))
/* The address of the ALT_ACPIDMAP_VID6WR_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_VID6WR_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_VID6WR_S_OFST))
/* The address of the ALT_ACPIDMAP_DYNRD_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_DYNRD_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_DYNRD_S_OFST))
/* The address of the ALT_ACPIDMAP_DYNWR_S register for the ALT_ACPIDMAP instance. */
#define ALT_ACPIDMAP_DYNWR_S_ADDR  ALT_CAST(void *, (ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + ALT_ACPIDMAP_DYNWR_S_OFST))
/* The base address byte offset for the start of the ALT_ACPIDMAP component. */
//#define ALT_ACPIDMAP_OFST        0xff707000
/* The start address of the ALT_ACPIDMAP component. */
//#define ALT_ACPIDMAP_ADDR        ALT_CAST(void *, (ALT_CAST(char *, ALT_HPS_ADDR) + ALT_ACPIDMAP_OFST))
/* The lower bound address range of the ALT_ACPIDMAP component. */
#define ALT_ACPIDMAP_LB_ADDR     ALT_ACPIDMAP_ADDR
/* The upper bound address range of the ALT_ACPIDMAP component. */
#define ALT_ACPIDMAP_UB_ADDR     ALT_CAST(void *, ((ALT_CAST(char *, ALT_ACPIDMAP_ADDR) + 0x1000) - 1))


//-----------------------------------------------------------------//
//----------code from socal/alt_sysmgr.h (for cyclone V)-----------//
//-----------------------------------------------------------------//

/* Extracts the ALT_SYSMGR_HPSINFO_CAN field value from a register. */
#define ALT_SYSMGR_HPSINFO_CAN_GET(value) (((value) & 0x00000002) >> 1)

/* The byte offset of the ALT_SYSMGR_HPSINFO register from the beginning of the component. */
#define ALT_SYSMGR_HPSINFO_OFST        0x18

/*
 * Enumerated value for register field ALT_SYSMGR_HPSINFO_CAN
 * 
 * CAN0 and CAN1 are not available.
 */
#define ALT_SYSMGR_HPSINFO_CAN_E_CAN_UNAVAILABLE    0x0

/*
 * Enumerated value for register field ALT_SYSMGR_HPSINFO_CAN
 * 
 * CAN0 and CAN1 are available.
 */
#define ALT_SYSMGR_HPSINFO_CAN_E_CAN_AVAILABLE      0x1

/* The byte offset of the ALT_SYSMGR_DMA_CTL register from the beginning of the component. */
#define ALT_SYSMGR_DMA_CTL_OFST        0x0

/*
 * Enumerated value for register field ALT_SYSMGR_DMA_CTL_CHANSEL_0
 * 
 * CAN drives peripheral request interface
 */
#define ALT_SYSMGR_DMA_CTL_CHANSEL_0_E_CAN  0x1

/* The Least Significant Bit (LSB) position of the ALT_SYSMGR_DMA_CTL_CHANSEL_0 register field. */
#define ALT_SYSMGR_DMA_CTL_CHANSEL_0_LSB        0
/* The Most Significant Bit (MSB) position of the ALT_SYSMGR_DMA_CTL_CHANSEL_0 register field. */
#define ALT_SYSMGR_DMA_CTL_CHANSEL_0_MSB        0
/* The width in bits of the ALT_SYSMGR_DMA_CTL_CHANSEL_0 register field. */
#define ALT_SYSMGR_DMA_CTL_CHANSEL_0_WIDTH      1
/* The mask used to set the ALT_SYSMGR_DMA_CTL_CHANSEL_0 register field value. */
#define ALT_SYSMGR_DMA_CTL_CHANSEL_0_SET_MSK    0x00000001
/* The mask used to clear the ALT_SYSMGR_DMA_CTL_CHANSEL_0 register field value. */
#define ALT_SYSMGR_DMA_CTL_CHANSEL_0_CLR_MSK    0xfffffffe
/* The reset value of the ALT_SYSMGR_DMA_CTL_CHANSEL_0 register field. */
#define ALT_SYSMGR_DMA_CTL_CHANSEL_0_RESET      0x0
/* Extracts the ALT_SYSMGR_DMA_CTL_CHANSEL_0 field value from a register. */
#define ALT_SYSMGR_DMA_CTL_CHANSEL_0_GET(value) (((value) & 0x00000001) >> 0)
/* Produces a ALT_SYSMGR_DMA_CTL_CHANSEL_0 register field value suitable for setting the register. */
#define ALT_SYSMGR_DMA_CTL_CHANSEL_0_SET(value) (((value) << 0) & 0x00000001)

/* The byte offset of the ALT_SYSMGR_DMA_PERSECURITY register from the beginning of the component. */
#define ALT_SYSMGR_DMA_PERSECURITY_OFST        0x4

/*
 * Field : Manager Thread Security - mgrnonsecure
 * 
 * Specifies the security state of the DMA manager thread.
 * 
 * 0 = assigns DMA manager to the Secure state.
 * 
 * 1 = assigns DMA manager to the Non-secure state.
 * 
 * Sampled by the DMA controller when it exits from reset.
 * 
 * Field Access Macros:
 * 
 */
/* The Least Significant Bit (LSB) position of the ALT_SYSMGR_DMA_CTL_MGRNONSECURE register field. */
#define ALT_SYSMGR_DMA_CTL_MGRNONSECURE_LSB        4
/* The Most Significant Bit (MSB) position of the ALT_SYSMGR_DMA_CTL_MGRNONSECURE register field. */
#define ALT_SYSMGR_DMA_CTL_MGRNONSECURE_MSB        4
/* The width in bits of the ALT_SYSMGR_DMA_CTL_MGRNONSECURE register field. */
#define ALT_SYSMGR_DMA_CTL_MGRNONSECURE_WIDTH      1
/* The mask used to set the ALT_SYSMGR_DMA_CTL_MGRNONSECURE register field value. */
#define ALT_SYSMGR_DMA_CTL_MGRNONSECURE_SET_MSK    0x00000010
/* The mask used to clear the ALT_SYSMGR_DMA_CTL_MGRNONSECURE register field value. */
#define ALT_SYSMGR_DMA_CTL_MGRNONSECURE_CLR_MSK    0xffffffef
/* The reset value of the ALT_SYSMGR_DMA_CTL_MGRNONSECURE register field. */
#define ALT_SYSMGR_DMA_CTL_MGRNONSECURE_RESET      0x0
/* Extracts the ALT_SYSMGR_DMA_CTL_MGRNONSECURE field value from a register. */
#define ALT_SYSMGR_DMA_CTL_MGRNONSECURE_GET(value) (((value) & 0x00000010) >> 4)
/* Produces a ALT_SYSMGR_DMA_CTL_MGRNONSECURE register field value suitable for setting the register. */
#define ALT_SYSMGR_DMA_CTL_MGRNONSECURE_SET(value) (((value) << 4) & 0x00000010)

/*
 * Field : IRQ Security - irqnonsecure
 * 
 * Specifies the security state of an event-interrupt resource.
 * 
 * If bit index [x] is 0, the DMAC assigns event<x> or irq[x] to the Secure state.
 * 
 * If bit index [x] is 1, the DMAC assigns event<x> or irq[x] to the Non-secure
 * state.
 * 
 * Field Access Macros:
 * 
 */
/* The Least Significant Bit (LSB) position of the ALT_SYSMGR_DMA_CTL_IRQNONSECURE register field. */
#define ALT_SYSMGR_DMA_CTL_IRQNONSECURE_LSB        5
/* The Most Significant Bit (MSB) position of the ALT_SYSMGR_DMA_CTL_IRQNONSECURE register field. */
#define ALT_SYSMGR_DMA_CTL_IRQNONSECURE_MSB        12
/* The width in bits of the ALT_SYSMGR_DMA_CTL_IRQNONSECURE register field. */
#define ALT_SYSMGR_DMA_CTL_IRQNONSECURE_WIDTH      8
/* The mask used to set the ALT_SYSMGR_DMA_CTL_IRQNONSECURE register field value. */
#define ALT_SYSMGR_DMA_CTL_IRQNONSECURE_SET_MSK    0x00001fe0
/* The mask used to clear the ALT_SYSMGR_DMA_CTL_IRQNONSECURE register field value. */
#define ALT_SYSMGR_DMA_CTL_IRQNONSECURE_CLR_MSK    0xffffe01f
/* The reset value of the ALT_SYSMGR_DMA_CTL_IRQNONSECURE register field. */
#define ALT_SYSMGR_DMA_CTL_IRQNONSECURE_RESET      0x0
/* Extracts the ALT_SYSMGR_DMA_CTL_IRQNONSECURE field value from a register. */
#define ALT_SYSMGR_DMA_CTL_IRQNONSECURE_GET(value) (((value) & 0x00001fe0) >> 5)
/* Produces a ALT_SYSMGR_DMA_CTL_IRQNONSECURE register field value suitable for setting the register. */
#define ALT_SYSMGR_DMA_CTL_IRQNONSECURE_SET(value) (((value) << 5) & 0x00001fe0)


//-----------------------------------------------------------------//
//-------------code from alt_cache.h (for cyclone V)---------------//
//-----------------------------------------------------------------//
/*!
 * This is the system wide cache line size, given in bytes.
 */
#define ALT_CACHE_LINE_SIZE         32



//-----------------------------------------------------------------//
//-------------code from alt_mmu.h (for cyclone V)---------------//
//-----------------------------------------------------------------//
/*!
 * This type defines the structure used by the VA to PA coalescing API. The
 * fields are internal to the coalescing API and thus not documented.
 */
typedef struct ALT_MMU_VA_TO_PA_COALESCE_s
{
    const char * va;
    size_t       size;

    uintptr_t nextsegpa;
    uint32_t  nextsegsize;

} ALT_MMU_VA_TO_PA_COALESCE_t;

#endif // _HWLIB_SOCAL_