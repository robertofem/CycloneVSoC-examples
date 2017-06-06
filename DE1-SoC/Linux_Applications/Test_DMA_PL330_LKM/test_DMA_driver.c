//#include <time.h>
//#include <librt.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include<errno.h>
//#include <math.h>

#include "configuration.h"

#include "hwlib/hwlib.h"
#include "hwlib/soc_cv_av/socal/socal.h"
#include "hwlib/soc_cv_av/socal/hps.h"

#include <time.h>

int main() {

	#ifdef ON_CHIP_RAM_ON_LIGHTWEIGHT
	  printf("TESTING ON_CHIP_RAM_ON_LIGHTWEIGHT\n");
	  printf("Size of uint32_soc is %d\n", sizeof(uint32_soc));
	#endif //ON_CHIP_RAM_ON_LIGHTWEIGHT
	#ifdef ON_CHIP_RAM_ON_HFBRIDGE32
	  printf("TESTING ON_CHIP_RAM_ON_HFBRIDGE32\n");
	  printf("Size of uint32_soc is %d\n", sizeof(uint32_soc));
	#endif //ON_CHIP_RAM_ON_HFBRIDGE32
	#ifdef ON_CHIP_RAM_ON_HFBRIDGE64
	  printf("TESTING ON_CHIP_RAM_ON_HFBRIDGE64\n");
	  printf("Size of uint64_soc is %d\n", sizeof(uint64_soc));
	#endif //ON_CHIP_RAM_ON_HFBRIDGE64
	#ifdef ON_CHIP_RAM_ON_HFBRIDGE128
	  printf("TESTING ON_CHIP_RAM_ON_HFBRIDGE128\n");
	  printf("Size of uint128_t is %d\n", sizeof(uint128_soc));
	#endif //ON_CHIP_RAM_ON_HFBRIDGE128

	int error;
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

	virtual_base = mmap( NULL, MMAP_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, MMAP_BASE );

	if( virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	void *on_chip_RAM_addr_void = virtual_base + ( ( unsigned long  )( BRIDGE_ADRESS_MAP_START +  ON_CHIP_MEMORY_BASE) & ( unsigned long)( MMAP_MASK ) );
	UINT_SOC* on_chip_RAM_addr = (UINT_SOC *) on_chip_RAM_addr_void;


	//---------CHECK IF THE FPGA OCR IS ACCESSIBLE AND RESET IT-------//
	//Check the on-chip RAM memory in the FPGA
	UINT_SOC* ocr_ptr = on_chip_RAM_addr;
	error = 0;
	for (i=0; i<MEMORY_SIZE_IN_WORDS; i++)
	{
		*ocr_ptr = (UINT_SOC)i;
		if (*ocr_ptr != (UINT_SOC)i) error = 1;
		ocr_ptr++;
	}

	if (error == 0) printf("Check On-Chip RAM OK\n");
	else
	{
		printf ("Error when checking On-Chip RAM\n");
		return 1;
	}

	//Reset all memory
	ocr_ptr = on_chip_RAM_addr;
	error = 0;
	for (i=0; i<MEMORY_SIZE_IN_WORDS; i++)
	{
		*ocr_ptr = 0;
		if (*ocr_ptr != 0) error = 1;
		ocr_ptr++;
	}
	if (error == 0) printf("Reset On-Chip RAM OK\n");
	else
	{
		printf ("Error when resetting On-Chip RAM\n");
		return 0;
	}

	//-------------MAKE A TRANSFER USING DMA DRIVER------------//
	//Change value of sysfs entries
	int f_sysfs;	
  	char d[7] = "320021";
  	f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/dma_buff_padd", O_WRONLY);
	if (f_sysfs < 0){
	  printf("Failed to open sysfs.\n");
	  return errno;
	}
	else
	{
	  printf("Open sysfs correct.\n");
	}
  	write (f_sysfs, &d, 6);
  	close(f_sysfs);	
	
	/*
	//Open read, write and close
	#define BUFF_LEN 4
	char buffer[BUFF_LEN];
	for (i=0; i<BUFF_LEN;i++) buffer[i] = i;
	
	int f=open("/dev/dma_ch0",O_RDWR);
	if (f < 0){
	  perror("Failed to open the device...");
	  return errno;
	}
	
	int ret = write(f, buffer, BUFF_LEN);
	if (ret < 0){
	  perror("Failed to write the message to the device.");
	  return errno;
	}
	
	close(f);
	*/
	
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
	if( munmap( virtual_base, MMAP_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	close( fd );

	return( 0 );
}
