//#include <time.h>
//#include <librt.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>

//#include <math.h>

//#include "hwlib/hwlib.h"
//#include "hwlib/soc_cv_av/socal/socal.h"
//#include "hwlib/soc_cv_av/socal/hps.h"

//#include <time.h>

//Constants to do mmap and get access to FPGA peripherals
#define HPS_FPGA_BRIDGE_BASE 0xC0000000
#define HW_REGS_BASE ( HPS_FPGA_BRIDGE_BASE )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )
#define ON_CHIP_MEMORY_BASE 0   //addres of FPGA On-Chip RAM relative to HPS-FPGA bridge

//MACROS TO CONTROL THE TRANSFER
#define DMA_TRANSFER_SIZE 	32
#define USE_ACP			0  //0 do not use acp, 1 use acp
//physical address of the buffer used when reading and writing using dma driver
//in this case we set 0xC0000000, the beginning of the HPS-FPGA BRIDGE
//In this address there should be a memory with enough space to do the DMA transfer
#define DMA_BUFF_PADD	(HPS_FPGA_BRIDGE_BASE + ON_CHIP_MEMORY_BASE)
#define PREPARE_MICROCODE_WHEN_OPEN 0 //0 prepare microcode when write or read
//1 prepare microcode when open. It saves microcode preparation time later when calling rean and write


void printbuff(char* buff, int size)
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
    
  //---------GENERATE ADRESSES TO ACCESS FPGA MEMORY FROM PROCESSOR----------//
  // map the address space for the LED registers into user space so we can interact with them.
  // we'll actually map in the entire CSR span of the HPS since we want to access various registers within that span
  void *virtual_base;
  int fd;
  if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
	  printf( "ERROR: could not open \"/dev/mem\"...\n" );
	  return( 1 );
  }

  virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );

  if( virtual_base == MAP_FAILED ) {
	  printf( "ERROR: mmap() failed...\n" );
	  close( fd );
	  return( 1 );
  }

  //virtual address of the FPGA buffer
  void *on_chip_RAM_addr_void = virtual_base + ( ( unsigned long  )(ON_CHIP_MEMORY_BASE) & ( unsigned long)( HW_REGS_MASK ) );
  uint8_t* on_chip_RAM_addr = (uint8_t *) on_chip_RAM_addr_void;

  //---------CHECK IF THE FPGA OCR IS ACCESSIBLE AND RESET IT-------//
  //Check the on-chip RAM memory in the FPGA
  uint8_t* ocr_ptr = on_chip_RAM_addr;
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    *ocr_ptr = (uint8_t) i;
    if (*ocr_ptr != (uint8_t)i)
    {
      printf ("Error when checking On-Chip RAM in Byte %d\n", i);
      return 0;
    }
    ocr_ptr++;
  }
  printf("Check On-Chip RAM OK\n");
  

  //Reset all memory
  ocr_ptr = on_chip_RAM_addr;
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    *ocr_ptr = 0;
    if (*ocr_ptr != 0)
    {
      printf ("Error when resetting On-Chip RAM in Byte %d\n", i);
      return 0;
    }
    ocr_ptr++;
  }
  printf("Reset On-Chip RAM OK\n");
 
  //-------------MAKE SOME TRANSFERS USING DMA DRIVER------------//
  //Configure the driver through sysfs
  int f_sysfs;
  char d[9];
  
  sprintf(d, "%u", (uint32_t) DMA_BUFF_PADD);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/dma_buff_padd", O_WRONLY);
  if (f_sysfs < 0){
    printf("Failed to open sysfs for dma_buff_padd.\n");
    return errno;
  }
  write (f_sysfs, &d, 9);
  close(f_sysfs);	
  
  sprintf(d, "%d", (int) USE_ACP);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/use_acp", O_WRONLY);
  if (f_sysfs < 0){
    printf("Failed to open sysfs for use_acp.\n");
    return errno;
  }
  write (f_sysfs, &d, 9);
  close(f_sysfs);
  
  sprintf(d, "%d", (int) PREPARE_MICROCODE_WHEN_OPEN);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/prepare_microcode_in_open", O_WRONLY);
  if (f_sysfs < 0){
    printf("Failed to open sysfs for prepare_microcode_in_open.\n");
    return errno;
  }
  write (f_sysfs, &d,9);
  close(f_sysfs);
  
  sprintf(d, "%d", (int) DMA_TRANSFER_SIZE);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/dma_transfer_size", O_WRONLY);
  if (f_sysfs < 0){
    printf("Failed to open sysfs for dma_transfer_size.\n");
    return errno;
  }
  write (f_sysfs, &d, 9);
  close(f_sysfs);
  
  //Fill uP buffer and show uP and FPGA buffers
  char buffer[DMA_TRANSFER_SIZE];
  for (i=0; i<DMA_TRANSFER_SIZE;i++) buffer[i] = 2;
  printf("uP   buff before WR = "); printbuff(buffer, DMA_TRANSFER_SIZE);
  printf("FPGA buff before WR = "); printbuff(DMA_BUFF_PADD, DMA_TRANSFER_SIZE);
	printf("Copy uP buff to FPGA buff \n");

  //Write uP fuffer to FPGA
  int f=open("/dev/dma_pl330",O_RDWR);
  if (f < 0){
    perror("Failed to open /dev/dma_pl330 on write...");
    return errno;
  }
	int ret = write(f, buffer, DMA_TRANSFER_SIZE);
	if (ret < 0){
	  perror("Failed to write the message to the device.");
	  return errno;
	}
	close(f);
	
  printf("uP   buff before WR = "); printbuff(buffer, DMA_TRANSFER_SIZE);
  printf("FPGA buff before WR = "); printbuff(DMA_BUFF_PADD, DMA_TRANSFER_SIZE);
	
	/*
	
			//check the content of the data just read
			error = 0;
			for (j=0; j<data_size[i]/BYTES_DATA_BUS; j++)
			{
				if (datai[j] != i) error = 1;
			}
			if (error != 0)
			{
				printf ("Error when checking On-Chip RAM with data size %dB\n", data_size[i]);
				return 0;

			}
			//else printf("Check On-Chip RAM with data size %dB was OK\n", data_size[i]);

*/

	// ------clean up our memory mapping and exit -------//
	if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	close( fd );

	return( 0 );
}
