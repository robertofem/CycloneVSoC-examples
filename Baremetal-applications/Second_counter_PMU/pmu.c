/**
 * @file        pmu.c
 * @author      Filipe Salgado (filipe.salgado@gmail.com), Roberto Fernández (robertofem@gmail.com)
 * @version     2.0
 * @date        22 July, 2015
 * @brief       ARMv7-A Performance Monitor Unit v2 Driver
 *              Modification of the original ARMv7-A Performance Monitor Unit v1 Driver by Sandro Pinto (sandro.pinto@dei.uminho.pt)
 */

/* Includes ---------------------------------------------*/
#include "pmu.h"
#include <stdio.h>

/* Private types ----------------------------------------*/

/* Private constants ------------------------------------*/

/* Private macro ----------------------------------------*/

/* Private variables ------------------------------------*/
float res_ns = 0.0;

/* Private function prototypes --------------------------*/

/* Private functions ------------------------------------*/

/**
 * @brief    PMU Initialization (Global Counter Enable) when measuring in ticks
 * @param    none
 * @retval   none
 */
void inline pmu_init(void)
{
	__asm volatile("mrc    p15, 0, r0, c9, c12, 0");    /* Read PMCR                            */
	__asm volatile("orr    r0, r0, #0x01");             /* Set E bit                            */
	__asm volatile("mcr    p15, 0, r0, c9, c12, 0");    /* Write PMCR 					  	  */
}

/**
 * @brief    PMU Initialization (Global Counter Enable) when measuring in ns or cycles
 * @param    cpu_freq (cpu frequency in MHz)
 * 			 freq_div=1,64 (clock source for PMU cycle counter can be cpu_freq
 * 			 or cpu_freq/64)
 * @retval   none
 */
void inline pmu_init_ns(int cpu_freq, int freq_div)
{
	//int var = 0;

	//initialize pmu
	pmu_init();

	//Select the frequency divider in cycle counter using PMCR.D bit
	__asm volatile ("mrc    p15, 0, r0, c9, c12, 0");    /* Read PMCR                            */
	if (freq_div == 1) __asm volatile ("and    r0, r0, #0xFFFFFFF7");   /* Reset D bit	       */
	else if (freq_div == 64) __asm volatile ("orr    r0, r0, #0x08");   /* Set D bit       	   */
	__asm volatile ("mcr    p15, 0, r0, c9, c12, 0");    /* Write PMCR                           */

	//calc resolution
	res_ns = 1000.0*((float)freq_div) / (float)cpu_freq;
}


/**
 * @brief    Get PMU Cycle Counter resolution (for cpu_freq and freq_div passed in pmu_init_ns())
 * @param    none
 * @retval   resolution in ns
 */
float inline pmu_getres_ns(void){return res_ns;}


/**
 * @brief    Enable PMU Cycle Counter
 * @param    none
 * @retval   none
 */
void inline pmu_counter_enable(void)
{
	//int var=0;
	//__asm volatile("mrc p15, 0, %[value], c9, c12, 1":[value]"+r" (var));
	//printf("Var=%#010x \n", var);

	__asm volatile("mrc    p15, 0, r0, c9, c12, 1");    /* Read PMCNTENSET                      */
	__asm volatile("orr    r0, r0, #0x80000000");		  /* Set E bit                            */
	__asm volatile("mcr    p15, 0, r0, c9, c12, 1");    /* Write PMCNTENSET		              */

	//__asm volatile("mrc p15, 0, %[value], c9, c12, 1":[value]"+r" (var));
	//printf("Var=%#010x \n", var);
}


/**
 * @brief    Disable PMU Cycle Counter
 * @param    none
 * @retval   none
 */
void inline pmu_counter_disable(void)
{

	//-----THIS FUNCTION DOES NOT WORK!!!!!!!--------//
	//For some reason PMCNTENSET is not written when using "mcr    p15, 0, r0, c9, c12, 1"

	//int var=0;
	//__asm volatile("mrc p15, 0, %[value], c9, c12, 1":[value]"+r" (var));
	//printf("Var=%#010x \n", var);

	__asm volatile("mrc    p15, 0, r0, c9, c12, 1");    /* Read PMCNTENSET                      */
	__asm volatile("ldr 	 r1, =0x7FFFFFFF");		      /* Mask to reset E bit 				  */
	__asm volatile("and    r0, r1");		  			  /* Reset E bit                          */
	__asm volatile("mcr    p15, 0, r0, c9, c12, 1");    /* Write PMCNTENSET	ç	              */

	//__asm volatile("mrc p15, 0, %[value], c9, c12, 1":[value]"+r" (var));
	//printf("Var=%#010x \n", var);

	//__asm volatile("mrc p15, 0, r3, c9, c12, 1");
}


/**
 * @brief    Reset PMU Cycle Counter (all zeros)
 * @param    none
 * @retval   none
 */
void inline pmu_counter_reset(void)
{
	//reset counter to put it in a known state far from overflow
	__asm volatile ("mrc    p15, 0, r0, c9, c12, 0");    /* Read PMCR                            */
	__asm volatile ("orr    r0, r0, #0x04");             /* Set C bit (Event Counter Reset)      */
	__asm volatile ("mcr    p15, 0, r0, c9, c12, 0");    /* Write PMCR                           */

	//reset overflow bit
	__asm volatile("mrc    p15, 0, r0, c9, c12, 3");    /* Read PMCCNTR                      	   */
	__asm volatile("orr    r0, r0, #0x80000000");		  /* Set C bit (Overflow reset)			   */
	__asm volatile("mcr    p15, 0, r0, c9, c12, 3");    /* Write PMCCNTR		               	   */

	//reset counter again
	__asm volatile ("mrc    p15, 0, r0, c9, c12, 0");    /* Read PMCR                            */
	__asm volatile ("orr    r0, r0, #0x04");             /* Set C bit (Event Counter Reset)      */
	__asm volatile ("mcr    p15, 0, r0, c9, c12, 0");    /* Write PMCR                           */
}

/**
 * @brief    Read PMU Cycle Counter
 * @param    counter value (number of ticks from reset) (it is an output variable)
 * @retval   overflow state = 0 (no overflow occurred), 1 (error, overflow occurred)
 */
int inline pmu_counter_read(unsigned int *counter_value)
{
	int value=0, overflow=0;

	//read counter
	__asm volatile("mrc p15, 0, %[value], c9, c13, 0":[value]"+r" (value)); /*Read PMCCNTR.CCNT Register*/
	*counter_value = value;

	//check overflow
	__asm volatile ("mrc    p15, 0, r0, c9, c12, 3");    /* Read PMCCNTR						           */
	__asm volatile("and    r0, r0, #0x80000000");		  /* Reset all less C bit (PMCCNTR[31])            */
	__asm volatile ("mov    %[value], r0":[value]"+r" (overflow)); //read r0 and save it in overflow

	if (overflow == 0) return 0; // no overflow
	else return 1; //overflow
}

/**
 * @brief    Read PMU Cycle Counter Time
 * @param    counter measured time (for cpu_freq and freq_div set in pmu_init_ns())
 * @retval   overflow state = 0 (no overflow occurred), 1 (error, overflow occurred)
 */
int inline pmu_counter_read_ns(unsigned long long int *value_ns)
{
	unsigned int counter_value;
	int overflow;

	overflow = pmu_counter_read(&counter_value);
	*value_ns = (unsigned long long int) ((float)counter_value * res_ns);

	return overflow;
}
