/******************************************************************************
*
* Author: Roberto Fernandez Molanes (robertofem@gmail.com)
* University of Vigo
*
* 2 June 2017
*
* Second Counter implemented with Performance Monitoring Unit (PMU)
******************************************************************************/

#include <stdio.h>
#include "pmu.h"

int __auto_semihosting;

int main(void)
{
    printf("Hello World!!\r\n");
	
	pmu_init_ns(800, 1); //Initialize PMU cycle counter, 800MHz source, frequency divider 1
	pmu_counter_enable();//Enable cycle counter inside PMU (it starts counting)
	float pmu_res = pmu_getres_ns();
	printf("PMU is used like timer with the following characteristics\n\r");
	printf("PMU cycle counter resolution is %f ns\n\r", pmu_res );	
	
	//Timing 60 seconds with PMU
	int seconds=0;
	unsigned long long pmu_counter_ns;
	
	pmu_counter_reset();
	printf("%d\n\r",seconds);
	
	while (seconds<61)
	{
		pmu_counter_read_ns(&pmu_counter_ns);
		if(pmu_counter_ns>1000000000)
		{
			pmu_counter_reset();
			seconds ++;
			printf("%d\n\r",seconds);
		}
	}

	printf("The end...\n\r");
    return 0;
}
