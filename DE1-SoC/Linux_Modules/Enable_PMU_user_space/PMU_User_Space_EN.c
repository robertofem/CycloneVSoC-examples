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
