/******************************************************************************
*
* Copyright 2013 Altera Corporation. All Rights Reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

/*
 * $Id: //acds/rel/16.1/embedded/ip/hps/altera_hps/hwlib/src/hwmgr/alt_dma.c#2 $
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <alt_printf.h>
#include "alt_cache.h"
#include "alt_dma.h"
#include "alt_mmu.h"

#include "socal/alt_rstmgr.h"
#include "socal/alt_sysmgr.h"

#if ALT_DMA_PERIPH_PROVISION_I2C_SUPPORT
#include "socal/alt_i2c.h"
#endif

#if ALT_DMA_PERIPH_PROVISION_QSPI_SUPPORT
#include "socal/alt_qspi.h"
#endif

#if ALT_DMA_PERIPH_PROVISION_16550_SUPPORT
#include "alt_16550_uart.h"
#include "socal/alt_uart.h"
#endif

#if defined(soc_a10)
#include "socal/alt_ecc_dmac.h"
#endif

#include "socal/socal.h"
#include "socal/hps.h"

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))
#endif

#ifdef DEBUG_ALT_DMA
  #define dprintf  printf
#else
  #define dprintf  null_printf
#endif

/*
 * SoCAL stand in for DMA Controller registers
 *
 * The base can be one of the following:
 *  - ALT_DMANONSECURE_ADDR
 *  - ALT_DMASECURE_ADDR
 *
 * Macros which have a channel parameter does no validation.
 * */

#if defined(soc_a10)
#define ALT_DMASECURE_ADDR    ALT_DMA_SCTL_ADDR
#define ALT_DMANONSECURE_ADDR ALT_DMA_NSCTL_ADDR
#endif

/* DMA Manager Status Register */
#define ALT_DMA_DSR_OFST 0x0
#define ALT_DMA_DSR_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_DSR_OFST))
#define ALT_DMA_DSR_DMASTATUS_SET_MSK 0x0000000f
#define ALT_DMA_DSR_DMASTATUS_GET(value) ((value) & 0x0000000f)

/* DMA Program Counter Register */
#define ALT_DMA_DPC_OFST 0x4
#define ALT_DMA_DPC_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_DPC_OFST))

/* Interrupt Enable Register */
#define ALT_DMA_INTEN_OFST 0x20
#define ALT_DMA_INTEN_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_INTEN_OFST))

/* Event-Interrupt Raw Status Register */
#define ALT_DMA_INT_EVENT_RIS_OFST 0x24
#define ALT_DMA_INT_EVENT_RIS_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_INT_EVENT_RIS_OFST))

/* Interrupt Status Register */
#define ALT_DMA_INTMIS_OFST 0x28
#define ALT_DMA_INTMIS_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_INTMIS_OFST))

/* Interrupt Clear Register */
#define ALT_DMA_INTCLR_OFST 0x2c
#define ALT_DMA_INTCLR_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_INTCLR_OFST))

/* Fault Status DMA Manager Register */
#define ALT_DMA_FSRD_OFST 0x30
#define ALT_DMA_FSRD_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_FSRD_OFST))

/* Fault Status DMA Channel Register */
#define ALT_DMA_FSRC_OFST 0x34
#define ALT_DMA_FSRC_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_FSRC_OFST))

/* Fault Type DMA Manager Register */
#define ALT_DMA_FTRD_OFST 0x38
#define ALT_DMA_FTRD_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_FSRD_OFST))

/* Fault Type DMA Channel Registers */
#define ALT_DMA_FTRx_OFST(channel) (0x40 + 0x4 * (channel))
#define ALT_DMA_FTRx_ADDR(base, channel) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_FTRx_OFST(channel)))

/* Channel Status Registers */
#define ALT_DMA_CSRx_OFST(channel) (0x100 + 0x8 * (channel))
#define ALT_DMA_CSRx_ADDR(base, channel) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_CSRx_OFST(channel)))
#define ALT_DMA_CSRx_CHANNELSTATUS_SET_MSK 0x0000000f
#define ALT_DMA_CSRx_CHANNELSTATUS_GET(value) ((value) & 0x0000000f)

/* Channel Program Counter Registers */
#define ALT_DMA_CPCx_OFST(channel) (0x104 + 0x8 * (channel))
#define ALT_DMA_CPCx_ADDR(base, channel) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_CPCx_OFST(channel)))

/* Source Address Registers */
#define ALT_DMA_SARx_OFST(channel) (0x400 + 0x20 * (channel))
#define ALT_DMA_SARx_ADDR(base, channel) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_SARx_OFST(channel)))

/* Destination Address Registers */
#define ALT_DMA_DARx_OFST(channel) (0x404 + 0x20 * (channel))
#define ALT_DMA_DARx_ADDR(base, channel) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_DARx_OFST(channel)))

/* Channel Control Registers */
#define ALT_DMA_CCRx_OFST(channel) (0x408 + 0x20 * (channel))
#define ALT_DMA_CCRx_ADDR(base, channel) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_CCRx_OFST(channel)))

/* Loop Counter 0 Registers */
#define ALT_DMA_LC0_x_OFST(channel) (0x40c + 0x20 * (channel))
#define ALT_DMA_LC0_x_ADDR(base, channel) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_LC0_x_OFST(channel)))

/* Loop Counter 1 Registers */
#define ALT_DMA_LC1_x_OFST(channel) (0x410 + 0x20 * (channel))
#define ALT_DMA_LC1_x_ADDR(base, channel) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_LC1_x_OFST(channel)))

/* Debug Status Register */
#define ALT_DMA_DBGSTATUS_OFST 0xd00
#define ALT_DMA_DBGSTATUS_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_DBGSTATUS_OFST))

/* Debug Command Register */
#define ALT_DMA_DBGCMD_OFST 0xd04
#define ALT_DMA_DBGCMD_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_DBGCMD_OFST))

/* Debug Instruction-0 Register */
#define ALT_DMA_DBGINST0_OFST 0xd08
#define ALT_DMA_DBGINST0_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_DBGINST0_OFST))
#define ALT_DMA_DBGINST0_CHANNELNUMBER_SET(value) (((value) & 0x7) << 8)
#define ALT_DMA_DBGINST0_DEBUGTHREAD_SET(value) ((value) & 0x1)
#define ALT_DMA_DBGINST0_DEBUGTHREAD_E_MANAGER 0
#define ALT_DMA_DBGINST0_DEBUGTHREAD_E_CHANNEL 1
#define ALT_DMA_DBGINST0_INSTRUCTIONBYTE0_SET(value) (((value) & 0xff) << 16)
#define ALT_DMA_DBGINST0_INSTRUCTIONBYTE1_SET(value) (((value) & 0xff) << 24)

/* Debug Instruction-1 Register */
#define ALT_DMA_DBGINST1_OFST 0xd0c
#define ALT_DMA_DBGINST1_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_DBGINST1_OFST))

/* Configuration Registers 0 - 4 */
#define ALT_DMA_CR0_OFST 0xe00
#define ALT_DMA_CR1_OFST 0xe04
#define ALT_DMA_CR2_OFST 0xe08
#define ALT_DMA_CR3_OFST 0xe0c
#define ALT_DMA_CR4_OFST 0xe10
#define ALT_DMA_CR0_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_CR0_OFST))
#define ALT_DMA_CR1_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_CR1_OFST))
#define ALT_DMA_CR2_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_CR2_OFST))
#define ALT_DMA_CR3_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_CR3_OFST))
#define ALT_DMA_CR4_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_CR4_OFST))

/* DMA Configuration Register */
#define ALT_DMA_CRD_OFST 0xe14
#define ALT_DMA_CRD_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_CRD_OFST))

/* Watchdog Register */
#define ALT_DMA_WD_OFST 0xe80
#define ALT_DMA_WD_ADDR(base) ALT_CAST(void *, (ALT_CAST(char *, (base)) + ALT_DMA_WD_OFST))

/*
 * Internal Data structures
 * */

/* This flag marks the channel as being allocated. */
#define ALT_DMA_CHANNEL_INFO_FLAG_ALLOCED (1 << 0)

typedef struct ALT_DMA_CHANNEL_INFO_s
{
    uint8_t flag;
}
ALT_DMA_CHANNEL_INFO_t;

static struct
{
    /* State information fo each DMA channel. */
    ALT_DMA_CHANNEL_INFO_t channel_info[8];

#if defined(soc_cv_av)

    /* This variable is true if CAN is available in the HPS. */
    bool can_exist;

#elif defined(soc_a10)

#else
#error Unsupported SoCFPGA device.
#endif

} g_dmaState;

/*
 * If users are not using the MMU (and not including alt_mmu.c), this function
 * will resolve to translating a flat memory mapping. This is needed to
 * decouple the MMU from the DMA modules.
 * */
__attribute__((weak)) uintptr_t alt_mmu_va_to_pa(const void * va, uint32_t * seglength, uint32_t * dfsr)
{
    *seglength = 0xffffffff;
    *dfsr      = 0;
    return (uintptr_t)va;
}

__attribute__((weak)) ALT_STATUS_CODE alt_mmu_va_to_pa_coalesce_begin(ALT_MMU_VA_TO_PA_COALESCE_t * coalesce, const void * va, size_t size)
{
    if ((uintptr_t)va + size - 1 < (uintptr_t)va)
    {
        return ALT_E_ERROR;
    }

    coalesce->va   = va;
    coalesce->size = size;

    coalesce->nextsegpa   = (uintptr_t)va;
    coalesce->nextsegsize = size;

    return ALT_E_SUCCESS;
}

__attribute__((weak)) ALT_STATUS_CODE alt_mmu_va_to_pa_coalesce_next(ALT_MMU_VA_TO_PA_COALESCE_t * coalesce, uintptr_t * segpa, uint32_t * segsize)
{
    if (coalesce->size == 0)
    {
        return ALT_E_ERROR;
    }
    
    coalesce->size = 0;

    *segpa   = coalesce->nextsegpa;
    *segsize = coalesce->nextsegsize;

    coalesce->nextsegpa   = 0;
    coalesce->nextsegsize = 0;

    return ALT_E_SUCCESS;
}

__attribute__((weak)) ALT_STATUS_CODE alt_mmu_va_to_pa_coalesce_end(ALT_MMU_VA_TO_PA_COALESCE_t * coalesce)
{
    if (coalesce->size)
    {
        return ALT_E_ERROR;
    }
    else
    {
        return ALT_E_SUCCESS;
    }
}

/*
 * If users are not using the cache (and not including alt_cache.c) this
 * function will resolve to a no-op. This is needed to decouple DMA from cache.
 * */
__attribute__((weak)) ALT_STATUS_CODE alt_cache_system_clean(void * address, size_t length)
{
    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_init(const ALT_DMA_CFG_t * dma_cfg)
{
    int i;
#if defined(soc_cv_av)

    /* Update the System Manager DMA configuration items */
    uint32_t dmactrl = 0;

    /* Update the System Manager DMA peripheral security items */
    uint32_t dmapersecurity = 0;

    /* Initialize the channel information array */
    for (i = 0; i < ARRAY_COUNT(g_dmaState.channel_info); ++i)
    {
        g_dmaState.channel_info[i].flag = 0;
    }

    /* See if CAN is available on the system. */
    g_dmaState.can_exist = ALT_SYSMGR_HPSINFO_CAN_GET(alt_read_word(ALT_SYSMGR_HPSINFO_ADDR))
                        == ALT_SYSMGR_HPSINFO_CAN_E_CAN_AVAILABLE;

    /* Handle FPGA / CAN muxing */
    for (i = 0; i < ARRAY_COUNT(dma_cfg->periph_mux); ++i)
    {
        /* The default is FPGA. */
        switch (dma_cfg->periph_mux[i])
        {
        case ALT_DMA_PERIPH_MUX_DEFAULT:
        case ALT_DMA_PERIPH_MUX_FPGA:
            break;
        case ALT_DMA_PERIPH_MUX_CAN:
            if (!g_dmaState.can_exist)
            {
                return ALT_E_BAD_ARG;
            }
            dmactrl |= (ALT_SYSMGR_DMA_CTL_CHANSEL_0_SET_MSK << i);
            break;
        default:
            return ALT_E_ERROR;
        }
    }

    /* Handle Manager security */
    /* Default is Secure state. */
    switch (dma_cfg->manager_sec)
    {
    case ALT_DMA_SECURITY_DEFAULT:
    case ALT_DMA_SECURITY_SECURE:
        break;
    case ALT_DMA_SECURITY_NONSECURE:
        dmactrl |= ALT_SYSMGR_DMA_CTL_MGRNONSECURE_SET_MSK;
        break;
    default:
        return ALT_E_ERROR;
    }

    /* Handle IRQ security */
    for (i = 0; i < ALT_SYSMGR_DMA_CTL_IRQNONSECURE_WIDTH; ++i)
    {
        /* Default is Secure state. */
        switch (dma_cfg->irq_sec[i])
        {
        case ALT_DMA_SECURITY_DEFAULT:
        case ALT_DMA_SECURITY_SECURE:
            break;
        case ALT_DMA_SECURITY_NONSECURE:
            dmactrl |= (1 << (i + ALT_SYSMGR_DMA_CTL_IRQNONSECURE_LSB));
            break;
        default:
            return ALT_E_ERROR;
        }
    }

    alt_write_word(ALT_SYSMGR_DMA_CTL_ADDR, dmactrl);

    for (i = 0; i < ARRAY_COUNT(dma_cfg->periph_sec); ++i)
    {
        /* Default is Secure state. */
        switch (dma_cfg->periph_sec[i])
        {
        case ALT_DMA_SECURITY_DEFAULT:
        case ALT_DMA_SECURITY_SECURE:
            break;
        case ALT_DMA_SECURITY_NONSECURE:
            dmapersecurity |= (1 << i);
            break;
        default:
            return ALT_E_ERROR;
        }
    }

    alt_write_word(ALT_SYSMGR_DMA_PERSECURITY_ADDR, dmapersecurity);

    /* Take DMA out of reset. */

    alt_clrbits_word(ALT_RSTMGR_PERMODRST_ADDR, ALT_RSTMGR_PERMODRST_DMA_SET_MSK);

#elif defined(soc_a10)

    /* Update the System Manager DMA configuration items */
    uint32_t sysmgrdma = 0;
    uint32_t sysmgrdmaperiph = 0;

    /* Initialize the channel information array */
    for (i = 0; i < ARRAY_COUNT(g_dmaState.channel_info); ++i)
    {
        g_dmaState.channel_info[i].flag = 0;
    }

    /* Handle FPGA / {Security Manager / I2C4} muxing */

    switch (dma_cfg->periph_mux[0]) /* For index 0, default is security manager. */
    {
    case ALT_DMA_PERIPH_MUX_DEFAULT:
    case ALT_DMA_PERIPH_MUX_SECMGR:
        sysmgrdma |= ALT_SYSMGR_DMA_CHANSEL_2_SET(ALT_SYSMGR_DMA_CHANSEL_2_E_SECMGR);
        break;
    case ALT_DMA_PERIPH_MUX_FPGA:
        sysmgrdma |= ALT_SYSMGR_DMA_CHANSEL_2_SET(ALT_SYSMGR_DMA_CHANSEL_2_E_FPGA);
        break;
    default:
        return ALT_E_ERROR;
    }

    switch (dma_cfg->periph_mux[1]) /* For index 1, default is FPGA. */
    {
    case ALT_DMA_PERIPH_MUX_DEFAULT:
    case ALT_DMA_PERIPH_MUX_FPGA:
        sysmgrdma |= ALT_SYSMGR_DMA_CHANSEL_1_SET(ALT_SYSMGR_DMA_CHANSEL_1_E_FPGA);
        break;
    case ALT_DMA_PERIPH_MUX_I2C:
        sysmgrdma |= ALT_SYSMGR_DMA_CHANSEL_1_SET(ALT_SYSMGR_DMA_CHANSEL_1_E_I2C4_RX);
        break;
    default:
        return ALT_E_ERROR;
    }

    switch (dma_cfg->periph_mux[2]) /* For index 2, default is FPGA. */
    {
    case ALT_DMA_PERIPH_MUX_DEFAULT:
    case ALT_DMA_PERIPH_MUX_FPGA:
        sysmgrdma |= ALT_SYSMGR_DMA_CHANSEL_0_SET(ALT_SYSMGR_DMA_CHANSEL_0_E_FPGA);
        break;
    case ALT_DMA_PERIPH_MUX_I2C:
        sysmgrdma |= ALT_SYSMGR_DMA_CHANSEL_0_SET(ALT_SYSMGR_DMA_CHANSEL_0_E_I2C4_TX);
        break;
    default:
        return ALT_E_ERROR;
    }

    /* Handle Manager security */

    switch (dma_cfg->manager_sec)
    {
    case ALT_DMA_SECURITY_DEFAULT:
    case ALT_DMA_SECURITY_SECURE:
        break;
    case ALT_DMA_SECURITY_NONSECURE:
        sysmgrdma |= ALT_SYSMGR_DMA_MGR_NS_SET_MSK;
        break;
    default:
        return ALT_E_ERROR;
    }

    /* Handle IRQ Security */

    for (i = 0; i < ALT_SYSMGR_DMA_IRQ_NS_WIDTH; ++i)
    {
        switch (dma_cfg->irq_sec[i])
        {
        case ALT_DMA_SECURITY_DEFAULT:
        case ALT_DMA_SECURITY_SECURE:
            break;
        case ALT_DMA_SECURITY_NONSECURE:
            sysmgrdma |= (1 << (i + ALT_SYSMGR_DMA_IRQ_NS_LSB));
            break;
        default:
            return ALT_E_ERROR;
        }
    }

    /* Write out the sysmgrdma value. */

    alt_write_word(ALT_SYSMGR_DMA_ADDR, sysmgrdma);

    /* Update System Manager DMA peripheral security items */

    for (i = 0; i < ALT_SYSMGR_DMA_PERIPH_NS_WIDTH; ++i)
    {
        /* Default is Secure state. */
        switch (dma_cfg->periph_sec[i])
        {
        case ALT_DMA_SECURITY_DEFAULT:
        case ALT_DMA_SECURITY_SECURE:
            break;
        case ALT_DMA_SECURITY_NONSECURE:
            sysmgrdmaperiph |= (1 << i);
            break;
        default:
            return ALT_E_ERROR;
        }
    }

    alt_write_word(ALT_SYSMGR_DMA_PERIPH_ADDR, sysmgrdmaperiph);

    /* Take DMA out of reset. */

    alt_clrbits_word(ALT_RSTMGR_PER0MODRST_ADDR, ALT_RSTMGR_PER0MODRST_DMA_SET_MSK);

#endif

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_uninit(void)
{
    int i;
    /* DMAKILL all channel and free all allocated channels. */
    for (i = 0; i < ARRAY_COUNT(g_dmaState.channel_info); ++i)
    {
        if (g_dmaState.channel_info[i].flag & ALT_DMA_CHANNEL_INFO_FLAG_ALLOCED)
        {
            alt_dma_channel_kill((ALT_DMA_CHANNEL_t)i);
            alt_dma_channel_free((ALT_DMA_CHANNEL_t)i);
        }
    }

    /* Put DMA into reset. */

#if defined(soc_cv_av)

    alt_setbits_word(ALT_RSTMGR_PERMODRST_ADDR, ALT_RSTMGR_PERMODRST_DMA_SET_MSK);

#elif defined(soc_a10)

    alt_setbits_word(ALT_RSTMGR_PER0MODRST_ADDR, ALT_RSTMGR_PER0MODRST_DMA_SET_MSK);

#endif

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_channel_alloc(ALT_DMA_CHANNEL_t channel)
{
    /* Validate channel */
    switch (channel)
    {
    case ALT_DMA_CHANNEL_0:
    case ALT_DMA_CHANNEL_1:
    case ALT_DMA_CHANNEL_2:
    case ALT_DMA_CHANNEL_3:
    case ALT_DMA_CHANNEL_4:
    case ALT_DMA_CHANNEL_5:
    case ALT_DMA_CHANNEL_6:
    case ALT_DMA_CHANNEL_7:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* Verify channel is unallocated */

    if (g_dmaState.channel_info[channel].flag & ALT_DMA_CHANNEL_INFO_FLAG_ALLOCED)
    {
        return ALT_E_ERROR;
    }

    /* Mark channel as allocated */

    g_dmaState.channel_info[channel].flag |= ALT_DMA_CHANNEL_INFO_FLAG_ALLOCED;

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_channel_alloc_any(ALT_DMA_CHANNEL_t * allocated)
{
    /* Sweep channel array for unallocated channel */
    int i;
    for (i = 0; i < ARRAY_COUNT(g_dmaState.channel_info); ++i)
    {
        if (!(g_dmaState.channel_info[i].flag & ALT_DMA_CHANNEL_INFO_FLAG_ALLOCED))
        {
            /* Allocate that free channel. */

            ALT_STATUS_CODE status = alt_dma_channel_alloc((ALT_DMA_CHANNEL_t)i);
            if (status == ALT_E_SUCCESS)
            {
                *allocated = (ALT_DMA_CHANNEL_t)i;
            }
            return status;
        }
    }

    /* No free channels found. */

    return ALT_E_ERROR;
}

ALT_STATUS_CODE alt_dma_channel_free(ALT_DMA_CHANNEL_t channel)
{
    ALT_DMA_CHANNEL_STATE_t state;
    ALT_STATUS_CODE status;

    /* Validate channel */
    switch (channel)
    {
    case ALT_DMA_CHANNEL_0:
    case ALT_DMA_CHANNEL_1:
    case ALT_DMA_CHANNEL_2:
    case ALT_DMA_CHANNEL_3:
    case ALT_DMA_CHANNEL_4:
    case ALT_DMA_CHANNEL_5:
    case ALT_DMA_CHANNEL_6:
    case ALT_DMA_CHANNEL_7:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* Verify channel is allocated */

    if (!(g_dmaState.channel_info[channel].flag & ALT_DMA_CHANNEL_INFO_FLAG_ALLOCED))
    {
        return ALT_E_ERROR;
    }

    /* Verify channel is stopped */

    status = alt_dma_channel_state_get(channel, &state);
    if (status != ALT_E_SUCCESS)
    {
        return status;
    }
    if (state != ALT_DMA_CHANNEL_STATE_STOPPED)
    {
        return ALT_E_ERROR;
    }

    /* Mark channel as unallocated. */

    g_dmaState.channel_info[channel].flag &= ~ALT_DMA_CHANNEL_INFO_FLAG_ALLOCED;

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_channel_exec(ALT_DMA_CHANNEL_t channel, ALT_DMA_PROGRAM_t * pgm)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    uintptr_t pgmpa = 0;

    /* Validate channel */
    switch (channel)
    {
    case ALT_DMA_CHANNEL_0:
    case ALT_DMA_CHANNEL_1:
    case ALT_DMA_CHANNEL_2:
    case ALT_DMA_CHANNEL_3:
    case ALT_DMA_CHANNEL_4:
    case ALT_DMA_CHANNEL_5:
    case ALT_DMA_CHANNEL_6:
    case ALT_DMA_CHANNEL_7:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* Verify channel is allocated */

    if (!(g_dmaState.channel_info[channel].flag & ALT_DMA_CHANNEL_INFO_FLAG_ALLOCED))
    {
        return ALT_E_ERROR;
    }

    /* Verify channel is stopped */

    if (status == ALT_E_SUCCESS)
    {
        ALT_DMA_CHANNEL_STATE_t state;
        status = alt_dma_channel_state_get(channel, &state);

        if (state != ALT_DMA_CHANNEL_STATE_STOPPED)
        {
            return ALT_E_ERROR;
        }
    }

    /* Validate the program */

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_validate(pgm);
    }

    /* Sync the DMA program to RAM. */

    if (status == ALT_E_SUCCESS)
    {
        void * vaddr  = (void *)((uintptr_t)(pgm->program + pgm->buffer_start) & ~(ALT_CACHE_LINE_SIZE - 1));
        void * vend   = (void *)(((uintptr_t)(pgm->program + pgm->buffer_start + pgm->code_size) + (ALT_CACHE_LINE_SIZE - 1)) & ~(ALT_CACHE_LINE_SIZE - 1));
        size_t length = (uintptr_t)vend - (uintptr_t)vaddr;

        status = alt_cache_system_clean(vaddr, length);
    }

    /*
     * Get the PA of the program buffer.
     * */

    if (status == ALT_E_SUCCESS)
    {
        uint32_t dfsr;
        uint32_t seglength;
        pgmpa = alt_mmu_va_to_pa(pgm->program + pgm->buffer_start, &seglength, &dfsr);
        if (dfsr)
        {
            dprintf("DMA[exec]: ERROR: Cannot get VA-to-PA of pgm->program + pgm->buffer_start= %p.\n", pgm->program + pgm->buffer_start);
            status = ALT_E_ERROR;
        }
    }

    /*
     * Execute the program
     * */

    /* Configure DBGINST0 and DBGINST1 to execute DMAGO targetting the requested channel. */

    /* For information on APB Interface, see PL330, section 2.5.1.
     * For information on DBGINSTx, see PL330, section 3.3.20 - 3.3.21.
     * For information on DMAGO, see PL330, section 4.3.5. */

    if (status == ALT_E_SUCCESS)
    {
        dprintf("DMA[exec]: program = 0x%x (PA).\n", pgmpa);

        alt_write_word(ALT_DMA_DBGINST0_ADDR(ALT_DMASECURE_ADDR),
                       ALT_DMA_DBGINST0_INSTRUCTIONBYTE0_SET(0xa0) |
                       ALT_DMA_DBGINST0_INSTRUCTIONBYTE1_SET(channel));

        alt_write_word(ALT_DMA_DBGINST1_ADDR(ALT_DMASECURE_ADDR), pgmpa);

        /* Execute the instruction held in DBGINST{0,1} */

        /* For information on DBGCMD, see PL330, section 3.3.19. */

        alt_write_word(ALT_DMA_DBGCMD_ADDR(ALT_DMASECURE_ADDR), 0);
    }

    return status;
}

ALT_STATUS_CODE alt_dma_channel_kill(ALT_DMA_CHANNEL_t channel)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_DMA_CHANNEL_STATE_t current;
    uint32_t i = 20000;

    /* Validate channel */
    switch (channel)
    {
    case ALT_DMA_CHANNEL_0:
    case ALT_DMA_CHANNEL_1:
    case ALT_DMA_CHANNEL_2:
    case ALT_DMA_CHANNEL_3:
    case ALT_DMA_CHANNEL_4:
    case ALT_DMA_CHANNEL_5:
    case ALT_DMA_CHANNEL_6:
    case ALT_DMA_CHANNEL_7:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* Verify channel is allocated */

    if (!(g_dmaState.channel_info[channel].flag & ALT_DMA_CHANNEL_INFO_FLAG_ALLOCED))
    {
        return ALT_E_ERROR;
    }

    /* NOTE: Don't worry about the current channel state. Just issue DMAKILL
     *   instruction. The channel state cannot move from from Stopped back to
     *   Killing. */

    /* Configure DBGINST0 to execute DMAKILL on the requested channel thread.
     * DMAKILL is short enough not to use DBGINST1 register. */

    /* For information on APB Interface, see PL330, section 2.5.1.
     * For information on DBGINSTx, see PL330, section 3.3.20 - 3.3.21.
     * For information on DMAKILL, see PL330, section 4.3.6. */

    alt_write_word(ALT_DMA_DBGINST0_ADDR(ALT_DMASECURE_ADDR),
                   ALT_DMA_DBGINST0_INSTRUCTIONBYTE0_SET(0x1) |
                   ALT_DMA_DBGINST0_CHANNELNUMBER_SET(channel) |
                   ALT_DMA_DBGINST0_DEBUGTHREAD_SET(ALT_DMA_DBGINST0_DEBUGTHREAD_E_CHANNEL));

    /* Execute the instruction held in DBGINST0 */

    /* For information on DBGCMD, see PL330, section 3.3.19. */

    alt_write_word(ALT_DMA_DBGCMD_ADDR(ALT_DMASECURE_ADDR), 0);

    /* Wait for channel to move to KILLING or STOPPED state. Do not wait for
     * the STOPPED only. If the AXI transaction hangs permanently, it can be
     * waiting indefinately. */

    while (--i)
    {
        status = alt_dma_channel_state_get(channel, &current);
        if (status != ALT_E_SUCCESS)
        {
            break;
        }
        if (   (current == ALT_DMA_CHANNEL_STATE_KILLING)
            || (current == ALT_DMA_CHANNEL_STATE_STOPPED))
        {
            break;
        }
    }

    if (i == 0)
    {
        status = ALT_E_TMO;
    }

    return status;
}

ALT_STATUS_CODE alt_dma_channel_reg_get(ALT_DMA_CHANNEL_t channel,
                                        ALT_DMA_PROGRAM_REG_t reg, uint32_t * val)
{
    /* Validate channel */
    switch (channel)
    {
    case ALT_DMA_CHANNEL_0:
    case ALT_DMA_CHANNEL_1:
    case ALT_DMA_CHANNEL_2:
    case ALT_DMA_CHANNEL_3:
    case ALT_DMA_CHANNEL_4:
    case ALT_DMA_CHANNEL_5:
    case ALT_DMA_CHANNEL_6:
    case ALT_DMA_CHANNEL_7:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* For information on SAR, see PL330, section 3.3.13.
     * For information on DAR, see PL330, section 3.3.14.
     * For information on CCR, see PL330, section 3.3.15. */

    switch (reg)
    {
    case ALT_DMA_PROGRAM_REG_SAR:
        *val = alt_read_word(ALT_DMA_SARx_ADDR(ALT_DMASECURE_ADDR, channel));
        break;
    case ALT_DMA_PROGRAM_REG_DAR:
        *val = alt_read_word(ALT_DMA_DARx_ADDR(ALT_DMASECURE_ADDR, channel));
        break;
    case ALT_DMA_PROGRAM_REG_CCR:
        *val = alt_read_word(ALT_DMA_CCRx_ADDR(ALT_DMASECURE_ADDR, channel));
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_send_event(ALT_DMA_EVENT_t evt_num)
{
    /* Validate evt_num */

    switch (evt_num)
    {
    case ALT_DMA_EVENT_0:
    case ALT_DMA_EVENT_1:
    case ALT_DMA_EVENT_2:
    case ALT_DMA_EVENT_3:
    case ALT_DMA_EVENT_4:
    case ALT_DMA_EVENT_5:
    case ALT_DMA_EVENT_6:
    case ALT_DMA_EVENT_7:
    case ALT_DMA_EVENT_ABORT:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* Issue the DMASEV on the DMA manager thread.
     * DMASEV is short enough not to use DBGINST1 register. */

    /* For information on APB Interface, see PL330, section 2.5.1.
     * For information on DBGINSTx, see PL330, section 3.3.20 - 3.3.21.
     * For information on DMASEV, see PL330, section 4.3.15. */

    alt_write_word(ALT_DMA_DBGINST0_ADDR(ALT_DMASECURE_ADDR),
                   ALT_DMA_DBGINST0_INSTRUCTIONBYTE0_SET(0x34) | /* opcode for DMASEV */
                   ALT_DMA_DBGINST0_INSTRUCTIONBYTE1_SET(evt_num << 3) |
                   ALT_DMA_DBGINST0_DEBUGTHREAD_SET(ALT_DMA_DBGINST0_DEBUGTHREAD_E_MANAGER)
        );

    /* Execute the instruction held in DBGINST0 */

    /* For information on DBGCMD, see PL330, section 3.3.19. */

    alt_write_word(ALT_DMA_DBGCMD_ADDR(ALT_DMASECURE_ADDR), 0);

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_manager_state_get(ALT_DMA_MANAGER_STATE_t * state)
{
    /* For information on DSR, see PL330, section 3.3.1. */

    uint32_t raw_state = alt_read_word(ALT_DMA_DSR_ADDR(ALT_DMASECURE_ADDR));

    *state = (ALT_DMA_MANAGER_STATE_t)ALT_DMA_DSR_DMASTATUS_GET(raw_state);

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_channel_state_get(ALT_DMA_CHANNEL_t channel,
                                          ALT_DMA_CHANNEL_STATE_t * state)
{
    uint32_t raw_state;
    /* Validate channel */
    switch (channel)
    {
    case ALT_DMA_CHANNEL_0:
    case ALT_DMA_CHANNEL_1:
    case ALT_DMA_CHANNEL_2:
    case ALT_DMA_CHANNEL_3:
    case ALT_DMA_CHANNEL_4:
    case ALT_DMA_CHANNEL_5:
    case ALT_DMA_CHANNEL_6:
    case ALT_DMA_CHANNEL_7:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* For information on CSR, see PL330, section 3.3.11. */

    raw_state = alt_read_word(ALT_DMA_CSRx_ADDR(ALT_DMASECURE_ADDR, channel));

    *state = (ALT_DMA_CHANNEL_STATE_t)ALT_DMA_CSRx_CHANNELSTATUS_GET(raw_state);

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_manager_fault_status_get(ALT_DMA_MANAGER_FAULT_t * fault)
{
    /* For information on FTRD, see PL330, section 3.3.9. */

    *fault = (ALT_DMA_MANAGER_FAULT_t)alt_read_word(ALT_DMA_FTRD_ADDR(ALT_DMASECURE_ADDR));

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_channel_fault_status_get(ALT_DMA_CHANNEL_t channel,
                                                 ALT_DMA_CHANNEL_FAULT_t * fault)
{
    /* Validate channel */
    switch (channel)
    {
    case ALT_DMA_CHANNEL_0:
    case ALT_DMA_CHANNEL_1:
    case ALT_DMA_CHANNEL_2:
    case ALT_DMA_CHANNEL_3:
    case ALT_DMA_CHANNEL_4:
    case ALT_DMA_CHANNEL_5:
    case ALT_DMA_CHANNEL_6:
    case ALT_DMA_CHANNEL_7:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* For information on FTR, see PL330, section 3.3.10. */

    *fault = (ALT_DMA_CHANNEL_FAULT_t)alt_read_word(ALT_DMA_FTRx_ADDR(ALT_DMASECURE_ADDR, channel));

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_event_int_select(ALT_DMA_EVENT_t evt_num,
                                         ALT_DMA_EVENT_SELECT_t opt)
{
    /* Validate evt_num */
    switch (evt_num)
    {
    case ALT_DMA_EVENT_0:
    case ALT_DMA_EVENT_1:
    case ALT_DMA_EVENT_2:
    case ALT_DMA_EVENT_3:
    case ALT_DMA_EVENT_4:
    case ALT_DMA_EVENT_5:
    case ALT_DMA_EVENT_6:
    case ALT_DMA_EVENT_7:
    case ALT_DMA_EVENT_ABORT:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* For information on INTEN, see PL330, section 3.3.3. */

    switch (opt)
    {
    case ALT_DMA_EVENT_SELECT_SEND_EVT:
        alt_clrbits_word(ALT_DMA_INTEN_ADDR(ALT_DMASECURE_ADDR), 1 << evt_num);
        break;
    case ALT_DMA_EVENT_SELECT_SIG_IRQ:
        alt_setbits_word(ALT_DMA_INTEN_ADDR(ALT_DMASECURE_ADDR), 1 << evt_num);
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_dma_event_int_status_get_raw(ALT_DMA_EVENT_t evt_num)
{
    uint32_t status_raw;

    /* Validate evt_num */
    switch (evt_num)
    {
    case ALT_DMA_EVENT_0:
    case ALT_DMA_EVENT_1:
    case ALT_DMA_EVENT_2:
    case ALT_DMA_EVENT_3:
    case ALT_DMA_EVENT_4:
    case ALT_DMA_EVENT_5:
    case ALT_DMA_EVENT_6:
    case ALT_DMA_EVENT_7:
    case ALT_DMA_EVENT_ABORT:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* For information on INT_EVENT_RIS, see PL330, section 3.3.4. */

    status_raw = alt_read_word(ALT_DMA_INT_EVENT_RIS_ADDR(ALT_DMASECURE_ADDR));

    if (status_raw & (1 << evt_num))
    {
        return ALT_E_TRUE;
    }
    else
    {
        return ALT_E_FALSE;
    }
}

ALT_STATUS_CODE alt_dma_int_status_get(ALT_DMA_EVENT_t irq_num)
{
    uint32_t int_status;

    /* Validate evt_num */
    switch (irq_num)
    {
    case ALT_DMA_EVENT_0:
    case ALT_DMA_EVENT_1:
    case ALT_DMA_EVENT_2:
    case ALT_DMA_EVENT_3:
    case ALT_DMA_EVENT_4:
    case ALT_DMA_EVENT_5:
    case ALT_DMA_EVENT_6:
    case ALT_DMA_EVENT_7:
    case ALT_DMA_EVENT_ABORT:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* For information on INTMIS, see PL330, section 3.3.5. */

    int_status = alt_read_word(ALT_DMA_INTMIS_ADDR(ALT_DMASECURE_ADDR));

    if (int_status & (1 << irq_num))
    {
        return ALT_E_TRUE;
    }
    else
    {
        return ALT_E_FALSE;
    }
}

ALT_STATUS_CODE alt_dma_int_clear(ALT_DMA_EVENT_t irq_num)
{
    /* Validate evt_num */
    switch (irq_num)
    {
    case ALT_DMA_EVENT_0:
    case ALT_DMA_EVENT_1:
    case ALT_DMA_EVENT_2:
    case ALT_DMA_EVENT_3:
    case ALT_DMA_EVENT_4:
    case ALT_DMA_EVENT_5:
    case ALT_DMA_EVENT_6:
    case ALT_DMA_EVENT_7:
    case ALT_DMA_EVENT_ABORT:
        break;
    default:
        return ALT_E_BAD_ARG;
    }

    /* For information on INTCLR, see PL330, section 3.3.6. */

    alt_write_word(ALT_DMA_INTCLR_ADDR(ALT_DMASECURE_ADDR), 1 << irq_num);

    return ALT_E_SUCCESS;
}

static ALT_STATUS_CODE alt_dma_memory_to_memory_segment(ALT_DMA_PROGRAM_t * program,
                                                        uintptr_t segdstpa,
                                                        uintptr_t segsrcpa,
                                                        size_t segsize)
{
    uint32_t burstcount;
    bool correction;
    size_t sizeleft = segsize;
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR, segsrcpa);
    }
    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR, segdstpa);
    }

    /*
     * The algorithm uses the strategy described in PL330 B.3.1.
     * It is extended for 2-byte and 1-byte unaligned cases.
     * */

    /* First see how many byte(s) we need to transfer to get src to be 8 byte aligned */
    if (segsrcpa & 0x7)
    {
        uint32_t aligncount = ALT_MIN(8 - (segsrcpa & 0x7), sizeleft);
        sizeleft -= aligncount;

        dprintf("DMA[M->M][seg]: Total pre-alignment 1-byte burst size transfer(s): %" PRIu32 ".\n", aligncount);

        /* Program in the following parameters:
         *  - SS8   : Source      burst size of 1-byte
         *  - DS8   : Destination burst size of 1-byte
         *  - SBx   : Source      burst length of [aligncount] transfer(s)
         *  - DBx   : Destination burst length of [aligncount] transfer(s)
         *  - SC(7) : Source      cacheable write-back, allocate on reads only
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ((aligncount - 1) << 4) /* SB */
                                              | ALT_DMA_CCR_OPT_SS8
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ((aligncount - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DS8
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
    }

    /* This is the number of 8-byte bursts */
    burstcount = sizeleft >> 3;

    /* If bursting was done and src and dst are not mod-8 congruent, we need to
     * correct for some data left over in the MFIFO due to unaligment issues. */
    correction = (burstcount != 0) && ((segsrcpa & 0x7) != (segdstpa & 0x7));

    /* Update the size left to transfer */
    sizeleft &= 0x7;

    dprintf("DMA[M->M][seg]: Total Main 8-byte burst size transfer(s): %" PRIu32 ".\n", burstcount);

    /* Determine how many 16 length bursts can be done */

    if (burstcount >> 4)
    {
        uint32_t length16burstcount = burstcount >> 4;
        burstcount &= 0xf;

        dprintf("DMA[M->M][seg]:   Number of 16 burst length 8-byte transfer(s): %" PRIu32 ".\n", length16burstcount);
        dprintf("DMA[M->M][seg]:   Number of remaining 8-byte transfer(s):       %" PRIu32 ".\n", burstcount);

        /* Program in the following parameters:
         *  - SS64  : Source      burst size of 8-byte
         *  - DS64  : Destination burst size of 8-byte
         *  - SB16  : Source      burst length of 16 transfers
         *  - DB16  : Destination burst length of 16 transfers
         *  - SC(7) : Source      cacheable write-back, allocate on reads only
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SB16
                                              | ALT_DMA_CCR_OPT_SS64
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ALT_DMA_CCR_OPT_DB16
                                              | ALT_DMA_CCR_OPT_DS64
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        while (length16burstcount > 0)
        {
            uint32_t loopcount = ALT_MIN(length16burstcount, 256);
            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            length16burstcount -= loopcount;

            dprintf("DMA[M->M][seg]:   Looping %" PRIu32 "x 16 burst length 8-byte transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }
    }

    /* At this point, we should have [burstcount] 8-byte transfer(s)
     * remaining. [burstcount] should be less than 16. */

    /* Do one more burst with a SB / DB of length [burstcount]. */

    if (burstcount)
    {
        /* Program in the following parameters:
         *  - SS64  : Source      burst size of 8-byte
         *  - DS64  : Destination burst size of 8-byte
         *  - SBx   : Source      burst length of [burstlength] transfer(s)
         *  - DBx   : Destination burst length of [burstlength] transfer(s)
         *  - SC(7) : Source      cacheable write-back, allocate on reads only
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ((burstcount - 1) << 4) /* SB */
                                              | ALT_DMA_CCR_OPT_SS64
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ((burstcount - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DS64
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
    }

    /* Corrections may be needed if usrc and udst are relatively 8-byte unaligned
     * and bursts were previously used. */

    if (correction)
    {
        if (status == ALT_E_SUCCESS)
        {
            /* This is the number of 1-byte corrections DMAST needed.
             * This is determined by how many remaining data is in the MFIFO after
             * the burst(s) have completed. */
            int correctcount = (segdstpa + (8 - (segsrcpa & 0x7))) & 0x7;

            dprintf("DMA[M->M][seg]: Total correction 1-byte burst size transfer(s): %u.\n", correctcount);

            /* Program in the following parameters:
             *  - SS8   : Source      burst size of 1-byte
             *  - DS8   : Destination burst size of 1-byte
             *  - SBx   : Source      burst length of [correctcount] transfer(s)
             *  - DBx   : Destination burst length of [correctcount] transfer(s)
             *  - SC(7) : Source      cacheable write-back, allocate on reads only
             *  - DC(7) : Destination cacheable write-back, allocate on writes only
             *  - All other options default. */

            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ((correctcount - 1) << 4) /* SB */
                                              | ALT_DMA_CCR_OPT_SS8
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ((correctcount - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DS8
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
    }

    /* At this point, there should be 0 - 7 1-byte transfers remaining. */

    if (sizeleft)
    {
        dprintf("DMA[M->M][seg]: Total post 1-byte burst size transfer(s): %u.\n", sizeleft);

        /* Program in the following parameters:
         *  - SS8   : Source      burst size of 1-byte)
         *  - DS8   : Destination burst size of 1-byte)
         *  - SBx   : Source      burst length of [sizeleft] transfer(s)
         *  - DBx   : Destination burst length of [sizeleft] transfer(s)
         *  - SC(7) : Source      cacheable write-back, allocate on reads only
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ((sizeleft - 1) << 4) /* SB */
                                              | ALT_DMA_CCR_OPT_SS8
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ((sizeleft - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DS8
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
    }

    return status;
}

ALT_STATUS_CODE alt_dma_memory_to_memory(ALT_DMA_CHANNEL_t channel,
                                         ALT_DMA_PROGRAM_t * program,
                                         void * dst,
                                         const void * src,
                                         size_t size,
                                         bool send_evt,
                                         ALT_DMA_EVENT_t evt)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /* If the size is zero, and no event is requested, just return success. */
    if ((size == 0) && (send_evt == false))
    {
        return ALT_E_SUCCESS;
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_init(program);
    }

    if (size != 0)
    {
        ALT_MMU_VA_TO_PA_COALESCE_t coalesce_dst;
        ALT_MMU_VA_TO_PA_COALESCE_t coalesce_src;
        uintptr_t segpa_dst   = 0;
        uintptr_t segpa_src   = 0;
        uint32_t  segsize_dst = 0;
        uint32_t  segsize_src = 0;

        dprintf("DMA[M->M]: dst  = %p.\n",   dst);
        dprintf("DMA[M->M]: src  = %p.\n",   src);
        dprintf("DMA[M->M]: size = 0x%x.\n", size);

        /* Detect if memory regions overshoots the address space.
         * This error checking is handled by the coalescing API. */

        /* Detect if memory regions overlaps. */

        if ((uintptr_t)dst > (uintptr_t)src)
        {
            if ((uintptr_t)src + size - 1 > (uintptr_t)dst)
            {
                return ALT_E_BAD_ARG;
            }
        }
        else
        {
            if ((uintptr_t)dst + size - 1 > (uintptr_t)src)
            {
                return ALT_E_BAD_ARG;
            }
        }

        /*
         * Attempt to coalesce and make the transfer.
         */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_begin(&coalesce_dst, dst, size);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_begin(&coalesce_src, src, size);
        }

        while (size)
        {
            uint32_t segsize;
            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            /*
             * If any of dst or src segments has been completed (or not started), determine its
             * next segment.
             */

            if ((status == ALT_E_SUCCESS) && (segsize_dst == 0))
            {
                status = alt_mmu_va_to_pa_coalesce_next(&coalesce_dst, &segpa_dst, &segsize_dst);

                dprintf("DMA[M->M]: Next dst segment: PA = 0x%x, size = 0x%" PRIx32 ".\n", segpa_dst, segsize_dst);
            }

            if ((status == ALT_E_SUCCESS) && (segsize_src == 0))
            {
                status = alt_mmu_va_to_pa_coalesce_next(&coalesce_src, &segpa_src, &segsize_src);

                dprintf("DMA[M->M]: Next src segment: PA = 0x%x, size = 0x%" PRIx32 ".\n", segpa_src, segsize_src);
            }

            /*
             * Determine the largest segment to safely transfer next.
             * */

            /* This is the largest segment that can safely be transfered considering both dst and
             * src segmentation. Typically dst or src or both segment(s) will complete. */
            segsize = ALT_MIN(segsize_dst, segsize_src);

            /*
             * Transfer the largest safe segment.
             * */

            if (status == ALT_E_SUCCESS)
            {
                dprintf("DMA[M->M]: Transfering safe size = 0x%" PRIx32 ".\n", segsize);

                status = alt_dma_memory_to_memory_segment(program, segpa_dst, segpa_src, segsize);
            }

            /*
             * Update some bookkeeping.
             * */

            segsize_dst -= segsize;
            segsize_src -= segsize;

            if (segsize_dst)
            {
                segpa_dst += segsize;

                dprintf("DMA[M->M]: Updated dst segment: PA = 0x%x, size = 0x%" PRIx32 ".\n", segpa_dst, segsize_dst);
            }

            if (segsize_src)
            {
                segpa_src += segsize;

                dprintf("DMA[M->M]: Updated src segment: PA = 0x%x, size = 0x%" PRIx32 ".\n", segpa_src, segsize_src);
            }

            /* Use ALT_MIN() to assuredly prevent infinite loop. If either the dst or src has not yet
             * completed, coalesce_end() will catch that logical error. */
            size -= ALT_MIN(segsize, size);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_end(&coalesce_dst);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_end(&coalesce_src);
        }

    } /* if (size != 0) */

    /* Send event if requested. */
    if (send_evt)
    {
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAWMB(program);
        }

        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[M->M]: Adding event ...\n");
            status = alt_dma_program_DMASEV(program, evt);
        }
    }

    /* Now that everything is done, end the program. */
    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAEND(program);
    }

    /* If there was a problem assembling the program, clean up the buffer and exit. */
    if (status != ALT_E_SUCCESS)
    {
        /* Do not report the status for the clear operation. A failure should be
         * reported regardless of if the clear is successful. */
        alt_dma_program_clear(program);
        return status;
    }

    /* Execute the program on the given channel. */
    return alt_dma_channel_exec(channel, program);
}

static ALT_STATUS_CODE alt_dma_zero_to_memory_segment(ALT_DMA_PROGRAM_t * program,
                                                      uintptr_t segbufpa,
                                                      size_t segsize)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    size_t sizeleft = segsize;
    uint32_t burstcount;
    uint32_t loop0;
    uint32_t loop1;

    dprintf("DMA[Z->M][seg]: buf  = 0x%x (PA).\n", segbufpa);
    dprintf("DMA[Z->M][seg]: size = 0x%x.\n",      segsize);

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR, segbufpa);
    }

    /* First see how many byte(s) we need to transfer to get dst to be 8 byte aligned. */
    if (segbufpa & 0x7)
    {
        uint32_t aligncount = ALT_MIN(8 - (segbufpa & 0x7), sizeleft);
        sizeleft -= aligncount;

        dprintf("DMA[Z->M][seg]: Total pre-alignment 1-byte burst size transfer(s): %" PRIu32 ".\n", aligncount);

        /* Program in the following parameters:
         *  - DS8   : Destination burst size of 1-byte
         *  - DBx   : Destination burst length of [aligncount] transfer(s)
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SB_DEFAULT
                                              | ALT_DMA_CCR_OPT_SS_DEFAULT
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ((aligncount - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DS8
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMASTZ(program);
        }
    }

    /* This is the number of 8-byte bursts left */
    burstcount = sizeleft >> 3;

    /* Update the size left to transfer */
    sizeleft &= 0x7;

    dprintf("DMA[Z->M][seg]: Total Main 8-byte burst size transfer(s): %" PRIu32 ".\n", burstcount);

    /* Determine how many 16 length bursts can be done */
    if (burstcount >> 4)
    {
        uint32_t length16burstcount = burstcount >> 4;
        burstcount &= 0xf;

        dprintf("DMA[Z->M][seg]:   Number of 16 burst length 8-byte transfer(s): %" PRIu32 ".\n", length16burstcount);
        dprintf("DMA[Z->M][seg]:   Number of remaining 8-byte transfer(s):       %" PRIu32 ".\n", burstcount);

        /* Program in the following parameters:
         *  - DS64  : Destination burst size of 8-byte
         *  - DB16  : Destination burst length of 16 transfers
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SB_DEFAULT
                                              | ALT_DMA_CCR_OPT_SS_DEFAULT
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ALT_DMA_CCR_OPT_DB16
                                              | ALT_DMA_CCR_OPT_DS64
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        while (length16burstcount > 0)
        {

            if (length16burstcount > 256) {
              loop0 = ALT_MIN(length16burstcount/256,256);
              loop0 = (loop0) ? loop0 : 1;
              loop1 = 256;
            } else {
              loop0 = 1;
              loop1 = length16burstcount;
            }

            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            length16burstcount -= loop0 * loop1;

            dprintf("DMA[Z->M][seg]:   Looping %" PRIu32 "x 16 burst length 8-byte transfer(s).\n", loop0*loop1);

            if ((status == ALT_E_SUCCESS) && (loop0 > 1))
            {
                status = alt_dma_program_DMALP(program, loop0);
            }
            if ((status == ALT_E_SUCCESS) && (loop1 > 1))
            {
                status = alt_dma_program_DMALP(program, loop1);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMASTZ(program);
            }
            if ((status == ALT_E_SUCCESS) && (loop1 > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
            if ((status == ALT_E_SUCCESS) && (loop0 > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }
    }

    /* At this point, we should have [burstcount] 8-byte transfer(s)
     * remaining. [burstcount] should be less than 16. */

    /* Do one more burst with a SB / DB of length [burstcount]. */

    if (burstcount)
    {
        /* Program in the following parameters:
         *  - DS64  : Destination burst size of 8-byte
         *  - DBx   : Destination burst length of [burstlength] transfer(s)
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SB_DEFAULT
                                              | ALT_DMA_CCR_OPT_SS_DEFAULT
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ((burstcount - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DS64
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMASTZ(program);
        }
    }

    /* At this point, there should be 0 - 7 1-byte transfers remaining. */

    if (sizeleft)
    {
        dprintf("DMA[Z->M][seg]: Total post 1-byte burst size transfer(s): %u.\n", sizeleft);

        /* Program in the following parameters:
         *  - DS8   : Destination burst size of 1-byte
         *  - DBx   : Destination burst length of [sizeleft] transfer(s)
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SB_DEFAULT
                                              | ALT_DMA_CCR_OPT_SS_DEFAULT
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ((sizeleft - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DS8
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMASTZ(program);
        }
    }

    return status;
}

ALT_STATUS_CODE alt_dma_zero_to_memory(ALT_DMA_CHANNEL_t channel,
                                       ALT_DMA_PROGRAM_t * program,
                                       void * buf,
                                       size_t size,
                                       bool send_evt,
                                       ALT_DMA_EVENT_t evt)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /* If the size is zero, and no event is requested, just return success. */
    if ((size == 0) && (send_evt == false))
    {
        return ALT_E_SUCCESS;
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_init(program);
    }

    if (size != 0)
    {
        /* Detect if memory region overshoots the address space.
         * This error checking is handled by the coalescing API. */

        /*
         * Attempt to coalesce and make the transfer.
         * */

        ALT_MMU_VA_TO_PA_COALESCE_t coalesce;

        dprintf("DMA[Z->M]: buf  = %p (VA).\n", buf);
        dprintf("DMA[Z->M]: size = 0x%x.\n",    size);

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, buf, size);
        }

        while (size)
        {
            /*
             * Extract the next segment.
             * */

            uintptr_t segpa   = 0;
            uint32_t  segsize = 0;

            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);

            size -= segsize;
            dprintf("DMA[Z->M]: Next segment PA = 0x%x, size = 0x%" PRIx32 "; remaining = 0x%x.\n", segpa, segsize, size);

            /*
             * Zero out the current segment.
             * */

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_zero_to_memory_segment(program,
                                                        segpa,
                                                        segsize);
            }
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
        }
    } /* if (size != 0) */

    /* Send event if requested. */
    if (send_evt)
    {
        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[Z->M]: Adding event ...\n");
            status = alt_dma_program_DMASEV(program, evt);
        }
    }

    /* Now that everything is done, end the program. */
    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAEND(program);
    }

    /* If there was a problem assembling the program, clean up the buffer and exit. */
    if (status != ALT_E_SUCCESS)
    {
        /* Do not report the status for the clear operation. A failure should be
         * reported regardless of if the clear is successful. */
        alt_dma_program_clear(program);
        return status;
    }

    /* Execute the program on the given channel. */
    return alt_dma_channel_exec(channel, program);
}

static ALT_STATUS_CODE alt_dma_memory_to_register_segment(ALT_DMA_CHANNEL_t channel,
                                                          ALT_DMA_PROGRAM_t * program,
                                                          uint32_t ccr_ss_ds_mask,
                                                          uintptr_t segsrcpa,
                                                          size_t count)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    /* This is the remaining count left to process. */
    uint32_t countleft = count;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR, segsrcpa);
    }

    /* See how many 16-length bursts we can use */
    if (countleft >> 4)
    {
        uint32_t length16burst;

        /* Program in the following parameters:
         *  - SSx   : Source      burst size of [ccr_ss_ds_mask]
         *  - DSx   : Destination burst size of [ccr_ss_ds_mask]
         *  - DAF   : Destination address fixed
         *  - SB16  : Source      burst length of 16 transfers
         *  - DB16  : Destination burst length of 16 transfers
         *  - SC(7) : Source      cacheable write-back, allocate on reads only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ccr_ss_ds_mask
                                              | ALT_DMA_CCR_OPT_SB16
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ALT_DMA_CCR_OPT_DB16
                                              | ALT_DMA_CCR_OPT_DAF
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC_DEFAULT
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        length16burst = countleft >> 4;
        countleft &= 0xf;

        dprintf("DMA[M->R][seg]:   Number of 16 burst length transfer(s): %" PRIu32 ".\n", length16burst);
        dprintf("DMA[M->R][seg]:   Number of remaining transfer(s):       %" PRIu32 ".\n", countleft);

        /* See how many 256x 16-length bursts we can use */
        if (length16burst >> 8)
        {
            uint32_t loop256length16burst = length16burst >> 8;
            length16burst &= ((1 << 8) - 1);

            dprintf("DMA[M->R][seg]:     Number of 256-looped 16 burst length transfer(s): %" PRIu32 ".\n", loop256length16burst);
            dprintf("DMA[M->R][seg]:     Number of remaining 16 burst length transfer(s):  %" PRIu32 ".\n", length16burst);

            while (loop256length16burst > 0)
            {
                uint32_t loopcount = ALT_MIN(loop256length16burst, 256);
                loop256length16burst -= loopcount;

                if (status != ALT_E_SUCCESS)
                {
                    break;
                }

                dprintf("DMA[M->R][seg]:     Looping %" PRIu32 "x super loop transfer(s).\n", loopcount);

                if ((status == ALT_E_SUCCESS) && (loopcount > 1))
                {
                    status = alt_dma_program_DMALP(program, loopcount);
                }

                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALP(program, 256);
                }
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }

                if ((status == ALT_E_SUCCESS) && (loopcount > 1))
                {
                    status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }
            }
        }

        /* The super loop above ensures that the length16burst is below 256. */
        if (length16burst > 0)
        {
            uint32_t loopcount = length16burst;
            length16burst = 0;

            dprintf("DMA[M->R][seg]:   Looping %" PRIu32 "x 16 burst length transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }
    }

    /* At this point, we should have [countleft] transfer(s) remaining.
     * [countleft] should be less than 16. */

    if (countleft)
    {
        /* Program in the following parameters:
         *  - SSx   : Source      burst size of [ccr_ss_ds_mask]
         *  - DSx   : Destination burst size of [ccr_ss_ds_mask]
         *  - DAF   : Destination address fixed
         *  - SBx   : Source      burst length of [countleft] transfer(s)
         *  - DBx   : Destination burst length of [countleft] transfer(s)
         *  - SC(7) : Source      cacheable write-back, allocate on reads only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[M->R][seg]:   Tail end %" PRIu32 "x transfer(s).\n", countleft);

            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ccr_ss_ds_mask
                                              | ((countleft - 1) << 4) /* SB */
                                              | ALT_DMA_CCR_OPT_SA_DEFAULT
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ((countleft - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DAF
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC_DEFAULT
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
    }

    return status;
}

ALT_STATUS_CODE alt_dma_memory_to_register(ALT_DMA_CHANNEL_t channel,
                                           ALT_DMA_PROGRAM_t * program,
                                           void * dst_reg,
                                           const void * src_buf,
                                           size_t count,
                                           uint32_t register_width_bits,
                                           bool send_evt,
                                           ALT_DMA_EVENT_t evt)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /* If the count is zero, and no event is requested, just return success. */
    if ((count == 0) && (send_evt == false))
    {
        return ALT_E_SUCCESS;
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_init(program);
    }

    if (count != 0)
    {
        /* Verify valid register_width_bits and construct the CCR SS and DS parameters. */
        uint32_t ccr_ss_ds_mask = 0;
        uint32_t reg_width_bytes_log2 = 0;
        size_t size;
        ALT_MMU_VA_TO_PA_COALESCE_t coalesce;

        if (status == ALT_E_SUCCESS)
        {
            switch (register_width_bits)
            {
            case 8:
                /* Program in the following parameters:
                 *  - SS8 (Source      burst size of 8 bits)
                 *  - DS8 (Destination burst size of 8 bits) */
                ccr_ss_ds_mask = ALT_DMA_CCR_OPT_SS8 | ALT_DMA_CCR_OPT_DS8;
                reg_width_bytes_log2 = 0;
                break;
            case 16:
                /* Program in the following parameters:
                 *  - SS16 (Source      burst size of 16 bits)
                 *  - DS16 (Destination burst size of 16 bits) */
                ccr_ss_ds_mask = ALT_DMA_CCR_OPT_SS16 | ALT_DMA_CCR_OPT_DS16;
                reg_width_bytes_log2 = 1;
                break;
            case 32:
                /* Program in the following parameters:
                 *  - SS32 (Source      burst size of 32 bits)
                 *  - DS32 (Destination burst size of 32 bits) */
                ccr_ss_ds_mask = ALT_DMA_CCR_OPT_SS32 | ALT_DMA_CCR_OPT_DS32;
                reg_width_bytes_log2 = 2;
                break;
            case 64:
                /* Program in the following parameters:
                 *  - SS64 (Source      burst size of 64 bits)
                 *  - DS64 (Destination burst size of 64 bits) */
                ccr_ss_ds_mask = ALT_DMA_CCR_OPT_SS64 | ALT_DMA_CCR_OPT_DS64;
                reg_width_bytes_log2 = 3;
                break;
            default:
                status = ALT_E_BAD_ARG;
                break;
            }
        }

        /* Detect if memory region overshoots address space
         * This error checking is handled by the coalescing API. */

        /* Verify that the dst_reg and src_buf are aligned to the register width */
        if (status == ALT_E_SUCCESS)
        {
            if      (((uintptr_t)dst_reg & ((1 << reg_width_bytes_log2) - 1)) != 0)
            {
                status = ALT_E_BAD_ARG;
            }
            else if (((uintptr_t)src_buf & ((1 << reg_width_bytes_log2) - 1)) != 0)
            {
                status = ALT_E_BAD_ARG;
            }
            else
            {
                dprintf("DMA[M->R]: dst_reg = %p.\n",   dst_reg);
                dprintf("DMA[M->R]: src_buf = %p.\n",   src_buf);
                dprintf("DMA[M->R]: count   = 0x%x.\n", count);
            }
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR, (uint32_t)dst_reg);
        }

        /*
         * The register operations are slightly more complicated because it
         * counts everything in register transfers, and not bytes.
         * */

        size = count * (1 << reg_width_bytes_log2);

        /*
         * Attempt to coalesce and make the transfer.
         * */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, src_buf, size);
        }

        while (size)
        {
            /*
             * Extract the next segment.
             */

            uintptr_t segpa   = 0;
            uint32_t  segsize = 0;

            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);
            }

            size -= segsize;
            dprintf("DMA[M->R]: Next segment PA = 0x%x, count = 0x%" PRIx32 "; remaining = 0x%x.\n", segpa, segsize >> reg_width_bytes_log2, size >> reg_width_bytes_log2);

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_memory_to_register_segment(channel,
                                                            program,
                                                            ccr_ss_ds_mask,
                                                            segpa,
                                                            segsize >> reg_width_bytes_log2);
            }
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
        }

    } /* if (count != 0) */

    /* Send event if requested. */
    if (send_evt)
    {
        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[M->R]: Adding event ...\n");
            status = alt_dma_program_DMASEV(program, evt);
        }
    }

    /* Now that everything is done, end the program. */
    if (status == ALT_E_SUCCESS)
    {
        dprintf("DMA[M->R]: DMAEND program.\n");
        status = alt_dma_program_DMAEND(program);
    }

    /* If there was a problem assembling the program, clean up the buffer and exit. */
    if (status != ALT_E_SUCCESS)
    {
        /* Do not report the status for the clear operation. A failure should be
         * reported regardless of if the clear is successful. */
        alt_dma_program_clear(program);
        return status;
    }

    /* Execute the program on the given channel. */
    return alt_dma_channel_exec(channel, program);
}

static ALT_STATUS_CODE alt_dma_register_to_memory_segment(ALT_DMA_CHANNEL_t channel,
                                                          ALT_DMA_PROGRAM_t * program,
                                                          uint32_t ccr_ss_ds_mask,
                                                          uintptr_t segdstpa,
                                                          size_t count)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /* This is the remaining count left to process. */
    uint32_t countleft = count;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR, segdstpa);
    }

    /* See how many 16-length bursts we can use */
    if (countleft >> 4)
    {
        uint32_t length16burst = countleft >> 4;
        countleft &= 0xf;

        dprintf("DMA[R->M][seg]:   Number of 16 burst length transfer(s): %" PRIu32 ".\n", length16burst);
        dprintf("DMA[R->M][seg]:   Number of remaining transfer(s):       %" PRIu32 ".\n", countleft);

        /*
         * The algorithm uses the strategy described in PL330 B.2.3.
         * Not sure if registers will accept burst transfers so read the register in its own transfer.
         * */

        /* Program in the following parameters:
         *  - SAF   : Source      address fixed
         *  - SSx   : Source      burst size of [ccr_ss_ds_mask]
         *  - DSx   : Destination burst size of [ccr_ss_ds_mask]
         *  - SB16  : Source      burst length of 16 transfers
         *  - DB16  : Destination burst length of 16 transfers
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ccr_ss_ds_mask
                                              | ALT_DMA_CCR_OPT_SB16
                                              | ALT_DMA_CCR_OPT_SAF
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ALT_DMA_CCR_OPT_DB16
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        /* See how many 256x 16-length bursts we can do */
        if (length16burst >> 8)
        {
            uint32_t loop256length16burst = length16burst >> 8;
            length16burst &= ((1 << 8) - 1);

            dprintf("DMA[R->M][seg]:     Number of 256-looped 16 burst length transfer(s): %" PRIu32 ".\n", loop256length16burst);
            dprintf("DMA[R->M][seg]:     Number of remaining 16 burst length transfer(s):  %" PRIu32 ".\n", length16burst);

            while (loop256length16burst > 0)
            {
                uint32_t loopcount = ALT_MIN(loop256length16burst, 256);
                loop256length16burst -= loopcount;

                if (status != ALT_E_SUCCESS)
                {
                    break;
                }

                dprintf("DMA[R->M][seg]:     Looping %" PRIu32 "x super loop transfer(s).\n", loopcount);

                if ((status == ALT_E_SUCCESS) && (loopcount > 1))
                {
                    status = alt_dma_program_DMALP(program, loopcount);
                }

                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALP(program, 256);
                }
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }

                if ((status == ALT_E_SUCCESS) && (loopcount > 1))
                {
                    status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }
            }
        }

        /* The super loop above ensures that the length16burst is below 256. */
        if (length16burst > 0)
        {
            uint32_t loopcount = length16burst;
            length16burst = 0;

            dprintf("DMA[R->M][seg]:   Looping %" PRIu32 "x 16 burst length transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }
    }

    /* At this point, we should have [countleft] transfer(s) remaining.
     * [countleft] should be less than 16. */

    if (countleft)
    {
        dprintf("DMA[R->M][seg]:   Tail end %" PRIu32 "x transfer(s).\n", countleft);

        /* Program in the following parameters:
         *  - SAF   : Source      address fixed
         *  - SSx   : Source      burst size of [ccr_ss_ds_mask]
         *  - DSx   : Destination burst size of [ccr_ss_ds_mask]
         *  - SBx   : Source      burst length of [countleft] transfer(s)
         *  - DBx   : Destination burst length of [countleft] transfer(s)
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other options default. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ccr_ss_ds_mask
                                              | ((countleft - 1) << 4) /* SB */
                                              | ALT_DMA_CCR_OPT_SAF
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ((countleft - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DA_DEFAULT
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
    }

    return status;
}

ALT_STATUS_CODE alt_dma_register_to_memory(ALT_DMA_CHANNEL_t channel,
                                           ALT_DMA_PROGRAM_t * program,
                                           void * dst_buf,
                                           const void * src_reg,
                                           size_t count,
                                           uint32_t register_width_bits,
                                           bool send_evt,
                                           ALT_DMA_EVENT_t evt)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /* If the count is zero, and no event is requested, just return success. */
    if ((count == 0) && (send_evt == false))
    {
        return ALT_E_SUCCESS;
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_init(program);
    }

    if (count != 0)
    {
        /* Verify valid register_width_bits and construct the CCR SS and DS parameters. */
        ALT_MMU_VA_TO_PA_COALESCE_t coalesce;
        uint32_t ccr_ss_ds_mask = 0;
        uint32_t reg_width_bytes_log2 = 0;
        size_t size;

        if (status == ALT_E_SUCCESS)
        {
            switch (register_width_bits)
            {
            case 8:
                /* Program in the following parameters:
                 *  - SS8 (Source      burst size of 8 bits)
                 *  - DS8 (Destination burst size of 8 bits) */
                ccr_ss_ds_mask = ALT_DMA_CCR_OPT_SS8 | ALT_DMA_CCR_OPT_DS8;
                reg_width_bytes_log2 = 0;
                break;
            case 16:
                /* Program in the following parameters:
                 *  - SS16 (Source      burst size of 16 bits)
                 *  - DS16 (Destination burst size of 16 bits) */
                ccr_ss_ds_mask = ALT_DMA_CCR_OPT_SS16 | ALT_DMA_CCR_OPT_DS16;
                reg_width_bytes_log2 = 1;
                break;
            case 32:
                /* Program in the following parameters:
                 *  - SS32 (Source      burst size of 32 bits)
                 *  - DS32 (Destination burst size of 32 bits) */
                ccr_ss_ds_mask = ALT_DMA_CCR_OPT_SS32 | ALT_DMA_CCR_OPT_DS32;
                reg_width_bytes_log2 = 2;
                break;
            case 64:
                /* Program in the following parameters:
                 *  - SS64 (Source      burst size of 64 bits)
                 *  - DS64 (Destination burst size of 64 bits) */
                ccr_ss_ds_mask = ALT_DMA_CCR_OPT_SS64 | ALT_DMA_CCR_OPT_DS64;
                reg_width_bytes_log2 = 3;
                break;
            default:
                dprintf("DMA[R->M]: Invalid register width.\n");
                status = ALT_E_BAD_ARG;
                break;
            }
        }

        /* Detect if memory region overshoots address space
         * This error checking is handled by the coalescing API. */

        /* Verify that the dst_buf and src_reg are aligned to the register width */
        if (status == ALT_E_SUCCESS)
        {
            if      (((uintptr_t)dst_buf & ((1 << reg_width_bytes_log2) - 1)) != 0)
            {
                status = ALT_E_BAD_ARG;
            }
            else if (((uintptr_t)src_reg & ((1 << reg_width_bytes_log2) - 1)) != 0)
            {
                status = ALT_E_BAD_ARG;
            }
            else
            {
                dprintf("DMA[R->M]: dst_reg = %p.\n",   dst_buf);
                dprintf("DMA[R->M]: src_buf = %p.\n",   src_reg);
                dprintf("DMA[R->M]: count   = 0x%x.\n", count);
            }
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR, (uint32_t)src_reg);
        }

        /*
         * The register operations are slightly more complicated because it
         * counts everything in register transfers, and not bytes.
         * */

        size = count * (1 << reg_width_bytes_log2);

        /*
         * Attempt to coalesce and make the transfer.
        */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, dst_buf, size);
        }

        while (size)
        {
            /*
             * Extract the next segment.
            */

            uintptr_t segpa   = 0;
            uint32_t  segsize = 0;

            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);
            }

            size -= segsize;
            dprintf("DMA[R->M]: Next segment PA = 0x%x, count = 0x%" PRIx32 "; remaining = 0x%x.\n", segpa, segsize >> reg_width_bytes_log2, size >> reg_width_bytes_log2);

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_register_to_memory_segment(channel,
                                                            program,
                                                            ccr_ss_ds_mask,
                                                            segpa,
                                                            segsize >> reg_width_bytes_log2);
            }
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
        }

    } /* if (count != 0) */

    /* Send event if requested. */
    if (send_evt)
    {
        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[R->M]: Adding event ...\n");
            status = alt_dma_program_DMASEV(program, evt);
        }
    }

    /* Now that everything is done, end the program. */
    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAEND(program);
    }

    /* If there was a problem assembling the program, clean up the buffer and exit. */
    if (status != ALT_E_SUCCESS)
    {
        /* Do not report the status for the clear operation. A failure should be
         * reported regardless of if the clear is successful. */
        alt_dma_program_clear(program);
        return status;
    }

    /* Execute the program on the given channel. */
    return alt_dma_channel_exec(channel, program);
}

#if ALT_DMA_PERIPH_PROVISION_I2C_SUPPORT
/*
 * These are here to break the dependence on including alt_i2c.c in HWLibs projects.
*/

__attribute__((weak)) ALT_STATUS_CODE alt_i2c_op_mode_get(ALT_I2C_DEV_t *i2c_dev,
                                                          ALT_I2C_MODE_t* mode)
{
    uint32_t cfg_register = alt_read_word(ALT_I2C_CON_ADDR(i2c_dev->location));
    uint32_t mst_mod_stat = ALT_I2C_CON_MST_MOD_GET(cfg_register);

    *mode = (ALT_I2C_MODE_t)mst_mod_stat;

    return ALT_E_SUCCESS;
}

static ALT_STATUS_CODE alt_dma_memory_to_i2c_master_custom(ALT_DMA_PROGRAM_t * program,
                                                           ALT_DMA_PERIPH_t periph,
                                                           ALT_I2C_DEV_t * i2c_dev,
                                                           uintptr_t write_info_pa,
                                                           size_t request_count,
                                                           bool first)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    dprintf("DMA[M->P][I2C][master][custom]: Create %u custom write(s) [first = %s].\n", request_count, (first ? "true" : "false"));

    if ((status == ALT_E_SUCCESS) && (first))
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR,
                                        (uintptr_t)ALT_I2C_DATA_CMD_ADDR(i2c_dev->location));
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR,
                                        write_info_pa);
    }

    if ((status == ALT_E_SUCCESS) && (first))
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                        (   ALT_DMA_CCR_OPT_SAF
                                          | ALT_DMA_CCR_OPT_SS32
                                          | ALT_DMA_CCR_OPT_SB1
                                          | ALT_DMA_CCR_OPT_SP_DEFAULT
                                          | ALT_DMA_CCR_OPT_SC(7)
                                          | ALT_DMA_CCR_OPT_DAF
                                          | ALT_DMA_CCR_OPT_DS32
                                          | ALT_DMA_CCR_OPT_DB1
                                          | ALT_DMA_CCR_OPT_DP_DEFAULT
                                          | ALT_DMA_CCR_OPT_DC_DEFAULT
                                          | ALT_DMA_CCR_OPT_ES_DEFAULT
                                        )
            );
    }

    while (request_count > 0)
    {
        uint32_t loopcount = ALT_MIN(request_count, 256);
        request_count -= loopcount;

        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        dprintf("DMA[M->P][I2C][master][custom][S]: Creating %" PRIu32 " single-type transfer(s).\n", loopcount);

        if ((status == ALT_E_SUCCESS) && (loopcount > 1))
        {
            status = alt_dma_program_DMALP(program, loopcount);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAFLUSHP(program, periph);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAWFP(program, periph, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }

        if ((status == ALT_E_SUCCESS) && (loopcount > 1))
        {
            status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
        }
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_memory_to_i2c_common_segment(ALT_DMA_PROGRAM_t * program,
                                                            ALT_DMA_PERIPH_t periph,
                                                            uint8_t threshold,
                                                            uintptr_t segsrcpa,
                                                            size_t segsize)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    uint32_t burst_size   = 0;
    uint32_t burst_count  = 0;
    uint32_t single_count = 0;

    if (status == ALT_E_SUCCESS)
    {
        size_t size_left = segsize;

        if (threshold > 1)
        {
            ldiv_t rslt;
            if (threshold > 16)
            {
                threshold = 16;
            }

            burst_size = threshold;

            rslt = ldiv(size_left, burst_size);
            burst_count = rslt.quot;
            size_left   = rslt.rem;
        }

        single_count = size_left;
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR,
                                        segsrcpa);
    }

    if (burst_count)
    {
        dprintf("DMA[M->P][I2C][common][seg][B]: Burst size = %" PRIu32 " bytes, count = %" PRIu32 ".\n", burst_size, burst_count);

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SAI
                                              | ALT_DMA_CCR_OPT_SS8
                                              | ((burst_size - 1) << 4)
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ALT_DMA_CCR_OPT_DAF
                                              | ALT_DMA_CCR_OPT_DS8
                                              | ALT_DMA_CCR_OPT_DB1
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC_DEFAULT
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        while (burst_count > 0)
        {
            uint32_t loopcount = ALT_MIN(burst_count, 256);
            int i;

            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            burst_count -= loopcount;
            dprintf("DMA[M->P][I2C][common][seg][B]: Creating %" PRIu32 " burst-type transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAFLUSHP(program, periph);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAWFP(program, periph, ALT_DMA_PROGRAM_INST_MOD_BURST);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
            }

            for (i = 0; i < burst_size; ++i)
            {
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
                }
            }

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }
    }

    if (single_count)
    {
        dprintf("DMA[M->P][I2C][common][seg][S]: Single size = 1 byte, count = %" PRIu32 ".\n", single_count);

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SAI
                                              | ALT_DMA_CCR_OPT_SS8
                                              | ALT_DMA_CCR_OPT_SB1
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ALT_DMA_CCR_OPT_DAF
                                              | ALT_DMA_CCR_OPT_DS8
                                              | ALT_DMA_CCR_OPT_DB1
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC_DEFAULT
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        while (single_count > 0)
        {
            uint32_t loopcount = ALT_MIN(single_count, 256);
            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            single_count -= loopcount;

            dprintf("DMA[M->P][I2C][common][seg][S]: Creating %" PRIu32 " single-type transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAFLUSHP(program, periph);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAWFP(program, periph, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
            }

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_memory_to_i2c_common(ALT_DMA_PROGRAM_t * program,
                                                    ALT_DMA_PERIPH_t periph,
                                                    ALT_DMA_PERIPH_INFO_I2C_t * i2c_info,
                                                    const char * src,
                                                    size_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_MMU_VA_TO_PA_COALESCE_t coalesce;


    /* Use the I2C register interface to avoid coupling I2C and DMA. */
    uint8_t threshold = ALT_I2C_TX_TL_TX_TL_GET(alt_read_word(ALT_I2C_TX_TL_ADDR(i2c_info->i2c_dev->location))) + 1;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR,
                                        (uintptr_t)ALT_I2C_DATA_CMD_ADDR(i2c_info->i2c_dev->location));
    }

    /*
     * Attempt to coalesce and make the transfer.
    */

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, src, size);
    }

    while (size)
    {
        /*
         * Extract the next segment;
        */

        uintptr_t segpa   = 0;
        uint32_t  segsize = 0;

        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);

        size -= segsize;
        dprintf("DMA[M->P][I2C][common]: Next segment PA = 0x%x, size = 0x%" PRIx32 "; remaining = 0x%x\n", segpa, segsize, size);

        /*
         * Transfer out the current segment.
        */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_memory_to_i2c_common_segment(program,
                                                          periph,
                                                          threshold,
                                                          segpa,
                                                          segsize);
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_memory_to_i2c_master(ALT_DMA_PROGRAM_t * program,
                                                    ALT_DMA_PERIPH_t periph,
                                                    ALT_DMA_PERIPH_INFO_I2C_t * i2c_info,
                                                    const char * src,
                                                    size_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    uintptr_t scratchpa;

    if (status == ALT_E_SUCCESS)
    {
        uint32_t dfsr;
        uint32_t seglength;
        scratchpa = alt_mmu_va_to_pa(i2c_info->scratch, &seglength, &dfsr);
        if (dfsr)
        {
            printf("DMA[M->P][I2C]: Error: Cannot get VA-to-PA of scratch buffer.\n");
            status = ALT_E_ERROR;
        }
    }

    if (size == 1)
    {
        i2c_info->scratch[3] = ALT_I2C_DATA_CMD_DAT_SET(*src)                        |
                               ALT_I2C_DATA_CMD_RESTART_SET(i2c_info->issue_restart) |
                               ALT_I2C_DATA_CMD_STOP_SET(i2c_info->issue_stop);

        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[M->P][I2C][master]: Custom transfer (single).\n");

            status = alt_dma_memory_to_i2c_master_custom(program,
                                                         periph,
                                                         i2c_info->i2c_dev,
                                                         scratchpa + (sizeof(uint32_t) * 3), /* &i2c_info->scratch[3], */
                                                         1,
                                                         true);
        }
    }
    else
    {
        /* RESTART and STOP decreases the effective size as the first and/or last bytes requires custom
         * handling. */

        if (i2c_info->issue_stop)
        {
            /* Process the STOP first because the RESTART may alter the src pointer. */

            i2c_info->scratch[2] = ALT_I2C_DATA_CMD_DAT_SET(src[size - 1]) |
                                   ALT_I2C_DATA_CMD_RESTART_SET(false)     |
                                   ALT_I2C_DATA_CMD_STOP_SET(true);

            --size;
        }

        if (i2c_info->issue_restart)
        {
            i2c_info->scratch[1] = ALT_I2C_DATA_CMD_DAT_SET(src[0])   |
                                   ALT_I2C_DATA_CMD_RESTART_SET(true) |
                                   ALT_I2C_DATA_CMD_STOP_SET(false);

            --size;

            if (status == ALT_E_SUCCESS)
            {
                dprintf("DMA[M->P][I2C][master]: Custom transfer (restart).\n");

                status = alt_dma_memory_to_i2c_master_custom(program,
                                                             periph,
                                                             i2c_info->i2c_dev,
                                                             scratchpa + (sizeof(uint32_t) * 1), /* &i2c_info->scratch[1], */
                                                             1,
                                                             true);

                /* Increment the src here so the single/burst transfers will avoid the first item. */
                ++src;
            }
        }

        /* Transfer non-{RESTART,STOP} of the data. */

        if ((status == ALT_E_SUCCESS) && (size > 0))
        {
            status = alt_dma_memory_to_i2c_common(program,
                                                  periph,
                                                  i2c_info,
                                                  src,
                                                  size);
        }

        if (i2c_info->issue_stop)
        {
            if (status == ALT_E_SUCCESS)
            {
                dprintf("DMA[M->P][I2C][master]: Custom transfer (stop).\n");

                status = alt_dma_memory_to_i2c_master_custom(program,
                                                             periph,
                                                             i2c_info->i2c_dev,
                                                             scratchpa + (sizeof(uint32_t) * 2), /* &i2c_info->scratch[2], */
                                                             1,
                                                             (size != 0));
            }
        }
    }

    /* Ensure that the scratch data is in SDRAM for the DMAC to read. */
    if (status == ALT_E_SUCCESS)
    {
        /* Sync the periph scratch information to RAM. */
        void * vaddr  = (void *)((uintptr_t)(i2c_info->scratch) & ~(ALT_CACHE_LINE_SIZE - 1));
        void * vend   = (void *)(((uintptr_t)(i2c_info->scratch) + sizeof(i2c_info->scratch) + (ALT_CACHE_LINE_SIZE - 1)) & ~(ALT_CACHE_LINE_SIZE - 1));
        size_t length = (uintptr_t)vend - (uintptr_t)vaddr;

        status = alt_cache_system_clean(vaddr, length);
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_memory_to_i2c(ALT_DMA_PROGRAM_t * program,
                                             ALT_DMA_PERIPH_t periph,
                                             ALT_DMA_PERIPH_INFO_I2C_t * i2c_info,
                                             const void * src,
                                             size_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_I2C_MODE_t mode;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_i2c_op_mode_get(i2c_info->i2c_dev, &mode);
    }

    if (status == ALT_E_SUCCESS)
    {
        if (mode == ALT_I2C_MODE_MASTER)
        {
            dprintf("DMA[M->P][I2C]: Controller detected as Master.\n");

            status = alt_dma_memory_to_i2c_master(program,
                                                  periph,
                                                  i2c_info,
                                                  src,
                                                  size);

        }
        else
        {
            dprintf("DMA[M->P][I2C]: Controller detected as Slave.\n");

            status = alt_dma_memory_to_i2c_common(program,
                                                  periph,
                                                  i2c_info,
                                                  src,
                                                  size);
        }
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_i2c_to_memory_common_segment(ALT_DMA_PROGRAM_t * program,
                                                            ALT_DMA_PERIPH_t periph,
                                                            uint8_t threshold,
                                                            uintptr_t segdstpa,
                                                            size_t segsize)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    uint32_t burst_size   = 0;
    uint32_t burst_count  = 0;
    uint32_t single_count = 0;

    if (status == ALT_E_SUCCESS)
    {
        size_t size_left = segsize;

        if (threshold > 1)
        {
            ldiv_t rslt;
            if (threshold > 16)
            {
                threshold = 16;
            }

            burst_size = threshold;

            rslt = ldiv(size_left, burst_size);
            burst_count = rslt.quot;
            size_left   = rslt.rem;
        }

        single_count = size_left;
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR,
                                        segdstpa);
    }

    if (burst_count)
    {
        dprintf("DMA[P->M][I2C][common][seg][B]: Burst size = %" PRIu32 " bytes, count = %" PRIu32 ".\n", burst_size, burst_count);

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SAF
                                              | ALT_DMA_CCR_OPT_SS8
                                              | ALT_DMA_CCR_OPT_SB1
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ALT_DMA_CCR_OPT_DAI
                                              | ALT_DMA_CCR_OPT_DS8
                                              | ((burst_size - 1) << 18)
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        while (burst_count > 0)
        {
            uint32_t loopcount = ALT_MIN(burst_count, 256);
            int i;

            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            burst_count -= loopcount;

            dprintf("DMA[P->M][I2C][common][seg][B]: Creating %" PRIu32 " burst-type transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAFLUSHP(program, periph);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAWFP(program, periph, ALT_DMA_PROGRAM_INST_MOD_BURST);
            }

            for (i = 0; i < burst_size; ++i)
            {
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
                }
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
            }

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }
    }

    if (single_count)
    {
        dprintf("DMA[P->M][I2C][common][seg][S]: Single size = 1 byte, count = %" PRIu32 ".\n", single_count);

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SAF
                                              | ALT_DMA_CCR_OPT_SS8
                                              | ALT_DMA_CCR_OPT_SB1
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ALT_DMA_CCR_OPT_DAI
                                              | ALT_DMA_CCR_OPT_DS8
                                              | ALT_DMA_CCR_OPT_DB1
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        while (single_count > 0)
        {
            uint32_t loopcount = ALT_MIN(single_count, 256);
            single_count -= loopcount;

            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            dprintf("DMA[P->M][I2C][common][seg][S]: Creating %" PRIu32 " single-type transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAFLUSHP(program, periph);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAWFP(program, periph, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
            }

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_i2c_to_memory_common(ALT_DMA_PROGRAM_t * program,
                                                    ALT_DMA_PERIPH_t periph,
                                                    ALT_DMA_PERIPH_INFO_I2C_t * i2c_info,
                                                    void * dst,
                                                    size_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_MMU_VA_TO_PA_COALESCE_t coalesce;

    /* Use the I2C register interface to avoid coupling I2C and DMA. */
    uint8_t threshold = ALT_I2C_RX_TL_RX_TL_GET(alt_read_word(ALT_I2C_RX_TL_ADDR(i2c_info->i2c_dev->location))) + 1;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR,
                                        (uintptr_t)ALT_I2C_DATA_CMD_ADDR(i2c_info->i2c_dev->location));
    }

    /*
     * Attempt to coalesce and make the transfer.
    */

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, dst, size);
    }

    while (size)
    {
        /*
         * Extract the next segment.
        */

        uintptr_t segpa   = 0;
        uint32_t  segsize = 0;

        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);

        size -= segsize;
        dprintf("DMA[P->M][I2C][common]: Next segment PA = 0x%x, size = 0x%" PRIx32 "; remaining = 0x%x.\n", segpa, segsize, size);

        /*
         * Transfer in the current segment.
        */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_i2c_to_memory_common_segment(program,
                                                          periph,
                                                          threshold,
                                                          segpa,
                                                          segsize);
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_i2c_to_memory_master(ALT_DMA_PROGRAM_t * program,
                                                    ALT_DMA_PERIPH_t rx_periph,
                                                    ALT_DMA_PERIPH_INFO_I2C_t * i2c_info,
                                                    char * dst,
                                                    size_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    uintptr_t scratchpa;
    uint32_t prefill_count;

    /*
     * Looking at the periph values, the TX is always 1 less than the RX.
     *
     * TX request DMA items cannot be bursted as the memory used to store that
     * info is only 1 item wide.
     * */
    ALT_DMA_PERIPH_t tx_periph = (ALT_DMA_PERIPH_t)((int)rx_periph - 1);
    size_t           tx_left   = size;
    size_t           rx_left   = size;

    uint32_t fifo_size = ALT_MIN(ALT_I2C_RX_FIFO_NUM_ENTRIES, ALT_I2C_TX_FIFO_NUM_ENTRIES);

    if (status == ALT_E_SUCCESS)
    {
        uint32_t dfsr;
        uint32_t seglength;
        scratchpa = alt_mmu_va_to_pa(i2c_info->scratch, &seglength, &dfsr);
        if (dfsr)
        {
            printf("DMA[M->P][I2C]: Error: Cannot get VA-to-PA of scratch buffer.\n");
            status = ALT_E_ERROR;
        }
    }

    /* DMA scratch space allocation:
     *  [0] => regular reads (no restart, no stop)
     *  [1] => start read (restart, no stop)
     *  [2] => stop read (no restart, stop)
     *  [3] => single read (restart, stop) */

    i2c_info->scratch[0] = ALT_I2C_DATA_CMD_CMD_SET(ALT_I2C_DATA_CMD_CMD_E_RD) |
                           ALT_I2C_DATA_CMD_RESTART_SET(false)                 |
                           ALT_I2C_DATA_CMD_STOP_SET(false);

    i2c_info->scratch[1] = ALT_I2C_DATA_CMD_CMD_SET(ALT_I2C_DATA_CMD_CMD_E_RD)   |
                           ALT_I2C_DATA_CMD_RESTART_SET(i2c_info->issue_restart) |
                           ALT_I2C_DATA_CMD_STOP_SET(false);

    i2c_info->scratch[2] = ALT_I2C_DATA_CMD_CMD_SET(ALT_I2C_DATA_CMD_CMD_E_RD) |
                           ALT_I2C_DATA_CMD_RESTART_SET(false)                 |
                           ALT_I2C_DATA_CMD_STOP_SET(i2c_info->issue_stop);

    i2c_info->scratch[3] = ALT_I2C_DATA_CMD_CMD_SET(ALT_I2C_DATA_CMD_CMD_E_RD)   |
                           ALT_I2C_DATA_CMD_RESTART_SET(i2c_info->issue_restart) |
                           ALT_I2C_DATA_CMD_STOP_SET(i2c_info->issue_stop);

    /* First, fill up the TX buffer with the read requests. It doesn't matter if the TX has not yet flushed
     * from a previous request. */

    prefill_count = ALT_MIN(tx_left, fifo_size);
    tx_left -= prefill_count;

    /* Issue the read which may contain the restart. */

    if (prefill_count == 1)
    {
        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[P->M][I2C][master]: Prefill read requests (single).\n");

            status = alt_dma_memory_to_i2c_master_custom(program,
                                                         tx_periph,
                                                         i2c_info->i2c_dev,
                                                         scratchpa + (sizeof(uint32_t) * 3), /* &i2c_info->scratch[3], */
                                                         1,
                                                         true);
        }
    }
    else if (tx_left == 0)
    {
        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[P->M][I2C][master]: Prefill read requests (first).\n");

            status = alt_dma_memory_to_i2c_master_custom(program,
                                                         tx_periph,
                                                         i2c_info->i2c_dev,
                                                         scratchpa + (sizeof(uint32_t) * 1), /* &i2c_info->scratch[1], */
                                                         1,
                                                         true);
        }

        if ((status == ALT_E_SUCCESS) && (prefill_count > 2))
        {
            dprintf("DMA[P->M][I2C][master]: Prefill read requests (middle); count = %" PRIu32 ".\n", prefill_count - 2);

            status = alt_dma_memory_to_i2c_master_custom(program,
                                                         tx_periph,
                                                         i2c_info->i2c_dev,
                                                         scratchpa + (sizeof(uint32_t) * 0), /* &i2c_info->scratch[0], */
                                                         prefill_count - 2,
                                                         false);
        }

        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[P->M][I2C][master]: Prefill read requests (last).\n");

            status = alt_dma_memory_to_i2c_master_custom(program,
                                                         tx_periph,
                                                         i2c_info->i2c_dev,
                                                         scratchpa + (sizeof(uint32_t) * 2), /* &i2c_info->scratch[2], */
                                                         1,
                                                         false);
        }
    }
    else
    {
        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[P->M][I2C][master]: Prefill read requests (first).\n");

            status = alt_dma_memory_to_i2c_master_custom(program,
                                                         tx_periph,
                                                         i2c_info->i2c_dev,
                                                         scratchpa + (sizeof(uint32_t) * 1), /* &i2c_info->scratch[1], */
                                                         1,
                                                         true);
        }

        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[P->M][I2C][master]: Prefill read requests (middle); count = %" PRIu32 ".\n", prefill_count - 1);

            status = alt_dma_memory_to_i2c_master_custom(program,
                                                         tx_periph,
                                                         i2c_info->i2c_dev,
                                                         scratchpa + (sizeof(uint32_t) * 0), /* &i2c_info->scratch[0], */
                                                         prefill_count - 1,
                                                         false);
        }
    }

    /* Loop:
     *  - read in half of the FIFO size
     *  - refill half FIFO size
     * Until all the data requested is transfered. */

    while (rx_left > 0)
    {
        /* Read in half of the FIFO size. */

        uint32_t single_count = ALT_MIN(rx_left, fifo_size / 2);

        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        status = alt_dma_i2c_to_memory_common(program,
                                              rx_periph,
                                              i2c_info,
                                              dst,
                                              single_count);

        rx_left -= single_count;
        dst     += single_count;

        /* Now request in another half of the TX FIFO. */

        if (tx_left > 0)
        {
            uint32_t refill_count = ALT_MIN(tx_left, fifo_size / 2);
            tx_left -= refill_count;

            if (tx_left == 0)
            {
                if (status == ALT_E_SUCCESS)
                {
                    dprintf("DMA[P->M][I2C][master]: Refill read requests (middle); count = %" PRIu32 ".\n", refill_count - 1);

                    status = alt_dma_memory_to_i2c_master_custom(program,
                                                                 tx_periph,
                                                                 i2c_info->i2c_dev,
                                                                 scratchpa + (sizeof(uint32_t) * 0), /* &i2c_info->scratch[0], */
                                                                 refill_count - 1,
                                                                 true);
                }

                if (status == ALT_E_SUCCESS)
                {
                    dprintf("DMA[P->M][I2C][master]: Refill read requests (last).\n");

                    status = alt_dma_memory_to_i2c_master_custom(program,
                                                                 tx_periph,
                                                                 i2c_info->i2c_dev,
                                                                 scratchpa + (sizeof(uint32_t) * 2), /* &i2c_info->scratch[2], */
                                                                 1,
                                                                 false);
                }
            }
            else
            {
                if (status == ALT_E_SUCCESS)
                {
                    dprintf("DMA[P->M][I2C][master]: Refill read requests (middle); count = %" PRIu32 ".\n", refill_count);

                    status = alt_dma_memory_to_i2c_master_custom(program,
                                                                 tx_periph,
                                                                 i2c_info->i2c_dev,
                                                                 scratchpa + (sizeof(uint32_t) * 0), /* &i2c_info->scratch[0], */
                                                                 refill_count,
                                                                 true);
                }
            }
        }

    } /* while (rx_left > 0) */

    /* Ensure that the scratch data is in SDRAM for the DMAC to read. */
    if (status == ALT_E_SUCCESS)
    {
        /* Sync the periph scratch information to RAM. */
        void * vaddr  = (void *)((uintptr_t)(i2c_info->scratch) & ~(ALT_CACHE_LINE_SIZE - 1));
        void * vend   = (void *)(((uintptr_t)(i2c_info->scratch) + sizeof(i2c_info->scratch) + (ALT_CACHE_LINE_SIZE - 1)) & ~(ALT_CACHE_LINE_SIZE - 1));
        size_t length = (uintptr_t)vend - (uintptr_t)vaddr;

        status = alt_cache_system_clean(vaddr, length);
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_i2c_to_memory(ALT_DMA_PROGRAM_t * program,
                                             ALT_DMA_PERIPH_t periph,
                                             ALT_DMA_PERIPH_INFO_I2C_t * i2c_info,
                                             void * dst,
                                             size_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_I2C_MODE_t mode;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_i2c_op_mode_get(i2c_info->i2c_dev, &mode);
    }

    if (status == ALT_E_SUCCESS)
    {
        if (mode == ALT_I2C_MODE_MASTER)
        {
            dprintf("DMA[P->M][I2C]: Controller detected as Master.\n");

            status = alt_dma_i2c_to_memory_master(program,
                                                  periph,
                                                  i2c_info,
                                                  dst,
                                                  size);

        }
        else
        {
            dprintf("DMA[P->M][I2C]: Controller detected as Slave.\n");

            status = alt_dma_i2c_to_memory_common(program,
                                                  periph,
                                                  i2c_info,
                                                  dst,
                                                  size);
        }
    }

    return status;
}
#endif

#if ALT_DMA_PERIPH_PROVISION_QSPI_SUPPORT
static ALT_STATUS_CODE alt_dma_memory_to_qspi_segment(ALT_DMA_PROGRAM_t * program,
                                                      uint32_t qspi_single_size_log2,
                                                      uint32_t qspi_burst_size_log2,
                                                      uintptr_t segsrcpa,
                                                      size_t segsize)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    uint32_t qspi_single_size = 1 << qspi_single_size_log2;
    uint32_t qspi_single_count = 0;
    uint32_t qspi_burst_size  = 1 << qspi_burst_size_log2;
    uint32_t qspi_burst_count;

    if (segsrcpa & 0x3)
    {
        return ALT_E_ERROR;
    }

    if (segsize & 0x3)
    {
        return ALT_E_ERROR;
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR,
                                        segsrcpa);
    }

    dprintf("DMA[M->P][QSPI][seg]: QSPI Single = %" PRIu32 "; Burst = %" PRIu32 ".\n", qspi_single_size, qspi_burst_size);

    /* Because single transfers are equal or smaller than burst (and in the
     * smaller case, it is always a clean multiple), only the single size
     * check is needed for transfer composability. */
    if (segsize & (qspi_single_size - 1))
    {
        dprintf("DMA[M->P][QSPI][seg]: QSPI DMA size configuration not suitable for transfer request.\n");
        return ALT_E_ERROR;
    }

    if (segsrcpa & 0x7)
    {
        /* Source address is not 8-byte aligned. Do 1x 32-bit transfer to get it 8-byte aligned. */

        dprintf("DMA[M->P][QSPI][seg]: Creating 1x 4-byte aligning transfer.\n");

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SAI
                                              | ALT_DMA_CCR_OPT_SS32
                                              | ALT_DMA_CCR_OPT_SB1
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ALT_DMA_CCR_OPT_DAF
                                              | ALT_DMA_CCR_OPT_DS32
                                              | ALT_DMA_CCR_OPT_DB1
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC_DEFAULT
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAFLUSHP(program, ALT_DMA_PERIPH_QSPI_FLASH_TX);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAWFP(program, ALT_DMA_PERIPH_QSPI_FLASH_TX, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }

        segsize -= sizeof(uint32_t);
    }

    qspi_burst_count  = segsize >> qspi_burst_size_log2;

    /* Use QSPI burst transfers if:
     *  - QSPI bursts are larger than QSPI singles [AND]
     *  - Size is large enough that at least 1 burst will be used. */

    if (   (qspi_burst_size_log2 > qspi_single_size_log2)
        && (qspi_burst_count != 0)
       )
    {
        /* 1 << 3 => 8 bytes => 64 bits, which is the width of the AXI bus. */
        uint32_t src_size_log2 = ALT_MIN(3, qspi_burst_size_log2);

        uint32_t dst_multiple;
        uint32_t src_length   = 0;
        uint32_t src_multiple = 0;

        /* qspi_burst_count = segsize >> qspi_burst_size_log2; */
        qspi_single_count   = (segsize & (qspi_burst_size - 1)) >> qspi_single_size_log2;

        dprintf("DMA[M->P][QSPI][seg][B]: Burst size = %" PRIu32 " bytes, count = %" PRIu32 ".\n", qspi_burst_size, qspi_burst_count);

        if ((qspi_burst_size >> src_size_log2) <= 16)
        {
            src_length   = qspi_burst_size >> src_size_log2;
            src_multiple = 1;
        }
        else
        {
            src_length   = 16;
            src_multiple = (qspi_burst_size >> src_size_log2) >> 4; /* divide by 16 */

            if (src_multiple == 0)
            {
                dprintf("DEBUG[QSPI][seg][B]: src_multiple is 0.\n");
                status = ALT_E_ERROR;
            }
        }

        /* uint32_t dst_length = 1; // dst_length is always 1 because the address is fixed. */
        dst_multiple = qspi_burst_size >> 2; /* divide by sizeof(uint32_t) */

        dprintf("DMA[M->P][QSPI][seg][B]: dst_size = %u bits, dst_length = %u, dst_multiple = %" PRIu32 ".\n",
                32,                       1,          dst_multiple);
        dprintf("DMA[M->P][QSPI][seg][B]: src_size = %u bits, src_length = %" PRIu32", src_multiple = %" PRIu32 ".\n",
                (1 << src_size_log2) * 8, src_length, src_multiple);

        /* Program in the following parameters:
         *  - SAI   : Source      address increment
         *  - SSx   : Source      burst size of [1 << src_size_log2]-bytes
         *  - SBx   : Source      burst length of [src_length] transfer(s)
         *  - SC(7) : Source      cacheable write-back, allocate on reads only
         *  - DAF   : Destination address fixed
         *  - DS32  : Destination burst size of 4-bytes
         *  - DB1   : Destination burst length of 1 transfer
         *  - All other parameters default */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SAI
                                              | (src_size_log2 << 1) /* SS */
                                              | ((src_length - 1) << 4) /* SB */
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC(7)
                                              | ALT_DMA_CCR_OPT_DAF
                                              | ALT_DMA_CCR_OPT_DS32
                                              | ALT_DMA_CCR_OPT_DB1
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC_DEFAULT
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        /* NOTE: We do not do the 256x bursts for M->P case because we only
         *   write up to 256 B at a time. */

        while (qspi_burst_count > 0)
        {
            uint32_t loopcount = ALT_MIN(qspi_burst_count, 256);
            uint32_t j,k;
            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            qspi_burst_count -= loopcount;

            dprintf("DMA[M->P][QSPI][seg][B]: Creating %" PRIu32 " burst-type transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAFLUSHP(program, ALT_DMA_PERIPH_QSPI_FLASH_TX);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAWFP(program, ALT_DMA_PERIPH_QSPI_FLASH_TX, ALT_DMA_PROGRAM_INST_MOD_BURST);
            }
            for (j = 0; j < src_multiple; ++j)
            {
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
                }
            }
            for (k = 0; k < dst_multiple; ++k)
            {
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
                }
            }

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }
    }
    else
    {
        qspi_single_count = segsize >> qspi_single_size_log2;
    }

    /* Assemble the single portion of the DMA program. */
    if (qspi_single_count)
    {
        /* 1 << 3 => 8 bytes => 64 bits, which is the width of the AXI bus. */
        uint32_t src_size_log2 = ALT_MIN(3, qspi_single_size_log2);

        uint32_t src_length   = 0;
        uint32_t src_multiple = 0;
        uint32_t dst_multiple;

        dprintf("DMA[M->P][QSPI][seg][S]: Single size = %" PRIu32 " bytes, count = %" PRIu32 ".\n", qspi_single_size, qspi_single_count);

        if ((qspi_single_size >> src_size_log2) <= 16)
        {
            src_length   = qspi_single_size >> src_size_log2;
            src_multiple = 1;
        }
        else
        {
            src_length   = 16;
            src_multiple = (qspi_single_size >> src_size_log2) >> 4; /* divide by 16 */

            if (src_multiple == 0)
            {
                dprintf("DEBUG[QSPI][seg][S]: src_multiple is 0.\n");
                status = ALT_E_ERROR;
            }
        }

        /* uint32_t dst_length = 1; // dst_length is always 1 becaus the address is fixed. */
        dst_multiple = qspi_single_size >> 2; /* divide by sizeof(uint32_t) */

        dprintf("DMA[M->P][QSPI][seg][S]: dst_size = %u bits, dst_length = %u, dst_multiple = %" PRIu32 ".\n",
                32,                      1,          dst_multiple);
        dprintf("DMA[M->P][QSPI][seg][S]: src_size = %u bits, src_length = %" PRIu32 ", src_multiple = %" PRIu32 ".\n",
                (1 <<src_size_log2) * 8, src_length, src_multiple);

        /* Program in the following parameters:
         *  - SAI   : Source      address increment)
         *  - SSx   : Source      burst size of [1 << src_size_log2]-bytes
         *  - SBx   : Source      burst length of [src_length] transfer(s)
         *  - SC(7) : Source      cacheable write-back, allocate on reads only
         *  - DAF   : Destination address fixed
         *  - DS32  : Destination burst size of 4-bytes
         *  - DB1   : Destination burst length of 1 transfer
         *  - All other parameters default */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SAI
                                              | (src_size_log2 << 1) /* SS */
                                              | ((src_length - 1) << 4) /* SB */
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ALT_DMA_CCR_OPT_DAF
                                              | ALT_DMA_CCR_OPT_DS32
                                              | ALT_DMA_CCR_OPT_DB1
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC_DEFAULT
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        /* NOTE: We do not do the 256x bursts for M->P case because we only
         *   write up to 256 B at a time. */

        while (qspi_single_count > 0)
        {
            uint32_t loopcount = ALT_MIN(qspi_single_count, 256);
            uint32_t j,k;
            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            qspi_single_count -= loopcount;

            dprintf("DMA[M->P][QSPI][seg][S]: Creating %" PRIu32 " single-type transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAFLUSHP(program, ALT_DMA_PERIPH_QSPI_FLASH_TX);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAWFP(program, ALT_DMA_PERIPH_QSPI_FLASH_TX, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
            }
            for (j = 0; j < src_multiple; ++j)
            {
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
                }
            }
            for (k = 0; k < dst_multiple; ++k)
            {
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
                }
            }

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }

    } /* if (qspi_single_count != 0) */

    return status;
}

static ALT_STATUS_CODE alt_dma_memory_to_qspi(ALT_DMA_PROGRAM_t * program,
                                              const char * src,
                                              size_t size)
{
    /* Detect if memory region overshoots address space
     * This error checking is handled by the coalescing API. */

    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    uint32_t dmaper;
    uint32_t qspi_single_size_log2;
    uint32_t qspi_burst_size_log2;
    ALT_MMU_VA_TO_PA_COALESCE_t coalesce;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR,
                                        (uint32_t)ALT_QSPIDATA_ADDR);
    }

    dmaper = alt_read_word(ALT_QSPI_DMAPER_ADDR);
    qspi_single_size_log2 = ALT_QSPI_DMAPER_NUMSGLREQBYTES_GET(dmaper);
    qspi_burst_size_log2  = ALT_QSPI_DMAPER_NUMBURSTREQBYTES_GET(dmaper);

    /*
     * Attempt to coalesce and make the transfer.
    */

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, src, size);
    }

    while (size)
    {
        /*
         * Extract the next segment.
        */

        uintptr_t segpa   = 0;
        uint32_t  segsize = 0;

        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);

        size -= segsize;
        dprintf("DMA[M->P][QSPI]: Next segment PA = 0x%x, size = 0x%" PRIx32 "; remaining = 0x%x.\n", segpa, segsize, size);

        /*
         * Transfer in the current segment.
        */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_memory_to_qspi_segment(program,
                                                    qspi_single_size_log2,
                                                    qspi_burst_size_log2,
                                                    segpa,
                                                    segsize);
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_qspi_to_memory_segment(ALT_DMA_PROGRAM_t * program,
                                                      uint32_t qspi_single_size_log2,
                                                      uint32_t qspi_burst_size_log2,
                                                      uintptr_t segdstpa,
                                                      size_t segsize)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    uint32_t qspi_single_size = 1 << qspi_single_size_log2;
    uint32_t qspi_single_count = 0;
    uint32_t qspi_burst_size  = 1 << qspi_burst_size_log2;
    uint32_t qspi_burst_count;

    if (segdstpa & 0x3)
    {
        return ALT_E_ERROR;
    }

    if (segsize & 0x3)
    {
        return ALT_E_ERROR;
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR,
                                        segdstpa);
    }

    dprintf("DMA[P->M][QSPI][seg]: QSPI Single = %" PRIu32 "; Burst = %" PRIu32 ".\n", qspi_single_size, qspi_burst_size);

    /* Because single transfers are equal or smaller than burst (and in the
     * smaller case, it is always a clean multiple), only the single size
     * check is needed for transfer composability. */
    if (segsize & (qspi_single_size - 1))
    {
        dprintf("DMA[P->M][QSPI][seg]: QSPI DMA size configuration not suitable for transfer request.\n");
        return ALT_E_ERROR;
    }

    if (segdstpa & 0x7)
    {
        /* Destination address is not 8-byte aligned. Do 1x 32-bit transfer to get it 8-byte aligned. */

        dprintf("DMA[P->M][QSPI][seg]: Creating 1x 4-byte aligning transfer.\n");

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SAF
                                              | ALT_DMA_CCR_OPT_SS32
                                              | ALT_DMA_CCR_OPT_SB1
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ALT_DMA_CCR_OPT_DAI
                                              | ALT_DMA_CCR_OPT_DS32
                                              | ALT_DMA_CCR_OPT_DB1
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAFLUSHP(program, ALT_DMA_PERIPH_QSPI_FLASH_RX);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAWFP(program, ALT_DMA_PERIPH_QSPI_FLASH_RX, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }

        segsize -= sizeof(uint32_t);
    }

    qspi_burst_count  = segsize >> qspi_burst_size_log2;

    /* Use QSPI burst transfers if:
     *  - QSPI bursts are larger than QSPI singles [AND]
     *  - Size is large enough that at least 1 burst will be used. */

    if (   (qspi_burst_size_log2 > qspi_single_size_log2)
        && (qspi_burst_count != 0)
       )
    {
        /* 1 << 3 => 8 bytes => 64 bits, which is the width of the AXI bus. */
        uint32_t dst_size_log2 = ALT_MIN(3, qspi_burst_size_log2);
        uint32_t dst_length   = 0;
        uint32_t dst_multiple = 0;
        uint32_t src_multiple;

        /* qspi_burst_count = segsize >> qspi_burst_size_log2; */
        qspi_single_count   = (segsize & (qspi_burst_size - 1)) >> qspi_single_size_log2;

        dprintf("DMA[P->M][QSPI][seg][B]: Burst size = %" PRIu32 " bytes, count = %" PRIu32 ".\n", qspi_burst_size, qspi_burst_count);

        if ((qspi_burst_size >> dst_size_log2) <= 16)
        {
            dst_length   = qspi_burst_size >> dst_size_log2;
            dst_multiple = 1;
        }
        else
        {
            dst_length   = 16;
            dst_multiple = (qspi_burst_size >> dst_size_log2) >> 4; /* divide by 16 */

            if (dst_multiple == 0)
            {
                dprintf("DEBUG[QSPI][B][seg]: dst_multiple is 0.\n");
                status = ALT_E_ERROR;
            }
        }

        /* uint32_t src_length = 1; // src_length is always 1 because the address is fixed. */
        src_multiple = qspi_burst_size >> 2; /* divide by sizeof(uint32_t) */

        dprintf("DMA[P->M][QSPI][seg][B]: dst_size = %u bits, dst_length = %" PRIu32 ", dst_multiple = %" PRIu32 ".\n",
                (1 << dst_size_log2) * 8, dst_length, dst_multiple);
        dprintf("DMA[P->M][QSPI][seg][B]: src_size = %u bits, src_length = %u, src_multiple = %" PRIu32 ".\n",
                32,                       1,          src_multiple);

        /* Program in the following parameters:
         *  - SAF   : Source      address fixed
         *  - SS32  : Source      burst size of 4-bytes
         *  - SB1   : Source      burst length of 1 transfer
         *  - DAI   : Destination address increment
         *  - DSx   : Destination burst size of [1 << dst_size_log2]-bytes]
         *  - DBx   : Destination burst length of [dst_length] transfer(s)
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other parameters default */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SAF
                                              | ALT_DMA_CCR_OPT_SS32
                                              | ALT_DMA_CCR_OPT_SB1
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ALT_DMA_CCR_OPT_DAI
                                              | (dst_size_log2 << 15) /* DS */
                                              | ((dst_length - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        /* See how many 256x bursts we can construct. This will allow for extremely large requests. */

        if (qspi_burst_count >> 8)
        {
            uint32_t j,k;
            uint32_t qspi_burst256_count = qspi_burst_count >> 8;
            qspi_burst_count &= (1 << 8) - 1;

            while (qspi_burst256_count > 0)
            {
                uint32_t loopcount = ALT_MIN(qspi_burst256_count, 256);
                if (status != ALT_E_SUCCESS)
                {
                    break;
                }

                qspi_burst256_count -= loopcount;

                dprintf("DMA[P->M][QSPI][seg][B]: Creating %" PRIu32 " 256x burst-type transfer(s).\n", loopcount);

                /* Outer loop { */

                if ((status == ALT_E_SUCCESS) && (loopcount > 1))
                {
                    status = alt_dma_program_DMALP(program, loopcount);
                }

                /* Inner loop { */

                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALP(program, 256);
                }

                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAFLUSHP(program, ALT_DMA_PERIPH_QSPI_FLASH_RX);
                }
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAWFP(program, ALT_DMA_PERIPH_QSPI_FLASH_RX, ALT_DMA_PROGRAM_INST_MOD_BURST);
                }
                for (j = 0; j < src_multiple; ++j)
                {
                    if (status == ALT_E_SUCCESS)
                    {
                        status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
                    }
                }
                for (k = 0; k < dst_multiple; ++k)
                {
                    if (status == ALT_E_SUCCESS)
                    {
                        status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
                    }
                }

                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }

                /* } Inner loop */

                if ((status == ALT_E_SUCCESS) && (loopcount > 1))
                {
                    status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }

                /* } Outer loop */
            }
        }

        while (qspi_burst_count > 0)
        {
            uint32_t loopcount = ALT_MIN(qspi_burst_count, 256);
            uint32_t j,k;
            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            qspi_burst_count -= loopcount;

            dprintf("DMA[P->M][QSPI][seg][B]: Creating %" PRIu32 " burst-type transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAFLUSHP(program, ALT_DMA_PERIPH_QSPI_FLASH_RX);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAWFP(program, ALT_DMA_PERIPH_QSPI_FLASH_RX, ALT_DMA_PROGRAM_INST_MOD_BURST);
            }
            for (j = 0; j < src_multiple; ++j)
            {
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
                }
            }
            for (k = 0; k < dst_multiple; ++k)
            {
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
                }
            }

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }
    }
    else
    {
        qspi_single_count = segsize >> qspi_single_size_log2;
    }

    /* Assemble the single portion of the DMA program. */
    if (qspi_single_count)
    {
        /* 1 << 3 => 8 bytes => 64 bits, which is the width of the AXI bus. */
        uint32_t dst_size_log2 = ALT_MIN(3, qspi_single_size_log2);
        uint32_t dst_length   = 0;
        uint32_t dst_multiple = 0;
        uint32_t src_multiple;

        dprintf("DMA[P->M][QSPI][seg][S]: Single size = %" PRIu32 " bytes, count = %" PRIu32 ".\n", qspi_single_size, qspi_single_count);

        if ((qspi_single_size >> dst_size_log2) <= 16)
        {
            dst_length   = qspi_single_size >> dst_size_log2;
            dst_multiple = 1;
        }
        else
        {
            dst_length   = 16;
            dst_multiple = (qspi_single_size >> dst_size_log2) >> 4; /* divide by 16 */

            if (dst_multiple == 0)
            {
                dprintf("DEBUG[QSPI][seg][S]: dst_multiple is 0.\n");
                status = ALT_E_ERROR;
            }
        }

        /* uint32_t src_length = 1; // src_length is always 1 because the address is fixed. */
        src_multiple = qspi_single_size >> 2; /* divide by sizeof(uint32_t) */

        dprintf("DMA[P->M][QSPI][seg][S]: dst_size = %u bits, dst_length = %" PRIu32 ", dst_multiple = %" PRIu32 ".\n",
                (1 << dst_size_log2) * 8, dst_length, dst_multiple);
        dprintf("DMA[P->M][QSPI][seg][S]: src_size = %u bits, src_length = %u, src_multiple = %" PRIu32 ".\n",
                32,                       1,          src_multiple);

        /* Program in the following parameters:
         *  - SAF   : Source      address fixed
         *  - SS32  : Source      burst size of 4-bytes
         *  - SB1   : Source      burst length of 1 transfer
         *  - DAI   : Destination address increment
         *  - DSx   : Destination burst size of [1 << dst_size_log2]-bytes]
         *  - DBx   : Destination burst length of [dst_length] transfer(s)
         *  - DC(7) : Destination cacheable write-back, allocate on writes only
         *  - All other parameters default */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                            (   ALT_DMA_CCR_OPT_SAF
                                              | ALT_DMA_CCR_OPT_SS32
                                              | ALT_DMA_CCR_OPT_SB1
                                              | ALT_DMA_CCR_OPT_SP_DEFAULT
                                              | ALT_DMA_CCR_OPT_SC_DEFAULT
                                              | ALT_DMA_CCR_OPT_DAI
                                              | (dst_size_log2 << 15) /* DS */
                                              | ((dst_length - 1) << 18) /* DB */
                                              | ALT_DMA_CCR_OPT_DP_DEFAULT
                                              | ALT_DMA_CCR_OPT_DC(7)
                                              | ALT_DMA_CCR_OPT_ES_DEFAULT
                                            )
                );
        }

        /* See how many 256x bursts we can construct. This will allow for extremely large requests. */

        if (qspi_single_count >> 8)
        {
            uint32_t j,k;
            uint32_t qspi_single256_count = qspi_single_count >> 8;
            qspi_single_count &= (1 << 8) - 1;

            while (qspi_single256_count > 0)
            {
                uint32_t loopcount = ALT_MIN(qspi_single256_count, 256);
                if (status != ALT_E_SUCCESS)
                {
                    break;
                }

                qspi_single256_count -= loopcount;

                dprintf("DMA[P->M][QSPI][seg][S]: Creating %" PRIu32 " 256x single-type transfer(s).\n", loopcount);

                /* Outer loop { */

                if ((status == ALT_E_SUCCESS) && (loopcount > 1))
                {
                    status = alt_dma_program_DMALP(program, loopcount);
                }

                /* Inner loop { */

                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALP(program, 256);
                }

                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAFLUSHP(program, ALT_DMA_PERIPH_QSPI_FLASH_RX);
                }
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAWFP(program, ALT_DMA_PERIPH_QSPI_FLASH_RX, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
                }
                for (j = 0; j < src_multiple; ++j)
                {
                    if (status == ALT_E_SUCCESS)
                    {
                        status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
                    }
                }
                for (k = 0; k < dst_multiple; ++k)
                {
                    if (status == ALT_E_SUCCESS)
                    {
                        status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
                    }
                }

                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }

                /* } Inner loop */

                if ((status == ALT_E_SUCCESS) && (loopcount > 1))
                {
                    status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
                }

                /* } Outer loop */
            }
        }

        while (qspi_single_count > 0)
        {
            uint32_t j,k;
            uint32_t loopcount = ALT_MIN(qspi_single_count, 256);
            if (status != ALT_E_SUCCESS)
            {
                break;
            }

            qspi_single_count -= loopcount;

            dprintf("DMA[P->M][QSPI][seg][S]: Creating %" PRIu32 " single-type transfer(s).\n", loopcount);

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALP(program, loopcount);
            }

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAFLUSHP(program, ALT_DMA_PERIPH_QSPI_FLASH_RX);
            }
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_program_DMAWFP(program, ALT_DMA_PERIPH_QSPI_FLASH_RX, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
            }
            for (j = 0; j < src_multiple; ++j)
            {
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
                }
            }
            for (k = 0; k < dst_multiple; ++k)
            {
                if (status == ALT_E_SUCCESS)
                {
                    status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
                }
            }

            if ((status == ALT_E_SUCCESS) && (loopcount > 1))
            {
                status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_NONE);
            }
        }

    } /* if (qspi_single_count != 0) */

    return status;
}

static ALT_STATUS_CODE alt_dma_qspi_to_memory(ALT_DMA_PROGRAM_t * program,
                                              char * dst,
                                              size_t size)
{
    /* Detect if memory region overshoots address space
     * This error checking is handled by the coalescing API. */

    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_MMU_VA_TO_PA_COALESCE_t coalesce;
    uint32_t dmaper;
    uint32_t qspi_single_size_log2;
    uint32_t qspi_burst_size_log2;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR,
                                        (uint32_t)ALT_QSPIDATA_ADDR);
    }

    dmaper = alt_read_word(ALT_QSPI_DMAPER_ADDR);
    qspi_single_size_log2 = ALT_QSPI_DMAPER_NUMSGLREQBYTES_GET(dmaper);
    qspi_burst_size_log2  = ALT_QSPI_DMAPER_NUMBURSTREQBYTES_GET(dmaper);

    /*
     * Attempt to coalesce and make the transfer.
    */

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, dst, size);
    }

    while (size)
    {
        /*
         * Extract the next segment.
        */

        uintptr_t segpa   = 0;
        uint32_t  segsize = 0;

        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);
        }

        size -= segsize;
        dprintf("DMA[P->M][QSPI]: Next segment PA = 0x%x, size = 0x%" PRIx32 "; remaining = 0x%x.\n", segpa, segsize, size);

        /*
         * Transfer in the current segment.
        */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_qspi_to_memory_segment(program,
                                                    qspi_single_size_log2,
                                                    qspi_burst_size_log2,
                                                    segpa,
                                                    segsize);
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
    }

    return status;
}
#endif /* ALT_DMA_PERIPH_PROVISION_QSPI_SUPPORT */

#if ALT_DMA_PERIPH_PROVISION_16550_SUPPORT

static ALT_STATUS_CODE alt_dma_memory_to_16550_single_segment(ALT_DMA_PROGRAM_t * program,
                                                              ALT_DMA_PERIPH_t periph,
                                                              size_t segsize)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    uint32_t sizeleft;

    /* 16550 is unusual in that the segments does not write out the SAR.
     * This is because burst and singles can be mixed. Caller is responsible
     * for ensuring the SAR is updated. */

    /* Program in the following parameters:
     *  - SS8   : Source      burst size of 1-byte
     *  - DS8   : Destination burst size of 1-byte
     *  - SB1   : Source      burst length of 1 transfer
     *  - DB1   : Destination burst length of 1 transfer
     *  - DAF   : Destination address fixed
     *  - SC(7) : Source      cacheable write-back, allocate on reads only
     *  - All other options default. */

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                        (   ALT_DMA_CCR_OPT_SB1
                                          | ALT_DMA_CCR_OPT_SS8
                                          | ALT_DMA_CCR_OPT_SA_DEFAULT
                                          | ALT_DMA_CCR_OPT_SP_DEFAULT
                                          | ALT_DMA_CCR_OPT_SC(7)
                                          | ALT_DMA_CCR_OPT_DB1
                                          | ALT_DMA_CCR_OPT_DS8
                                          | ALT_DMA_CCR_OPT_DAF
                                          | ALT_DMA_CCR_OPT_DP_DEFAULT
                                          | ALT_DMA_CCR_OPT_DC_DEFAULT
                                          | ALT_DMA_CCR_OPT_ES_DEFAULT
                                        )
            );
    }

    sizeleft = segsize;

    while (sizeleft > 0)
    {
        uint32_t loopcount = ALT_MIN(sizeleft, 256);
        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        sizeleft -= loopcount;

        dprintf("DMA[M->P][16550][S][seg]: Creating %" PRIu32 " transfer(s).\n", loopcount);

        if ((status == ALT_E_SUCCESS) && (loopcount > 1))
        {
            status = alt_dma_program_DMALP(program, loopcount);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAFLUSHP(program, periph);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAWFP(program, periph, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }

        if ((status == ALT_E_SUCCESS) && (loopcount > 1))
        {
            status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_memory_to_16550_burst_segment(ALT_DMA_PROGRAM_t * program,
                                                             ALT_DMA_PERIPH_t periph,
                                                             size_t burst_size,
                                                             size_t burst_count)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /* 16550 is unusual in that the segments does not write out the SAR.
     * This is because burst and singles can be mixed. Caller is responsible
     * for ensuring the SAR is updated. */

    /* Program in the following parameters:
     *  - SS8   : Source      burst size of 1-byte
     *  - DS8   : Destination burst size of 1-byte
     *  - SB16  : Source      burst length of 16 transfers
     *  - DB16  : Destination burst length of 16 transfers
     *  - DAF   : Source      address fixed
     *  - SC(7) : Source      cacheable write-back, allocate on reads only
     *  - All other options default. */

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                        (   ALT_DMA_CCR_OPT_SB16
                                          | ALT_DMA_CCR_OPT_SS8
                                          | ALT_DMA_CCR_OPT_SA_DEFAULT
                                          | ALT_DMA_CCR_OPT_SP_DEFAULT
                                          | ALT_DMA_CCR_OPT_SC(7)
                                          | ALT_DMA_CCR_OPT_DB16
                                          | ALT_DMA_CCR_OPT_DS8
                                          | ALT_DMA_CCR_OPT_DAF
                                          | ALT_DMA_CCR_OPT_DP_DEFAULT
                                          | ALT_DMA_CCR_OPT_DC_DEFAULT
                                          | ALT_DMA_CCR_OPT_ES_DEFAULT
                                        )
            );
    }

    while (burst_count > 0)
    {
        uint32_t loopcount = ALT_MIN(burst_count, 256);
        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        burst_count -= loopcount;

        dprintf("DMA[M->P][16550][B][seg]: Creating outer %" PRIu32 " inner loop(s).\n", loopcount);

        /* Outer loop { */

        if ((status == ALT_E_SUCCESS) && (loopcount > 1))
        {
            status = alt_dma_program_DMALP(program, loopcount);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAFLUSHP(program, periph);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAWFP(program, periph, ALT_DMA_PROGRAM_INST_MOD_BURST);
        }

        /* Inner loop { */

        /* Loop [burst_size / 16] times. The burst_size was trimmed to the
         * nearest multiple of 16 by the caller. Each burst does 16 transfers
         * hence the need for the divide. */

        dprintf("DMA[M->P][16550][B][seg]: Creating inner %u transfer(s).\n", burst_size >> 4);

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALP(program, burst_size >> 4); /* divide by 16. */
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
        }

        /* } Inner loop */

        if ((status == ALT_E_SUCCESS) && (loopcount > 1))
        {
            status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
        }

        /* } Outer loop */
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_memory_to_16550_single(ALT_DMA_PROGRAM_t * program,
                                                      ALT_DMA_PERIPH_t periph,
                                                      const void * src,
                                                      size_t size)
{
    /* Detect if memory region overshoots address space.
     * This error checking is handled by the coalescing API. */

    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /*
     * Attempt to coalesce and make the transfer.
    */

    ALT_MMU_VA_TO_PA_COALESCE_t coalesce;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, src, size);
    }

    while (size)
    {
        /*
         * Extract the next segment.
        */

        uintptr_t segpa   = 0;
        uint32_t  segsize = 0;

        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);
        }

        size -= segsize;
        dprintf("DMA[M->P][16550][S]: Current segment PA = 0x%x, size = 0x%" PRIx32 "; remaining = 0x%x.\n", segpa, segsize, size);

        /*
         * Transfer out the current segment.
        */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR,
                                            segpa);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_memory_to_16550_single_segment(program,
                                                            periph,
                                                            segsize);
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_memory_to_16550_burst(ALT_DMA_PROGRAM_t * program,
                                                     ALT_DMA_PERIPH_t periph,
                                                     uint32_t burst_size,
                                                     const void * src,
                                                     size_t size)
{
    /* Detect if memory region overshoots address space.
     * This error checking is handled by the coalescing API. */

    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /*
     * Attempt to coalesce and make the transfer.
    */

    ALT_MMU_VA_TO_PA_COALESCE_t coalesce;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, src, size);
    }

    while (size)
    {
        uint32_t sizeleft;
        uintptr_t segpa   = 0;
        uint32_t  segsize = 0;
        uint32_t burst_count = 0;
        ldiv_t rslt;

        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        /*
         * Extract the next segment.
        */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);
        }

        size -= segsize;
        dprintf("DMA[M->P][16550][B]: Next segment PA = 0x%x, size = 0x%" PRIx32 "; remaining = 0x%x.\n", segpa, segsize, size);

        /*
         * Transfer out the current segment.
        */

        sizeleft = segsize;

        /* Determine how many burst transfers can be done */
        rslt = ldiv(sizeleft, burst_size);
        burst_count = rslt.quot;
        sizeleft    = rslt.rem;

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR,
                                            segpa);
        }

        if ((status == ALT_E_SUCCESS) && (burst_count))
        {
            /* Do the burst transfers */

            status = alt_dma_memory_to_16550_burst_segment(program,
                                                           periph,
                                                           burst_size,
                                                           burst_count);
        }

        if ((status == ALT_E_SUCCESS) && (sizeleft))
        {
            /* Do the single transfers */

            status = alt_dma_memory_to_16550_single_segment(program,
                                                            periph,
                                                            sizeleft);
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_memory_to_16550(ALT_DMA_PROGRAM_t * program,
                                               ALT_DMA_PERIPH_t periph,
                                               ALT_16550_HANDLE_t * handle,
                                               const void * src,
                                               size_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR,
                                        (uint32_t)ALT_UART_RBR_THR_DLL_ADDR(handle->location));
    }

    /* Determine if FIFOs are enabled from the FCR cache */

    if (ALT_UART_FCR_FIFOE_GET(handle->fcr) != 0)
    {
        /*
         * FIFOs are enabled.
        */

        uint32_t tx_size;
        uint32_t burst_size;

        dprintf("DMA[M->P][16550]: FIFOs enabled.\n");

        /* Get the TX FIFO Size
         * Use the register interface to avoid coupling the 16550 and DMA. */
        tx_size = ALT_UART_CPR_FIFO_MOD_GET(alt_read_word(ALT_UART_CPR_ADDR(handle->location))) << 4;

        /* Get the TX FIFO Trigger Level from the FCR cache */
        switch ((ALT_16550_FIFO_TRIGGER_TX_t)ALT_UART_FCR_TET_GET(handle->fcr))
        {
        case ALT_16550_FIFO_TRIGGER_TX_EMPTY:
            burst_size = tx_size;
            break;
        case ALT_16550_FIFO_TRIGGER_TX_ALMOST_EMPTY:
            burst_size = tx_size - 2;
            break;
        case ALT_16550_FIFO_TRIGGER_TX_QUARTER_FULL:
            burst_size = 3 * (tx_size >> 2);
            break;
        case ALT_16550_FIFO_TRIGGER_TX_HALF_FULL:
            burst_size = tx_size >> 1;
            break;
        default:
            /* This case should never happen. */
            return ALT_E_ERROR;
        }

        if (burst_size < 16)
        {
            /* There's no point bursting 1 byte at a time per notify, so just do single transfers. */
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_memory_to_16550_single(program,
                                                        periph,
                                                        src,
                                                        size);
            }
        }
        else
        {
            /* Now trim the burst size to a multiple of 16.
             * This will optimize the bursting in the fewest possible commands. */
            dprintf("DMA[M->P][16550]: Untrimmed burst size = %" PRIu32 ".\n", burst_size);
            burst_size &= ~0xf;
            dprintf("DMA[M->P][16550]: Trimmed burst size   = %" PRIu32 ".\n", burst_size);

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_memory_to_16550_burst(program,
                                                       periph,
                                                       burst_size,
                                                       src,
                                                       size);
            }
        }
    }
    else
    {
        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[M->P][16550]: FIFOs disabled.\n");

            /*
             * FIFOs are disabled.
            */

            status = alt_dma_memory_to_16550_single(program,
                                                    periph,
                                                    src,
                                                    size);
        }
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_16550_to_memory_single_segment(ALT_DMA_PROGRAM_t * program,
                                                              ALT_DMA_PERIPH_t periph,
                                                              size_t segsize)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    uint32_t sizeleft = segsize;


    /* 16550 is unusual in that the segments does not write out the DAR.
     * This is because burst and singles can be mixed. Caller is responsible
     * for ensuring the SAR is updated. */

    /* Program in the following parameters:
     *  - SS8   : Source      burst size of 1-byte
     *  - DS8   : Destination burst size of 1-byte
     *  - SB1   : Source      burst length of 1 transfer
     *  - DB1   : Destination burst length of 1 transfer
     *  - SAF   : Source      address fixed
     *  - DC(7) : Destination cacheable write-back, allocate on writes only
     *  - All other options default. */

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                        (   ALT_DMA_CCR_OPT_SB1
                                          | ALT_DMA_CCR_OPT_SS8
                                          | ALT_DMA_CCR_OPT_SAF
                                          | ALT_DMA_CCR_OPT_SP_DEFAULT
                                          | ALT_DMA_CCR_OPT_SC_DEFAULT
                                          | ALT_DMA_CCR_OPT_DB1
                                          | ALT_DMA_CCR_OPT_DS8
                                          | ALT_DMA_CCR_OPT_DA_DEFAULT
                                          | ALT_DMA_CCR_OPT_DP_DEFAULT
                                          | ALT_DMA_CCR_OPT_DC(7)
                                          | ALT_DMA_CCR_OPT_ES_DEFAULT
                                        )
            );
    }

    while (sizeleft > 0)
    {
        uint32_t loopcount = ALT_MIN(sizeleft, 256);
        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        sizeleft -= loopcount;

        dprintf("DMA[P->M][16550][S][seg]: Creating %" PRIu32 " transfer(s).\n", loopcount);

        if ((status == ALT_E_SUCCESS) && (loopcount > 1))
        {
            status = alt_dma_program_DMALP(program, loopcount);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAFLUSHP(program, periph);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAWFP(program, periph, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }
        if ((status == ALT_E_SUCCESS) && (loopcount > 1))
        {
            status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_SINGLE);
        }
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_16550_to_memory_burst_segment(ALT_DMA_PROGRAM_t * program,
                                                             ALT_DMA_PERIPH_t periph,
                                                             size_t burst_size,
                                                             size_t burst_count)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /* 16550 is unusual in that the segments does not write out the DAR.
     * This is because burst and singles can be mixed. Caller is responsible
     * for ensuring the SAR is updated. */

    /* Program in the following parameters:
     *  - SS8   : Source      burst size of 1-byte
     *  - DS8   : Destination burst size of 1-byte
     *  - SB16  : Source      burst length of 16 transfers
     *  - DB16  : Destination burst length of 16 transfers
     *  - SAF   : Source      address fixed
     *  - DC(7) : Destination cacheable write-back, allocate on writes only
     *  - All other options default. */

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_CCR,
                                        (   ALT_DMA_CCR_OPT_SB16
                                          | ALT_DMA_CCR_OPT_SS8
                                          | ALT_DMA_CCR_OPT_SAF
                                          | ALT_DMA_CCR_OPT_SP_DEFAULT
                                          | ALT_DMA_CCR_OPT_SC_DEFAULT
                                          | ALT_DMA_CCR_OPT_DB16
                                          | ALT_DMA_CCR_OPT_DS8
                                          | ALT_DMA_CCR_OPT_DA_DEFAULT
                                          | ALT_DMA_CCR_OPT_DP_DEFAULT
                                          | ALT_DMA_CCR_OPT_DC(7)
                                          | ALT_DMA_CCR_OPT_ES_DEFAULT
                                        )
            );
    }

    while (burst_count > 0)
    {
        uint32_t loopcount = ALT_MIN(burst_count, 256);
        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        burst_count -= loopcount;

        dprintf("DMA[P->M][16550][B][seg]: Creating outer %" PRIu32 " inner loop(s).\n", loopcount);

        /* Outer loop { */

        if ((status == ALT_E_SUCCESS) && (loopcount > 1))
        {
            status = alt_dma_program_DMALP(program, loopcount);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAFLUSHP(program, periph);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAWFP(program, periph, ALT_DMA_PROGRAM_INST_MOD_BURST);
        }

        /* Inner loop { */

        /* Loop [burst_size / 16] times. The burst_size was trimmed to the
         * nearest multiple of 16 by the caller. Each burst does 16 transfers
         * hence the need for the divide. */

        dprintf("DMA[P->M][16550][B][seg]: Creating inner %u transfer(s).\n", burst_size >> 4);

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALP(program, burst_size >> 4); /* divide by 16. */
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALD(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAST(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
        }
        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
        }

        /* } Inner loop */

        if ((status == ALT_E_SUCCESS) && (loopcount > 1))
        {
            status = alt_dma_program_DMALPEND(program, ALT_DMA_PROGRAM_INST_MOD_BURST);
        }

        /* } Outer loop */
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_16550_to_memory_single(ALT_DMA_PROGRAM_t * program,
                                                      ALT_DMA_PERIPH_t periph,
                                                      void * dst,
                                                      size_t size)
{
    /* Detect if memory region overshoots address space.
     * This error checking is handled by the coalescing API. */

    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /*
     * Attempt to coalesce and make the transfer.
     * */

    ALT_MMU_VA_TO_PA_COALESCE_t coalesce;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, dst, size);
    }

    while (size)
    {
        /*
         * Extract the next segment.
        */

        uintptr_t segpa   = 0;
        uint32_t  segsize = 0;

        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);

        size -= segsize;
        dprintf("DMA[P->M][16550][S]: Next segment PA = 0x%x, size = 0x%" PRIx32 "; remaining = 0x%x.\n", segpa, segsize, size);

        /*
         * Transfer in the current segment.
        */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR, segpa);
        }

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_16550_to_memory_single_segment(program, periph, segsize);
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_16550_to_memory_burst(ALT_DMA_PROGRAM_t * program,
                                                     ALT_DMA_PERIPH_t periph,
                                                     uint32_t burst_size,
                                                     void * dst,
                                                     size_t size)
{
    /* Detect if memory region overshoots address space.
     * This error checking is handled by the coalescing API. */

    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /*
     * Attempt to coalesce and make the transfer.
    */

    ALT_MMU_VA_TO_PA_COALESCE_t coalesce;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_begin(&coalesce, dst, size);
    }

    while (size)
    {
        /*
         * Extract the next segment.
        */

        uintptr_t segpa   = 0;
        uint32_t  segsize = 0;
        uint32_t burst_count = 0;
        size_t sizeleft;
        ldiv_t rslt;

        if (status != ALT_E_SUCCESS)
        {
            break;
        }

        status = alt_mmu_va_to_pa_coalesce_next(&coalesce, &segpa, &segsize);

        size -= segsize;
        dprintf("DMA[P->M][16550][B]: Next segment PA = 0x%x, size = 0x%" PRIx32 "; remaining = 0x%x.\n", segpa, segsize, size);

        /*
         * Transfer in the current segment.
        */

        sizeleft = segsize;

        /* Determine how many burst transfers can be done */

        rslt = ldiv(sizeleft, burst_size);
        burst_count = rslt.quot;
        sizeleft    = rslt.rem;

        /* TODO [Fred Hsueh]: Move out the dmamove DAR here. */

        if (status == ALT_E_SUCCESS)
        {
            status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_DAR,
                                            segpa);
        }

        if ((status == ALT_E_SUCCESS) && (burst_count))
        {
            /* Do the burst transfers */

            status = alt_dma_16550_to_memory_burst_segment(program,
                                                           periph,
                                                           burst_size,
                                                           burst_count);
        }

        if ((status == ALT_E_SUCCESS) && (sizeleft))
        {
            /* Do the single transfers */

            status = alt_dma_16550_to_memory_single_segment(program,
                                                            periph,
                                                            sizeleft);
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_to_pa_coalesce_end(&coalesce);
    }

    return status;
}

static ALT_STATUS_CODE alt_dma_16550_to_memory(ALT_DMA_PROGRAM_t * program,
                                               ALT_DMA_PERIPH_t periph,
                                               ALT_16550_HANDLE_t * handle,
                                               void * dst,
                                               size_t size)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAMOV(program, ALT_DMA_PROGRAM_REG_SAR,
                                        (uint32_t)ALT_UART_RBR_THR_DLL_ADDR(handle->location));
    }

    /* Determine if FIFOs are enabled from the FCR cache */

    if (ALT_UART_FCR_FIFOE_GET(handle->fcr) != 0)
    {
        /*
         * FIFOs are enabled.
        */

        uint32_t rx_size;
        uint32_t burst_size;

        dprintf("DMA[P->M][16550]: FIFOs enabled.\n");

        /* Get the RX FIFO Size
         * Use the register interface to avoid coupling the 16550 and DMA. */
        rx_size = ALT_UART_CPR_FIFO_MOD_GET(alt_read_word(ALT_UART_CPR_ADDR(handle->location))) << 4;

        /* Get the RX FIFO Trigger Level from the FCR cache */
        switch ((ALT_16550_FIFO_TRIGGER_RX_t)ALT_UART_FCR_RT_GET(handle->fcr))
        {
        case ALT_16550_FIFO_TRIGGER_RX_ANY:
            burst_size = 1;
            break;
        case ALT_16550_FIFO_TRIGGER_RX_QUARTER_FULL:
            burst_size = rx_size >> 2; /* divide by 4 */
            break;
        case ALT_16550_FIFO_TRIGGER_RX_HALF_FULL:
            burst_size = rx_size >> 1; /* divide by 2 */
            break;
        case ALT_16550_FIFO_TRIGGER_RX_ALMOST_FULL:
            burst_size = rx_size - 2;
            break;
        default:
            /* This case should never happen. */
            return ALT_E_ERROR;
        }

        if (burst_size < 16)
        {
            /* There's no point bursting 1 byte at a time per notify, so just do single transfers. */
            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_16550_to_memory_single(program,
                                                        periph,
                                                        dst,
                                                        size);
            }
        }
        else
        {
            /* Now trim the burst size to a multiple of 16.
             * This will optimize the bursting in the fewest possible commands. */
            dprintf("DMA[P->M][16550]: Untrimmed burst size = %" PRIu32 ".\n", burst_size);
            burst_size &= ~0xf;
            dprintf("DMA[P->M][16550]: Trimmed burst size   = %" PRIu32 ".\n", burst_size);

            if (status == ALT_E_SUCCESS)
            {
                status = alt_dma_16550_to_memory_burst(program,
                                                       periph,
                                                       burst_size,
                                                       dst,
                                                       size);
            }
        }
    }
    else
    {
        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[P->M][16550]: FIFOs disabled.\n");

            /*
             * FIFOs are disabled.
            */

            status = alt_dma_16550_to_memory_single(program,
                                                    periph,
                                                    dst,
                                                    size);
        }
    }

    return status;
}
#endif /* ALT_DMA_PERIPH_PROVISION_16550_SUPPORT */

ALT_STATUS_CODE alt_dma_memory_to_periph(ALT_DMA_CHANNEL_t channel,
                                         ALT_DMA_PROGRAM_t * program,
                                         ALT_DMA_PERIPH_t dstp,
                                         const void * src,
                                         size_t size,
                                         void * periph_info,
                                         bool send_evt,
                                         ALT_DMA_EVENT_t evt)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /* If the size is zero, and no event is requested, just return success. */
    if ((size == 0) && (send_evt == false))
    {
        return ALT_E_SUCCESS;
    }

    if (status == ALT_E_SUCCESS)
    {
        dprintf("DMA[M->P]: Init Program.\n");
        status = alt_dma_program_init(program);
    }

    if ((status == ALT_E_SUCCESS) && (size != 0))
    {
        /* Detect if memory region overshoots address space. */
        if ((uintptr_t)src + size - 1 < (uintptr_t)src)
        {
            return ALT_E_BAD_ARG;
        }

        switch (dstp)
        {
#if ALT_DMA_PERIPH_PROVISION_I2C_SUPPORT
        case ALT_DMA_PERIPH_I2C0_TX:
        case ALT_DMA_PERIPH_I2C1_TX:
        case ALT_DMA_PERIPH_I2C2_TX:
        case ALT_DMA_PERIPH_I2C3_TX:
            status = alt_dma_memory_to_i2c(program, dstp, periph_info, src, size);
            break;
#endif

#if ALT_DMA_PERIPH_PROVISION_QSPI_SUPPORT
        case ALT_DMA_PERIPH_QSPI_FLASH_TX:
            status = alt_dma_memory_to_qspi(program, src, size);
            break;
#endif

#if ALT_DMA_PERIPH_PROVISION_16550_SUPPORT
        case ALT_DMA_PERIPH_UART0_TX:
        case ALT_DMA_PERIPH_UART1_TX:
            status = alt_dma_memory_to_16550(program, dstp,
                                             (ALT_16550_HANDLE_t *)periph_info, src, size);
            break;
#endif

        case ALT_DMA_PERIPH_FPGA_0:
        case ALT_DMA_PERIPH_FPGA_1:
        case ALT_DMA_PERIPH_FPGA_2:
        case ALT_DMA_PERIPH_FPGA_3:
#if defined(soc_cv_av)
        case ALT_DMA_PERIPH_FPGA_4_OR_CAN0_IF1:
        case ALT_DMA_PERIPH_FPGA_5_OR_CAN0_IF2:
        case ALT_DMA_PERIPH_FPGA_6_OR_CAN1_IF1:
        case ALT_DMA_PERIPH_FPGA_7_OR_CAN1_IF2:
#elif defined(soc_a10)
        case ALT_DMA_PERIPH_FPGA_4:
        case ALT_DMA_PERIPH_FPGA_5_OR_SECMGR:
        case ALT_DMA_PERIPH_FPGA_6_OR_I2C4_TX:
#endif
        case ALT_DMA_PERIPH_SPI0_MASTER_TX:
        case ALT_DMA_PERIPH_SPI0_SLAVE_TX:
        case ALT_DMA_PERIPH_SPI1_MASTER_TX:
        case ALT_DMA_PERIPH_SPI1_SLAVE_TX:

        default:
            status = ALT_E_BAD_ARG;
            break;
        }
    }

    /* Send event if requested. */
    if (send_evt)
    {
        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[M->P]: Adding event.\n");
            status = alt_dma_program_DMASEV(program, evt);
        }
    }

    /* Now that everything is done, end the program. */
    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAEND(program);
    }

    /* If there was a problem assembling the program, clean up the buffer and exit. */
    if (status != ALT_E_SUCCESS)
    {
        /* Do not report the status for the clear operation. A failure should be
         * reported regardless of if the clear is successful. */
        alt_dma_program_clear(program);
        return status;
    }

    /* Execute the program on the given channel. */

    return alt_dma_channel_exec(channel, program);
}

ALT_STATUS_CODE alt_dma_periph_to_memory(ALT_DMA_CHANNEL_t channel,
                                         ALT_DMA_PROGRAM_t * program,
                                         void * dst,
                                         ALT_DMA_PERIPH_t srcp,
                                         size_t size,
                                         void * periph_info,
                                         bool send_evt,
                                         ALT_DMA_EVENT_t evt)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    /* If the size is zero, and no event is requested, just return success. */
    if ((size == 0) && (send_evt == false))
    {
        return ALT_E_SUCCESS;
    }

    if (status == ALT_E_SUCCESS)
    {
        dprintf("DMA[P->M]: Init Program.\n");
        status = alt_dma_program_init(program);
    }

    if ((status == ALT_E_SUCCESS) && (size != 0))
    {
        /* Detect if memory region overshoots address space. */
        if ((uintptr_t)dst + size - 1 < (uintptr_t)dst)
        {
            return ALT_E_BAD_ARG;
        }

        switch (srcp)
        {
#if ALT_DMA_PERIPH_PROVISION_I2C_SUPPORT
        case ALT_DMA_PERIPH_I2C0_RX:
        case ALT_DMA_PERIPH_I2C1_RX:
        case ALT_DMA_PERIPH_I2C2_RX:
        case ALT_DMA_PERIPH_I2C3_RX:
            status = alt_dma_i2c_to_memory(program, srcp, periph_info, dst, size);
            break;
#endif

#if ALT_DMA_PERIPH_PROVISION_QSPI_SUPPORT
        case ALT_DMA_PERIPH_QSPI_FLASH_RX:
            status = alt_dma_qspi_to_memory(program, dst, size);
            break;
#endif

#if ALT_DMA_PERIPH_PROVISION_16550_SUPPORT
        case ALT_DMA_PERIPH_UART0_RX:
        case ALT_DMA_PERIPH_UART1_RX:
            status = alt_dma_16550_to_memory(program, srcp,
                                             (ALT_16550_HANDLE_t *)periph_info, dst, size);
            break;
#endif

        case ALT_DMA_PERIPH_FPGA_0:
        case ALT_DMA_PERIPH_FPGA_1:
        case ALT_DMA_PERIPH_FPGA_2:
        case ALT_DMA_PERIPH_FPGA_3:
#if defined(soc_cv_av)
        case ALT_DMA_PERIPH_FPGA_4_OR_CAN0_IF1:
        case ALT_DMA_PERIPH_FPGA_5_OR_CAN0_IF2:
        case ALT_DMA_PERIPH_FPGA_6_OR_CAN1_IF1:
        case ALT_DMA_PERIPH_FPGA_7_OR_CAN1_IF2:
#elif defined(soc_a10)
        case ALT_DMA_PERIPH_FPGA_4:
        case ALT_DMA_PERIPH_FPGA_5_OR_SECMGR:
        case ALT_DMA_PERIPH_FPGA_7_OR_I2C4_RX:
#endif
        case ALT_DMA_PERIPH_SPI0_MASTER_RX:
        case ALT_DMA_PERIPH_SPI0_SLAVE_RX:
        case ALT_DMA_PERIPH_SPI1_MASTER_RX:
        case ALT_DMA_PERIPH_SPI1_SLAVE_RX:

        default:
            status = ALT_E_BAD_ARG;
            break;
        }
    }

    /* Send event if requested. */
    if (send_evt)
    {
        if (status == ALT_E_SUCCESS)
        {
            dprintf("DMA[P->M]: Adding event.\n");
            status = alt_dma_program_DMASEV(program, evt);
        }
    }

    /* Now that everything is done, end the program. */
    if (status == ALT_E_SUCCESS)
    {
        status = alt_dma_program_DMAEND(program);
    }

    /* If there was a problem assembling the program, clean up the buffer and exit. */
    if (status != ALT_E_SUCCESS)
    {
        /* Do not report the status for the clear operation. A failure should be
         * reported regardless of if the clear is successful. */
        alt_dma_program_clear(program);
        return status;
    }

    /* Execute the program on the given channel. */

    return alt_dma_channel_exec(channel, program);
}

static bool alt_dma_is_init(void)
{
#if defined(soc_cv_av)

    uint32_t permodrst = alt_read_word(ALT_RSTMGR_PERMODRST_ADDR);

    if (permodrst & ALT_RSTMGR_PERMODRST_DMA_SET_MSK)
    {
        return false;
    }
    else
    {
        return true;
    }

#elif defined(soc_a10)

    uint32_t per0modrst = alt_read_word(ALT_RSTMGR_PER0MODRST_ADDR);

    if (per0modrst & ALT_RSTMGR_PER0MODRST_DMA_SET_MSK)
    {
        return false;
    }
    else
    {
        return true;
    }

#endif
}

ALT_STATUS_CODE alt_dma_ecc_start(void * block, size_t size)
{
    int i;

    if (alt_dma_is_init() == false)
    {
        return ALT_E_ERROR;
    }

    /* Verify that all channels are either unallocated or allocated and idle. */

    for (i = 0; i < ARRAY_COUNT(g_dmaState.channel_info); ++i)
    {
        if (g_dmaState.channel_info[i].flag & ALT_DMA_CHANNEL_INFO_FLAG_ALLOCED)
        {
            ALT_DMA_CHANNEL_STATE_t state;
            alt_dma_channel_state_get((ALT_DMA_CHANNEL_t)i, &state);

            if (state != ALT_DMA_CHANNEL_STATE_STOPPED)
            {
                dprintf("DMA[ECC]: Error: Channel %d state is non-stopped (%d).\n", i, (int)state);
                return ALT_E_ERROR;
            }
        }
    }

#if defined(soc_cv_av)

    if ((uintptr_t)block & (sizeof(uint64_t) - 1))
    {
        return ALT_E_ERROR;
    }

    /* Enable ECC for DMA RAM */

    dprintf("DEBUG[DMA][ECC]: Enable ECC in SysMgr.\n");
    alt_write_word(ALT_SYSMGR_ECC_DMA_ADDR, ALT_SYSMGR_ECC_DMA_EN_SET_MSK);

    /* Clear any pending spurious DMA ECC interrupts. */

    dprintf("DEBUG[DMA][ECC]: Clear any pending spurious ECC status in SysMgr.\n");
    alt_write_word(ALT_SYSMGR_ECC_DMA_ADDR,
                     ALT_SYSMGR_ECC_DMA_EN_SET_MSK
                   | ALT_SYSMGR_ECC_DMA_SERR_SET_MSK
                   | ALT_SYSMGR_ECC_DMA_DERR_SET_MSK);

#elif defined(soc_a10)

    /*
     * Start ECC memory initialization and wait for it to complete.
     * NOTE: This needs to be done before enabling ECC.
     */
    alt_write_word(ALT_ECC_DMAC_CTL_ADDR,
                   ALT_ECC_DMAC_CTL_INITA_SET_MSK);

    while (!(alt_read_word(ALT_ECC_DMAC_INITSTAT_ADDR) & ALT_ECC_DMAC_INITSTAT_INITCOMPLETEA_SET_MSK))
    {
        ++i;
        if (i == 10000)
        {
            dprintf("ECC[start][DMA]: Timeout waiting for HW init.\n", i);
            return ALT_E_TMO;
        }
    }
    dprintf("ECC[start][DMA]: i = %d.\n", i);

    /*
     * Enable ECC on DMAC.
     */
    alt_write_word(ALT_ECC_DMAC_CTL_ADDR,
                   ALT_ECC_DMAC_CTL_ECC_EN_SET_MSK);

    /*
     * Enable SERR to interrupt.
     */
    alt_write_word(ALT_ECC_DMAC_ERRINTEN_ADDR,
                   ALT_ECC_DMAC_ERRINTEN_SERRINTEN_SET_MSK);

#endif

    return ALT_E_SUCCESS;
}
