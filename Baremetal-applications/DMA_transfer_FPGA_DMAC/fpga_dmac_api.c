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
