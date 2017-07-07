//file: PMU_User_Space_EN.c

/*PMU is a coprocessor located very close to the processor in ARM
Cortex-A9. It is in charge of gatheric statistics from the processor, 
i.e. the number of exceptions, divisions by 0, etc. Each processor
core has its own PMU. In case of Cyclone V the processor has two
cores and therefore two PMUs. PMU is not mapped in the address 
space of the processor. It is accessed through instructions like
the mathematical cores Neon.
The PMU has a CPU clock cycle counter that can be used to measure
time. To measure time with PMU is not a good practice because
PMU counts cycles from PMU clock, not time. Therefore if the clock
rate of the CPU changes (i.e. it is reduced to save energy) the
time measurement will be wrong. The advantage of using the PMU
is that it measures time very precisely, more than any OS clock. 
Therefore if clock rate is stable and we know its rate we can
measure time very precisely with PMU. A good practise is to use
an OS timer together with PMU to verify that the PMU measurement 
is correct. 
When using OS, PMU is not accessible from user space by default.
However setting the PMUSERENR.EN bit it can be accessed from
user space. This module accesses from kernel space to the PMU
and sets PMUSERENR.EN bit. After inserting this module therefore
the PMU will be able to be accessed from any application running
in user space.
*/
#include <linux/module.h>

static int mod_init(void)
{
		
	int var = 0;	

	pr_info("Module initialization\n");

	//Generic read and write demonstration using register r3
	//asm volatile ("mov    r3, %[value]"::[value]"r" (var)); //write (var int)
	//asm volatile ("mov    %[value], r3":[value]"+r" (var)); //read (var int)	

	//Enable PMU from user space setting PMUSERENR.EN bit
	asm volatile("mrc p15, 0, %[value], c9, c14, 0":[value]"+r" (var));//read PMUSERENR
	pr_info("PMU User Enable register=%d\n", var);//print PMUSERENR
	
	var = 1;
	pr_info("Enabling PMU\n");
	asm volatile("mcr p15, 0, %[value], c9, c14, 0"::[value]"r" (var));//Set PMUSERENR.EN 
	
	var = 2;
	asm volatile("mrc p15, 0, %[value], c9, c14, 0":[value]"+r" (var));//read PMUSERENR
	pr_info("PMU User Enable register=%d\n", var);//print PMUSERENR

	return 0;
}
static void mod_exit(void)
{
	pr_info("Module exit\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roberto Fernández <robertofem@gmail.com>");
MODULE_AUTHOR("Filipe Salgado <filipe.salgado@gmail.com>");
MODULE_DESCRIPTION("Make PMU coprocessor registers accesible from user space");
MODULE_VERSION("1.0");
