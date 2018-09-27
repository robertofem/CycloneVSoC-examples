/******************************************************************************
*
* Author: Roberto Fernandez Molanes (robertofem@gmail.com)
* University of Vigo
*
* 09 March 2018
*
* This is a simple example showing how to use a DMA in the FPGA and the
* alloc_dmable_buff driver to move data between and On-Chip RAM in the
* FPGA and a buffer in application.
* The DMAC component is the basic(non scatter-gather) DMA Controller
* available in Qsys. The files fpga_dmac_api.h and fpga_dmac_api.c are a bunch
* of macros and functions to control that DMA controller.
* In this particular example the DMA controller copies a buffer in a FPGA
* On-Chip RAM (FPGA-OCR) to a buffer in the program. Therefore the FPGA-OCR
* is connected to the read port of the DMAC and the FPGA-to-HPS bridge is
* connected to the write port of the DMAC.
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>

#include "fpga_dmac_api.h"

#define DMA_TRANSFER_SIZE 	32

/**************************SOME MACROS TO EASE PROGRAMMING*******************/
//Constants to do mmap and get access to FPGA and HPS peripherals
#define HPS_FPGA_BRIDGE_BASE 0xC0000000
#define HW_REGS_BASE ( HPS_FPGA_BRIDGE_BASE )
#define HW_REGS_SPAN ( 0x40000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

//The address of FPGA-DMAC is the HPS-FPGA bridge + Qsys address assigned to it
#define FPGA_DMAC_QSYS_ADDRESS 0x10000
#define FPGA_DMAC_ADDRESS ((uint8_t*)0xC0000000+FPGA_DMAC_QSYS_ADDRESS)
//Address of the On-Chip RAM in the FPGA, as seen by processor
#define FPGA_OCR_QSYS_ADDRESS_UP 0x0
#define FPGA_OCR_ADDRESS_UP ((uint8_t*)0xC0000000+FPGA_OCR_QSYS_ADDRESS_UP)
//Address of the On-Chip RAM in the FPGA, as seen by DMAC
#define FPGA_OCR_ADDRESS_DMAC 0x0
//GPIO to change AXI AXI_SIGNALS
#define GPIO_QSYS_ADDRESS 0x20000
#define GPIO_ADDRESS_UP ((uint8_t*)0xC0000000 + GPIO_QSYS_ADDRESS)

//DMAC transfer addresses (from processor they are virtual addresses)

#define DMA_TRANSFER_SRC_DMAC   ((uint8_t*) FPGA_OCR_ADDRESS_DMAC)
#define DMA_TRANSFER_SRC_UP     ((uint8_t*) FPGA_OCR_vaddr)
#define DMA_TRANSFER_DST_DMAC   ((uint8_t*) intermediate_buff_phys + 0x80000000) //through ACP
#define DMA_TRANSFER_DST_UP     ((uint8_t*) buff)


void printbuff(uint8_t* buff, int size)
{
  int i;
  printf("[");
  for (i=0; i<size; i++)
  {
    printf("%u",buff[i]);
    if (i<(size-1)) printf(",");
  }
  printf("]");
  printf("\n");
}

int main() {
  int i;
  uint32_t AXI_SIGNALS;

  //-------GENERATE VIRTUAL ADRESSES TO ACCESS FPGA FROM PROCESSOR---------//
  void *virtual_base;
  int fd;
  if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
	  printf( "ERROR: could not open \"/dev/mem\"...\n" );
	  return( 1 );
  }
  //mmap from 0xC0000000 to 0xFFFFFFFF (1GB): FPGA and HPS peripherals
  virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ),
    MAP_SHARED, fd, HW_REGS_BASE );

  if( virtual_base == MAP_FAILED ) {
	  printf( "ERROR: mmap() failed...\n" );
	  close( fd );
	  return( 1 );
  }

  //virtual addresses for all components
  void *FPGA_DMA_vaddr_void = virtual_base
  + ((unsigned long)(FPGA_DMAC_QSYS_ADDRESS) & (unsigned long)( HW_REGS_MASK ));

  void *FPGA_OCR_vaddr_void = virtual_base
  + ((unsigned long)(FPGA_OCR_QSYS_ADDRESS_UP) & (unsigned long)( HW_REGS_MASK ));
  uint8_t* FPGA_OCR_vaddr = (uint8_t *) FPGA_OCR_vaddr_void;

  void *AXI_GPIO_vaddr_void = virtual_base
  + ((unsigned long)(GPIO_QSYS_ADDRESS) & (unsigned long)( HW_REGS_MASK ));
  uint32_t* AXI_GPIO_vaddr = (uint32_t *) AXI_GPIO_vaddr_void;


  //-----CHECK IF THE FPGA OCR IS ACCESSIBLE AND RESET IT------//
  //Check the on-chip RAM memory in the FPGA (Source of the DMA transfer)
  uint8_t* fpga_ocr_ptr = FPGA_OCR_vaddr;
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    *fpga_ocr_ptr = (uint8_t) i;
    if (*fpga_ocr_ptr != (uint8_t)i)
    {
      printf ("Error when checking FPGA On-Chip RAM in Byte %d\n", i);
      return 0;
    }
    fpga_ocr_ptr++;
  }
  printf("Check FPGA On-Chip RAM OK\n");

  //Reset FPGA-OCR
  fpga_ocr_ptr = FPGA_OCR_vaddr;
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    *fpga_ocr_ptr = 0;
    if (*fpga_ocr_ptr != 0)
    {
      printf ("Error when resetting FPGA On-Chip RAM in Byte %d\n", i);
      return 0;
    }
    fpga_ocr_ptr++;
  }
  printf("Reset FPGA On-Chip RAM OK\n");

  //------DEFINE AXI SIGNALS THAT CAN AFFECT THE TRANSACTION-------//
  //AXI_SIGNALS[3-0]  = AWCACHE = 0111 (Cacheable write-back, allocate reads only)
  //AXI_SIGNALS[6-4]  = AWPROT = 000 (normal access, non-secure, data)
  //AXI_SIGNALS[11-7] = AWUSER = 00001 (Coherent access)
  //AXI_SIGNALS[19-16]  = ARCACHE = 0111
  //AXI_SIGNALS[22-20]  = ARPROT = 000
  //AXI_SIGNALS[27-23] = ARUSER = 00001
  //AXI_SIGNALS = 0x00870087; //works for WR and RD and gives fastest accesses
  AXI_SIGNALS = 0x00870087;
  //Write data to the GPIO connected to AXI signals
  *AXI_GPIO_vaddr = AXI_SIGNALS;

  //----Allocate the Buffers to do the transfer-----//
  //Allocate the final buffer in application
  char buff[DMA_TRANSFER_SIZE];
  for (i=0; i<DMA_TRANSFER_SIZE; i++) buff[i] = 0;
  //Allocate an intermediate buffer to do the DMA transfer using the driver
    //Set size
    int f_sysfs;
    char d[14];
    printf("\nConfig. dmable_buff module using sysfs entries in /sys/alloc_dmable_buffers\n");
    sprintf(d, "%u", (uint32_t) DMA_TRANSFER_SIZE);
    f_sysfs = open("/sys/alloc_dmable_buffers/attributes/buff_size[0]", O_WRONLY);
    if (f_sysfs < 0){
      printf("Failed to open sysfs for buff_size.\n");
      return errno;
    }
    write (f_sysfs, &d, 14);
    close(f_sysfs);
    //Set cacheable (goes through ACP)
    sprintf(d, "%u", (uint32_t) 0);
    f_sysfs = open("/sys/alloc_dmable_buffers/attributes/buff_uncached[0]", O_WRONLY);
    if (f_sysfs < 0){
      printf("Failed to open sysfs for buff_uncached.\n");
      return errno;
    }
    write (f_sysfs, &d, 14);
    close(f_sysfs);
    //Open the entry to allocate the buffer
    int f_intermediate_buff = open("/dev/dmable_buff0",O_RDWR);
    if (f_intermediate_buff < 0){
      perror("Failed to open /dev/dmable_buff0...");
      return errno;
    }
    //get physical address
    sprintf(d, "%u", (uint32_t) 0);
    f_sysfs = open("/sys/alloc_dmable_buffers/attributes/phys_buff[0]", O_RDONLY);
    if (f_sysfs < 0){
      printf("Failed to open sysfs for phys_buff.\n");
      return errno;
    }
    read(f_sysfs, d, 14);
    //fgets(d, 14, )
    unsigned int intermediate_buff_phys = 0;
    printf("d=%s, phys=%x", d, intermediate_buff_phys);
    intermediate_buff_phys = strtoul (d, NULL, 0);
    printf("d=%s, phys=%x", d, intermediate_buff_phys);
    close(f_sysfs);

  //-----------------DO DMA TRANSFER USING THE FPGA-DMAC-----------------//
  //Fill uP buffer and show uP and FPGA buffers
  printf("\nWRITE: Copy %d Bytes from FPGA OCR (%x) to Intermediate Buff (%x)\n",
    (int) DMA_TRANSFER_SIZE, (unsigned int) DMA_TRANSFER_SRC_UP,
    (unsigned int) intermediate_buff_phys);

  for (i=0; i<DMA_TRANSFER_SIZE;i++) DMA_TRANSFER_SRC_UP[i] = (uint8_t)rand();
  printf("FPGA OCR before DMA transfer = ");
  printbuff(DMA_TRANSFER_SRC_UP, DMA_TRANSFER_SIZE);
  printf("Buffer before DMA transfer = ");
  printbuff(DMA_TRANSFER_DST_UP, DMA_TRANSFER_SIZE);

  //Do the transfer using the DMA in the FPGA
  printf("Initializing DMA Controller\n");
  fpga_dma_write_reg( FPGA_DMA_vaddr_void,
                      FPGA_DMA_CONTROL,
                      FPGA_DMA_WORD_TRANSFERS |
                      FPGA_DMA_END_WHEN_LENGHT_ZERO
                    );
  fpga_dma_write_reg( FPGA_DMA_vaddr_void,   //set source address
                      FPGA_DMA_READADDRESS,
                      (uint32_t) DMA_TRANSFER_SRC_DMAC);
  fpga_dma_write_reg( FPGA_DMA_vaddr_void,  //set destiny address
                      FPGA_DMA_WRITEADDRESS,
                      (uint32_t) DMA_TRANSFER_DST_DMAC);
  fpga_dma_write_reg( FPGA_DMA_vaddr_void, //set transfer size
                      FPGA_DMA_LENGTH,
                      DMA_TRANSFER_SIZE);
  fpga_dma_write_bit( FPGA_DMA_vaddr_void, //clean the done bit
                      FPGA_DMA_STATUS,
                      FPGA_DMA_DONE,
                      0);

  //printf("Start DMA Transfer\n");
  fpga_dma_write_bit( FPGA_DMA_vaddr_void,//start transfer
                      FPGA_DMA_CONTROL,
                      FPGA_DMA_GO,
                      1);
  while(fpga_dma_read_bit(FPGA_DMA_vaddr_void, FPGA_DMA_STATUS,
    FPGA_DMA_DONE)==0) {}
  printf("DMA Transfer Finished\n");

  //Copy intermediate buffer into buffer
  read(f_intermediate_buff, buff, DMA_TRANSFER_SIZE);

    //print the result of the Write
  printf("FPGA OCR after DMA transfer = ");
  printbuff(DMA_TRANSFER_SRC_UP, DMA_TRANSFER_SIZE);
  printf("Buffer after DMA transfer = ");
  printbuff(DMA_TRANSFER_DST_UP, DMA_TRANSFER_SIZE);
	if(memcmp(DMA_TRANSFER_SRC_UP, DMA_TRANSFER_DST_UP,(size_t)DMA_TRANSFER_SIZE)==0)
    printf("Write Successful!\n");
  else
    printf("Write Error. Buffers are not equal\n");

	// --------------clean up our memory mapping and exit -----------------//
  close(f_intermediate_buff);

	if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	close( fd );

	return( 0 );
}
