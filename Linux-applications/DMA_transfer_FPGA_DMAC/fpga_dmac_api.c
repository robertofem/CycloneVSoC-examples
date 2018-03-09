//API for the Qsys DMA Controller
#include "fpga_dmac_api.h"

//-----------------Generic functions--------------------//
//(Addresses are multiplied by 4 because the peripheral has 32-bit (4byte) regs
uint32_t fpga_dma_read_reg(void* addr, uint32_t reg)
{
  return *((uint32_t*) (addr + 4*reg));
}

void fpga_dma_write_reg(void* addr, uint32_t reg, uint32_t val)
{
  *((uint32_t*) (addr + 4*reg)) = val;
}

uint32_t fpga_dma_read_bit(void* addr, uint32_t reg, uint32_t bit)
{
    return (bit & fpga_dma_read_reg(addr, reg));
}

void fpga_dma_write_bit(void* addr, uint32_t reg, uint32_t bit, uint32_t val)
{
  uint32_t old = fpga_dma_read_reg(addr, reg);
  if(val == 0)
  {
    fpga_dma_write_reg(addr, reg, (old & (~bit)));
  }
  else if(val == 1)
  {
    fpga_dma_write_reg(addr, reg, (old | bit));
  }
  return;
}

// //--------Basic functions to read each register---------//
// uint32_t fpga_dma_read_status(void* address)
// {
//   return *((uint32_t*) (address + 4*FPGA_DMA_STATUS);
// }
// void fpga_dma_write_status(void* address, uint32_t status)
// {
//   *((uint32_t*) (address + 4*FPGA_DMA_STATUS) = status;
//   return;
// }
// uint32_t fpga_dma_read_readaddress(void* address)
// {
//   return *((uint32_t*) (address + 4*FPGA_DMA_READADDRESS);
// }
// void fpga_dma_write_readaddress(void* address, uint32_t readaddress)
// {
//   *((uint32_t*) (address + 4*FPGA_DMA_READADDRESS) = readaddress;
//   return;
// }
// uint32_t fpga_dma_read_writeaddress(void* address)
// {
//   return *((uint32_t*) (address + 4*FPGA_DMA_WRITEADDRESS);
// }
// void fpga_dma_write_writeaddress(void* address, uint32_t writeaddress)
// {
//   *((uint32_t*) (address + 4*FPGA_DMA_WRITEADDRESS) = writeaddress;
//   return;
// }
// uint32_t fpga_dma_read_length(void* address)
// {
//   return *((uint32_t*) (address + 4*FPGA_DMA_LENGTH);
// }
// void fpga_dma_write_length(void* address, uint32_t length)
// {
//   *((uint32_t*) (address + 4*FPGA_DMA_LENGTH) = length;
//   return;
// }
// uint32_t fpga_dma_read_control(void* address)
// {
//   return *((uint32_t*) (address + 4*FPGA_DMA_COTROL);
// }
// void fpga_dma_write_control(void* address, uint32_t control)
// {
//   *((uint32_t*) (address + 4*FPGA_DMA_COTROL) = control;
//   return;
// }
//
//
// //------Extra functions to access individual bits------//
// uint32_t fpga_dma_read_done(void* address)
// {
//   return (FPGA_DMA_DONE & fpga_dma_read_status(address));
// }
// void fpga_dma_write_done(void* address, uint32_t value)
// {
//   uint32_t old_status = fpga_dma_read_status(address);
//   if(value = 0)
//   {
//     fpga_dma_write_status(address, (old_status & (~FPGA_DMA_DONE)))
//   }
//   else if(value = 1)
//   {
//     fpga_dma_write_status(address, (old_status | FPGA_DMA_DONE))
//   }
//   return;
// }
// uint32_t fpga_dma_read_go(void* address)
// {
//   return (FPGA_DMA_GO & fpga_dma_read_control(address));
// }
// void fpga_dma_write_go(void* address, uint32_t)
// {
//   uint32_t old_control = fpga_dma_read_control(address);
//   if(value = 0)
//   {
//     fpga_dma_write_control(address, (old_control & (~FPGA_DMA_GO)))
//   }
//   else if(value = 1)
//   {
//     fpga_dma_write_control(address, (old_control | FPGA_DMA_GO))
//   }
//   return;
// }
