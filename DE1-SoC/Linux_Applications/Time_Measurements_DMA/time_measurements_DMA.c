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

#include "pmu.h" //to measure time with PMU

//Constants to do mmap and get access to FPGA peripherals
#define HPS_FPGA_BRIDGE_BASE 0xC0000000
#define HW_REGS_BASE ( HPS_FPGA_BRIDGE_BASE )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )
#define ON_CHIP_MEMORY_BASE 0 //FPGA On-Chip RAM address relative to H2F bridge

//Constants for the time experiments
#define REP_TESTS 100 //repetitions of each time experiment
#define CLK_REP_TESTS 1000 //repetitions to get clock statistics
#define ON_CHIP_MEMORY_SPAN 262144 //FPGA On-Chip RAM size in Bytes
//DMA_BUFF_PADD: Physical address of the FPGA On-Chip RAM
#define DMA_BUFF_PADD (HPS_FPGA_BRIDGE_BASE + ON_CHIP_MEMORY_BASE)  

//Declaration of some extra functions
void printbuff(char* buff, int size);
void reset_cumulative(unsigned long long int * total, 
  unsigned long long int* min, unsigned long long int * max, 
  unsigned long long int * variance);
void update_cumulative(unsigned long long int * total,  
  unsigned long long int* min, unsigned long long int * max, 
  unsigned long long int * variance, unsigned long long int ns_begin, 
  unsigned long long int ns_end, unsigned long long int clk_read_delay);
unsigned long long variance (unsigned long long variance , 
  unsigned long long total, unsigned long long rep_tests);

int main() {
  int i,j,l,k; //for loops
  int f; //file pointer to open /dev/dma_pl330
  int ret; //return value for wr and rd /dev/dma_pl330
  int use_acp, prepare_microcode_in_open; //to config of DMA_PL330_LKM

  //----Save intermediate results in speed tests-------//
  unsigned long long int total_clk, min_clk, max_clk, variance_clk, 
  clk_read_average;

  unsigned long long int total_dma_rd, min_dma_rd, max_dma_rd, variance_dma_rd;
  unsigned long long int total_dma_wr, min_dma_wr, max_dma_wr, variance_dma_wr;
  
  //-----------some variables to define speed tests-------//
  //define data sizes
  int number_of_data_sizes = 21; //size of data_size[]
  int data_size [] =
  {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
  2048, 4096, 8192, 16384, 32768,
  65536, 131072, 262144, 524288, 1048576,
  2097152}; //data size in bytes

  //-----------auxiliar variables to perform speed tests-------//
  char* data;
  int data_in_one_operation;//data to write or read from memory (bytes)
  int operation_loops; //number of loops to read/write all data from/to memory

  
  printf("---------DMA TIME MEASUREMENTS IN ANGSTROM---------\n");
  printf("Each measurement is repeated %d times\n\n", REP_TESTS);

  //-----------INITIALIZATION OF PMU AS TIMER------------//
  pmu_init_ns(800, 1); //Initialize PMU cycle cntr, 800MHz src, freq divider 1
  pmu_counter_enable();//Enable cycle counter inside PMU (it starts counting)
  float pmu_res = pmu_getres_ns();
  printf("PMU is used like timer with the following characteristics\n\r");
  printf("PMU cycle counter resolution is %f ns\n\r", pmu_res );
  printf("Measuring PMU counter overhead...\n\r");
  reset_cumulative(&total_clk, &min_clk, &max_clk, &variance_clk);
  int overflow = 0;
  unsigned long long pmu_counter_ns;
  for(i = 0; i<CLK_REP_TESTS+2; i++)
  {
    pmu_counter_reset();
    overflow = pmu_counter_read_ns(&pmu_counter_ns);
    //printf("PMU counter (ns): %lld \n\r", pmu_counter_ns);
    if (overflow == 1){
      printf("Cycle counter overflow!! Program ended\n\r");return 1;}
    if (i>=2) update_cumulative(&total_clk, &min_clk, &max_clk, &variance_clk, 
      0, pmu_counter_ns, 0);
    //(We erase two first measurements because they are different from the 
      //others. Reason:Branch prediction misses when entering for loop)
  }
  printf("PMU Cycle Timer Stats for %d consecutive reads\n\r", CLK_REP_TESTS);
  printf("Average, Minimum, Maximum, Variance\n\r");
  printf("%lld,%lld,%lld,%lld\n\r", clk_read_average = total_clk/CLK_REP_TESTS, 
    min_clk, max_clk, variance (variance_clk , total_clk, CLK_REP_TESTS));


  //-------GENERATE ADRESSES TO ACCESS FPGA MEMORY FROM PROCESSOR---------//
  // map the address space for the LED registers into user space so we can 
  //interact with them. we'll actually map in the entire CSR span of the HPS 
  //since we want to access various registers within that span
  void *virtual_base;
  int fd;
  if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
	  printf( "ERROR: could not open \"/dev/mem\"...\n" );
	  return( 1 );
  }

  virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), 
    MAP_SHARED, fd, HW_REGS_BASE );

  if( virtual_base == MAP_FAILED ) {
	  printf( "ERROR: mmap() failed...\n" );
	  close( fd );
	  return( 1 );
  }

  //virtual address of the FPGA buffer
  void *on_chip_RAM_vaddr_void = virtual_base 
  + ((unsigned long)(ON_CHIP_MEMORY_BASE) & (unsigned long)( HW_REGS_MASK ));
  uint8_t* on_chip_RAM_vaddr = (uint8_t *) on_chip_RAM_vaddr_void;


  //-----------CHECK IF THE FPGA OCR IS ACCESSIBLE AND RESET IT----------//
  //Check the on-chip RAM memory in the FPGA
  uint8_t* ocr_ptr = on_chip_RAM_vaddr;
  for (i=0; i<ON_CHIP_MEMORY_SPAN; i++)
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
  ocr_ptr = on_chip_RAM_vaddr;
  for (i=0; i<ON_CHIP_MEMORY_SPAN; i++)
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
 
  //-----------MOVING DATA WITH DMAC------------//
  printf("\n------MOVING DATA WITH THE DMA_PL330 driver-----\n");

  //Configure the hardware address of the buffer to use in DMA transactions
  //(the On-Chip RAM in FPGA) using sysfs
  int f_sysfs;
  char d[14];
  printf("\nConfig. DMA_PL330 module using sysfs entries in /sys/dma_pl330\n");
  sprintf(d, "%u", (uint32_t) DMA_BUFF_PADD);
  f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/dma_buff_padd", O_WRONLY);
  if (f_sysfs < 0)
  {
    printf("Failed to open sysfs for dma_buff_padd.\n");
    return errno;
  }
  write (f_sysfs, &d, 14);
  close(f_sysfs); 

  for(k=0; k<4; k++)//for each configuration of the DMA_PL330 driver
  {
    //Select configuration
    switch(k)
    {
      case 0:
        use_acp = 0;
        prepare_microcode_in_open = 0;
        printf("\n--DO NOT USE ACP, DO NOT REPARE DMAC MICROCODE IN OPEN--\n");
        break;
      case 1:
        use_acp = 0;
        prepare_microcode_in_open = 1;
        printf("\n--DO NOT USE ACP, PREPARE DMAC MICROCODE IN OPEN--\n");
        break;
      case 2:
        use_acp = 1;
        prepare_microcode_in_open = 0;
        printf("\n--USE ACP, DO NOT PREPARE DMAC MICROCODE IN OPEN--\n");
        break;
      case 3:
        use_acp = 1;
        prepare_microcode_in_open = 1;
        printf("\n--USE ACP, PREPARE DMAC MICROCODE IN OPEN--\n");
        break;
    }

    //Apply configuration to driver
    sprintf(d, "%d", (int) use_acp);
    f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/use_acp", O_WRONLY);
    if (f_sysfs < 0)
    {
      printf("Failed to open sysfs for use_acp.\n");
      return errno;
    }
    write (f_sysfs, &d, 14);
    close(f_sysfs);

    sprintf(d, "%d", (int) prepare_microcode_in_open);
    f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/prepare_microcode_in_open", 
      O_WRONLY);
    if (f_sysfs < 0)
    {
      printf("Failed to open sysfs for prepare_microcode_in_open.\n");
      return errno;
    }
    write (f_sysfs, &d,14);
    close(f_sysfs);

    printf("Data Size,Average DMA_WR,Minimum DMA_WR,Maximum DMA_WR,");
    printf("Variance DMA_WR,Average DMA_RD,Minimum DMA_RD,Maximum DMA_RD,");
    printf("Variance DMA_RD\n");

    for(i=0; i<number_of_data_sizes; i++)//for each data size
    {
            //reset variables for statistics
      reset_cumulative(&total_dma_wr,&min_dma_wr,&max_dma_wr, &variance_dma_wr);
      reset_cumulative(&total_dma_rd,&min_dma_rd,&max_dma_rd, &variance_dma_rd);

      //if data is bigger than on-chip size in bytes
      if (data_size[i]>ON_CHIP_MEMORY_SPAN)
      {
        data_in_one_operation = ON_CHIP_MEMORY_SPAN;
        operation_loops = data_size[i]/ON_CHIP_MEMORY_SPAN;
      }
      else
      {
        data_in_one_operation = data_size[i];
        operation_loops = 1;
      }   

      //configure the data size for the driver (only applies when microcode
      //is generated in open)
      sprintf(d, "%d", (int) data_in_one_operation);
      f_sysfs = open("/sys/dma_pl330/pl330_lkm_attrs/dma_transfer_size",
        O_WRONLY);
      if (f_sysfs < 0){
        printf("Failed to open sysfs for dma_transfer_size.\n");
        return errno;
      }
      write (f_sysfs, &d, 14);
      close(f_sysfs);

      //Open DMA driver to write and read from it
      f=open("/dev/dma_pl330",O_RDWR);
      if (f < 0)
      {
        perror("Failed to open /dev/dma_pl330...");
        return errno;
      }

      for(l = 0; l<REP_TESTS+2; l++)
      {
        //reserve the exact memory of the data size
        data = (char*) malloc(data_size[i]);
        if (data == 0)
        {
          printf("ERROR when calling malloc: Out of memory\n");
          return 1;
        }

        //fill uP memory with some data
        for(j=0; j<data_size[i]; j++) data[j] = i+1;          

        //--WRITE DATA TO FPGA ON-CHIP RAM
        pmu_counter_reset();
        for (j=0; j<operation_loops; j++)
        {
          ret = write(f, &data[j*ON_CHIP_MEMORY_SPAN], data_in_one_operation);
          if (ret < 0){
            perror("Failed to write in /dev/dma_pl330.");
            return errno;
          }
        }
        overflow = pmu_counter_read_ns(&pmu_counter_ns);
        if (overflow == 1){printf("Cycle counter overflow!! \n");return 1;}

        if (l>=2) update_cumulative(&total_dma_wr, &min_dma_wr, &max_dma_wr, 
          &variance_dma_wr, 0, pmu_counter_ns, clk_read_average);
        //(We erase two first measurements because they are different from the
          // others. Reason:Branch prediction misses when entering for loop)
      
        //check the content of the data just read
        // Compare results
        if(0  != memcmp(&(data[0]), on_chip_RAM_vaddr_void, 
          data_in_one_operation))
        {
          printf("DMA src and dst have different data on WR!!\n");return 1;
        }


        //--READ DATA FROM FPGA ON-CHIP RAM
        pmu_counter_reset();
        for (j=0; j<operation_loops; j++)
        {
          ret = read(f, &data[j*ON_CHIP_MEMORY_SPAN], data_in_one_operation);
          if (ret < 0){
            perror("Failed to read from /dev/dma_pl330.");
            return errno;
          }
        }
        overflow = pmu_counter_read_ns(&pmu_counter_ns);
        if (overflow == 1){printf("Cycle counter overflow!! \n");return 1;}

        if (l>=2) update_cumulative(&total_dma_rd, &min_dma_rd, &max_dma_rd,
          &variance_dma_rd, 0, pmu_counter_ns, clk_read_average);
        //(We erase two first measurements because they are different from the
          // others. Reason:Branch prediction misses when entering for loop)
      
        //check the content of the data just read
        // Compare results
        if(0  != memcmp(&(data[0]), on_chip_RAM_vaddr_void, 
          data_in_one_operation))
        {
          printf("DMA src and dst have different data on WR!!\n");return 1;
        }

        //free dynamic memory
        free(data);
      }
      close(f);
      printf("%d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n\r", 
        data_size[i], total_dma_wr/REP_TESTS, min_dma_wr, max_dma_wr, 
        variance(variance_dma_wr, total_dma_wr, REP_TESTS), 
        total_dma_rd/REP_TESTS, min_dma_rd, max_dma_rd,  
        variance(variance_dma_rd, total_dma_rd, REP_TESTS) );
    }
  }
  
  
	// --------------clean up our memory mapping and exit -----------------//
	if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) 
  {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	close( fd );

  printf("All tests were successful!! End...\n");

	return( 0 );
}

//---------------------FUNCTION TO PRINT BUFFERS---------------------------//
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

//--------EXTRA FUNCTIONS TO CALCULATE SOME STATISTICS IN EXPERIMENTS-----//
void reset_cumulative(unsigned long long int * total, 
  unsigned long long int* min, unsigned long long int * max, 
  unsigned long long int * variance)
{
  *total = 0;
  *min = ~0;
  *max = 0;
  *variance = 0;
}

void update_cumulative(unsigned long long int * total,  
  unsigned long long int* min, unsigned long long int * max, 
  unsigned long long int * variance, unsigned long long int ns_begin, 
  unsigned long long int ns_end, unsigned long long int clk_read_delay)
{
  unsigned long long int tmp = ( ns_end < ns_begin ) ? 
    1000000000 - (ns_begin - ns_end) - clk_read_delay :
    ns_end - ns_begin - clk_read_delay ;

  *total = *total + tmp;
  *variance = *variance + tmp*tmp;

  if (tmp < *min) *min = tmp;
  if (tmp > *max) *max = tmp;

//  printf("total %lld, begin %lld, end %lld\n", *total, ns_begin, ns_end);
}

unsigned long long variance (unsigned long long variance , 
  unsigned long long total, unsigned long long rep_tests)
{

  float media_cuadrados, quef1, quef2, cuadrado_media, vari;

  media_cuadrados = (variance/(float)(rep_tests-1));
  quef1 = (total/(float)rep_tests);
  quef2=(total/(float)(rep_tests-1));
  cuadrado_media = quef1 * quef2;
  vari = media_cuadrados - cuadrado_media;
/*
  printf("media_cuadrados %f,",media_cuadrados );
  printf("quef1 %f,",quef1 );
  printf("quef2 %f,",quef2 );
  printf("cuadrado_media %f,",cuadrado_media );
  printf("variance %f\n",vari );
*/
  return (unsigned long long) vari;

  //return ((variance/(rep_tests-1))-(total/rep_tests)*(total/(rep_tests-1)));
}