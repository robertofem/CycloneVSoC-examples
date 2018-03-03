//API for the Qsys DMA Controller

#ifndef __FPGA_DMAC_API__
#define __FPGA_DMAC_API__

#include <inttypes.h>

//REGISTER MAP
#define FPGA_DMA_STATUS         0
#define FPGA_DMA_READADDRESS    1
#define FPGA_DMA_WRITEADDRESS   2
#define FPGA_DMA_LENGTH         3
//RESERVED                      4
//RESERVED                      5
#define FPGA_DMA_CONTROL        6
//RESERVED                      7

//MACROS to more easily program the control status register
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
#define FPGA_DMA_DOUBLEWORD_TRANSFER     0b0010000000000 //DOUBLEWORD
#define FPGA_DMA_QUADWORD_TRANSFER       0b0100000000000 //QUADWORD
#define FPGA_DMA_SOFTWARE_RESET          0b1000000000000 //SOFTWARE_RESET

//-----------------Generic functions--------------------//
uint32_t fpga_dma_read_reg(void* addr, uint32_t reg);
void fpga_dma_write_reg(void* addr, uint32_t reg, uint32_t val);
uint32_t fpga_dma_read_bit(void* addr, uint32_t reg, uint32_t bit);
void fpga_dma_write_bit(void* addr, uint32_t reg, uint32_t bit, uint32_t val);

// //--------Basic functions to read each register---------//
// uint32_t fpga_dma_read_status(void* address);
// void fpga_dma_write_status(void* address, uint32_t status);
// uint32_t fpga_dma_read_readaddress(void* address);
// void fpga_dma_write_readaddress(void* address, uint32_t readaddress);
// uint32_t fpga_dma_read_writeaddress(void* address);
// void fpga_dma_write_writeaddress(void* address, uint32_t writeaddress);
// uint32_t fpga_dma_read_length(void* address);
// void fpga_dma_write_length(void* address, uint32_t length);
// uint32_t fpga_dma_read_control(void* address);
// void fpga_dma_write_control(void* address, uint32_t control);
//
// //------Extra functions to access individual bits------//
// uint32_t fpga_dma_read_done(void* address);
// void fpga_dma_write_done(void* address, uint32_t);
// uint32_t fpga_dma_read_go(void* address);
// void fpga_dma_write_go(void* address, uint32_t);

#endif // __FPGA_DMAC_API__
