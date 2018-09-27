//API for the Qsys DMA Controller

#ifndef __FPGA_DMAC_API__
#define __FPGA_DMAC_API__

#include <inttypes.h>
#include <stdlib.h>

//REGISTER MAP
#define FPGA_DMA_STATUS         0
#define FPGA_DMA_READADDRESS    1
#define FPGA_DMA_WRITEADDRESS   2
#define FPGA_DMA_LENGTH         3
//RESERVED                      4
//RESERVED                      5
#define FPGA_DMA_CONTROL        6
//RESERVED                      7

//MACROS to more easily read the control status register bits
#define FPGA_DMA_DONE                    0b00001 //DONE
#define FPGA_DMA_BUSY                    0b00010 //BUSY
#define FPGA_DMA_REOP                    0b00100 //REOP
#define FPGA_DMA_WEOP                    0b01000 //WEOP
#define FPGA_DMA_LEN                     0b10000 //LEN

//MACROS to more easily program the control register
#define FPGA_DMA_BYTE_TRANSFERS          0b0000000000001 //BYTE
#define FPGA_DMA_HALFWORD_TRANSFERS      0b0000000000010 //HW
#define FPGA_DMA_WORD_TRANSFERS          0b0000000000100 //WORD
#define FPGA_DMA_GO                      0b0000000001000 //GO
#define FPGA_DMA_INTERRUPT_ENABLE        0b0000000010000 //I_EN
#define FPGA_DMA_END_RD_END_OF_PACKET    0b0000000100000 //REEN
#define FPGA_DMA_END_WR_END_OF_PACKET    0b0000001000000 //WEEN
#define FPGA_DMA_END_WHEN_LENGHT_ZERO    0b0000010000000 //LEEN
#define FPGA_DMA_READ_CONSTANT_ADDR      0b0000100000000 //RCON
#define FPGA_DMA_WRITE_CONSTANT_ADDR     0b0001000000000 //WCON
#define FPGA_DMA_DOUBLEWORD_TRANSFERS    0b0010000000000 //DOUBLEWORD
#define FPGA_DMA_QUADWORD_TRANSFERS      0b0100000000000 //QUADWORD
#define FPGA_DMA_SOFTWARE_RESET          0b1000000000000 //SOFTWARE_RESET

//-----------------Generic functions--------------------//
uint32_t fpga_dma_read_reg(void* addr, uint32_t reg);
void fpga_dma_write_reg(void* addr, uint32_t reg, uint32_t val);
uint32_t fpga_dma_read_bit(void* addr, uint32_t reg, uint32_t bit);
void fpga_dma_write_bit(void* addr, uint32_t reg, uint32_t bit, uint32_t val);

//------------Some specific functions-------------------//
void fpga_dma_init();
void fpga_dma_config_transfer(void* addr, void* src, void* dst, unsigned int size);
void fpga_dma_start_transfer(void* addr);
uint32_t fpga_dma_transfer_done(void* addr);
void* align_malloc (size_t size, void** unaligned_addr);

#endif // __FPGA_DMAC_API__
