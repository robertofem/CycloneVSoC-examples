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
#define ALT_RSTMGR_ADDR       rstmgr_vaddress // ALT_CAST(void *, (ALT_CAST(char *, ALT_HPS_ADDR) + ALT_RSTMGR_OFST))
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
#define ALT_DMASECURE_ADDR    pl330_vaddress //    ALT_CAST(void *, (ALT_CAST(char *, ALT_HPS_ADDR) + ALT_DMASECURE_OFST))
/* The lower bound address range of the ALT_DMASECURE component. */
#define ALT_DMASECURE_LB_ADDR     ALT_DMASECURE_ADDR
/* The upper bound address range of the ALT_DMASECURE component. */
#define ALT_DMASECURE_UB_ADDR     ALT_CAST(void *, ((ALT_CAST(char *, ALT_DMASECURE_ADDR) + 0x4) - 1))

#endif