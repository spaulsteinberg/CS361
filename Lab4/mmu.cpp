//mmu.cpp
//MMU Lab Template by Stephen Marz
/* 
   Author: Samuel Steinberg
   Date: March 7th, 2019 
   This program will be able to enable/disable the mmu, and map page tables for programs. 
   Collaborated with David Clevenger, Tanner Fry, Samuel Jones 
*/
#include <mmu.h>

extern uint32_t MMU_TABLE[MMU_TABLE_SIZE];
/* This is a test function to make checking addresses easier */
static void print_address(char *var, uint32_t num)
{
	char temp[30];
	hex_to_string(temp, num, true);
	write_string(var);
	write_stringln(temp);
}
/*This function will built the pages for the stack and program */
static void build_entries(uint32_t addr_start, uint32_t addr_end, PROCESS *p, uint32_t *root, uint32_t flag)
{
	uint32_t *level1, *level0;
	uint32_t vpn1, vpn0;
	/*See if either stack or program is being mapped */
	if (flag == 1) flag = 0x6;
	else flag = 0xA;

	/*extract VPN's */
	vpn0 = (addr_start >> 12) & 0x3ff;
	vpn1 = (addr_start >> 22) & 0x3ff;
	
	/*Level 1 and 0 page entries */
	level1 = root;
	level0 = level1 + 1024;

	/*Set the level 1 entry as the level 0 table and set the V bit */
	*(level1+vpn1) = ((uint32_t)level0 >> 2);
	*(level1+vpn1) |= 0x1;
	uint32_t i = 0;

	/*Same loop as above */
	for ( ; addr_start < addr_end; addr_start+=4096)
	{
		/*Set XR and V for program, RW and V for stack, then do ppn alignment and U-bit mode*/
		*(level0+vpn0) = 0x1 | flag;
		*(level0+vpn0) |= ((vpn1 << 20) | (vpn0 << 10));
		*(level0+vpn0) += (4096*i);
		if (p->mode == 0)
		{
			*(level0+vpn0) |= (1 << 4);
		}
		++vpn0;
		i++;
	}
}
/*This function diables the MMU by turning off the satp registers mode bit. */
void mmu_disable()
{
	set_satp( get_satp() & ~(1U << 31));
}
/*This function enables the MMU by getting the ppn (20 MSB) and setting the mode bit. */
void mmu_enable(PROCESS *p)
{
	uint32_t ppn;
	if (p->mmu_table != 0)
	{
		ppn = (p->mmu_table >> 12);
		set_satp(ppn | (1U << 31));
	}
}
/* This function maps the MMU. */
void mmu_map(PROCESS *p)
{
	uint32_t func_addr, stack_addr, temp, prog_end, stack_end, s_flag;
	uint32_t *root;

	/* Root and alignment by pid. Align by 4096 bytes and set the mmu table for each process to the root */
	root = MMU_TABLE + ((p->pid - 1) * 1024 * 10);
	temp = (uint32_t)(root);
	temp = (temp + (4096 - 1)) & -4096;
	root = (uint32_t*)temp;
	p->mmu_table = (uint32_t)root;
	/*Beginning of stack and program */
	stack_addr = p->regs[2];
	func_addr = (uint32_t)p->program;

	/*End --> program can have two pages and stack can have 3 */
	prog_end = ((uint32_t)p->program + 4095 + 4096) & ~(0xfff);
	stack_end = (p->regs[2] + 4095 + 8192) & ~(0xfff);
	/**************Stack portion*******************/

	s_flag = 1;
	build_entries(stack_addr, stack_end, p, root, s_flag);
	/***************Program counter part******************/
	s_flag = 0;
	build_entries(func_addr, prog_end, p, root, s_flag);
}
/*This function will zero out all the bits in the page entries and ensure unmapping */
static void unmap_entries(uint32_t start, uint32_t end, uint32_t *root)
{
	uint32_t *level1, *level0;
	uint32_t vpn0 = (start >> 12) & 0x3ff;
	uint32_t vpn1 = (start >> 22) & 0x3ff;
	level1 = root;
	level0 = level1 + 1024;
	//Run through each level and zero it -->sets V bit to 0 to un-map
	while (start < end)
	{
		*(level1 + vpn1) = 0;
		vpn1++;
		*(level0 + vpn0) = 0;
		vpn0++;
		start += 1;
	}
}
/*This function performs un-mapping of processes. */
void mmu_unmap(PROCESS *p)
{
	/*************Same access as map ****************/
	uint32_t *root;
	uint32_t temp, stack_addr, func_addr, prog_end, stack_end;
	root = MMU_TABLE + ((p->pid - 1) * 1024 * 10);
	temp = (uint32_t)(root);
	temp = (temp + (4096 - 1)) & -4096;
	root = (uint32_t*)temp;
	p->mmu_table = (uint32_t)root;
	stack_addr = p->regs[2];
	func_addr = (uint32_t)p->program;
	prog_end = ((uint32_t)p->program + 4095 + 4096) & ~(0xfff);	
	stack_end = (p->regs[2] + 4095 + 8192) & ~(0xfff);
	
	/*Call un map function to perform heavy lifting*/
	unmap_entries(stack_addr, stack_end, root);
	unmap_entries(func_addr, prog_end, root);
}

void hello()
{
	//This is sample code. This will run the process for 10,000,000 iterations
	//and then sleep for 5 seconds over and over again.
        ecall(SYS_SET_QUANTUM, 10);
        do {
                for (volatile int i = 0;i < 10000000;i++);
                ecall(SYS_SLEEP, 5);
        } while(1);
}

/*This function will test the un-map*/
static void sleep_for_10_seconds()
{
	ecall(SYS_SLEEP, 10);
	ecall(SYS_EXIT, 0);
}

/*Function to test disable, enable, map, un-map*/
void test()
{
	//Put whatever you want to test here.
      new_process(hello, 0, 0, MACHINE);
	  new_process(hello, 0, 0, SUPERVISOR);
      new_process(hello, 0, 0, USER);
	  new_process(sleep_for_10_seconds,0, 0, USER); // used for testing un-map
}
