/******************************************************************************
 *
 * Copyright 2015 Altera Corporation. All Rights Reserved.
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
 * $Id: //acds/rel/16.1/embedded/ip/hps/altera_hps/hwlib/include/soc_cv_av/alt_dma_periph.h#1 $
 */

#ifndef __ALT_DMA_PERIPH_H__
#define __ALT_DMA_PERIPH_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * This type definition enumerates the SoC system peripherals implementing the
 * required request interface that enables direct DMA transfers to/from the
 * device.
 *
 * FPGA soft IP interface to the DMA are required to comply with the Synopsys
 * protocol.
 *
 * Request interface numbers 4 through 7 are multiplexed between the CAN
 * controllers and soft logic implemented in the FPGA fabric. The selection
 * between the CAN controller and FPGA interfaces is determined at DMA
 * initialization.
 */
typedef enum ALT_DMA_PERIPH_e
{
    ALT_DMA_PERIPH_FPGA_0             = 0,  /*!< FPGA soft IP interface 0 */
    ALT_DMA_PERIPH_FPGA_1             = 1,  /*!< FPGA soft IP interface 1 */
    ALT_DMA_PERIPH_FPGA_2             = 2,  /*!< FPGA soft IP interface 2 */
    ALT_DMA_PERIPH_FPGA_3             = 3,  /*!< FPGA soft IP interface 3 */

    ALT_DMA_PERIPH_FPGA_4_OR_CAN0_IF1 = 4,  /*!< Selectively MUXed FPGA 4 or CAN 0 interface 1 */
    ALT_DMA_PERIPH_FPGA_5_OR_CAN0_IF2 = 5,  /*!< Selectively MUXed FPGA 5 or CAN 0 interface 2 */
    ALT_DMA_PERIPH_FPGA_6_OR_CAN1_IF1 = 6,  /*!< Selectively MUXed FPGA 6 or CAN 1 interface 1 */
    ALT_DMA_PERIPH_FPGA_7_OR_CAN1_IF2 = 7,  /*!< Selectively MUXed FPGA 7 or CAN 1 interface 2 */

    ALT_DMA_PERIPH_FPGA_4             = 4,  /*!< Alias for ALT_DMA_PERIPH_FPGA_4_OR_CAN0_IF1 */
    ALT_DMA_PERIPH_FPGA_5             = 5,  /*!< Alias for ALT_DMA_PERIPH_FPGA_5_OR_CAN0_IF2 */
    ALT_DMA_PERIPH_FPGA_6             = 6,  /*!< Alias for ALT_DMA_PERIPH_FPGA_6_OR_CAN1_IF1 */
    ALT_DMA_PERIPH_FPGA_7             = 7,  /*!< Alias for ALT_DMA_PERIPH_FPGA_7_OR_CAN1_IF2 */

    ALT_DMA_PERIPH_CAN0_IF1           = 4,  /*!< Alias for ALT_DMA_PERIPH_FPGA_4_OR_CAN0_IF1 */
    ALT_DMA_PERIPH_CAN0_IF2           = 5,  /*!< Alias for ALT_DMA_PERIPH_FPGA_5_OR_CAN0_IF2 */
    ALT_DMA_PERIPH_CAN1_IF1           = 6,  /*!< Alias for ALT_DMA_PERIPH_FPGA_6_OR_CAN1_IF1 */
    ALT_DMA_PERIPH_CAN1_IF2           = 7,  /*!< Alias for ALT_DMA_PERIPH_FPGA_7_OR_CAN1_IF2 */

    ALT_DMA_PERIPH_I2C0_TX            = 8,  /*!< I<sup>2</sup>C 0 TX */
    ALT_DMA_PERIPH_I2C0_RX            = 9,  /*!< I<sup>2</sup>C 0 RX */
    ALT_DMA_PERIPH_I2C1_TX            = 10, /*!< I<sup>2</sup>C 1 TX */
    ALT_DMA_PERIPH_I2C1_RX            = 11, /*!< I<sup>2</sup>C 1 RX */
    ALT_DMA_PERIPH_I2C2_TX            = 12, /*!< I<sup>2</sup>C 2 TX */
    ALT_DMA_PERIPH_I2C2_RX            = 13, /*!< I<sup>2</sup>C 2 RX */
    ALT_DMA_PERIPH_I2C3_TX            = 14, /*!< I<sup>2</sup>C 3 TX */
    ALT_DMA_PERIPH_I2C3_RX            = 15, /*!< I<sup>2</sup>C 3 RX */
    ALT_DMA_PERIPH_SPI0_MASTER_TX     = 16, /*!< SPI 0 Master TX */
    ALT_DMA_PERIPH_SPI0_MASTER_RX     = 17, /*!< SPI 0 Master RX */
    ALT_DMA_PERIPH_SPI0_SLAVE_TX      = 18, /*!< SPI 0 Slave TX */
    ALT_DMA_PERIPH_SPI0_SLAVE_RX      = 19, /*!< SPI 0 Slave RX */
    ALT_DMA_PERIPH_SPI1_MASTER_TX     = 20, /*!< SPI 1 Master TX */
    ALT_DMA_PERIPH_SPI1_MASTER_RX     = 21, /*!< SPI 1 Master RX */
    ALT_DMA_PERIPH_SPI1_SLAVE_TX      = 22, /*!< SPI 1 Slave TX */
    ALT_DMA_PERIPH_SPI1_SLAVE_RX      = 23, /*!< SPI 1 Slave RX */
    ALT_DMA_PERIPH_QSPI_FLASH_TX      = 24, /*!< QSPI Flash TX */
    ALT_DMA_PERIPH_QSPI_FLASH_RX      = 25, /*!< QSPI Flash RX */
    ALT_DMA_PERIPH_STM                = 26, /*!< System Trace Macrocell */
    ALT_DMA_PERIPH_RESERVED           = 27, /*!< Reserved */
    ALT_DMA_PERIPH_UART0_TX           = 28, /*!< UART 0 TX */
    ALT_DMA_PERIPH_UART0_RX           = 29, /*!< UART 0 RX */
    ALT_DMA_PERIPH_UART1_TX           = 30, /*!< UART 1 TX */
    ALT_DMA_PERIPH_UART1_RX           = 31  /*!< UART 1 RX */
}
ALT_DMA_PERIPH_t;

#ifdef __cplusplus
}
#endif

#endif /* __ALT_DMA_PERIPH_H__ */
