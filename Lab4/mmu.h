#pragma once
#ifndef _HAVE_MMU_H
#define _HAVE_MMU_H

typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned char uint8_t;
typedef char int8_t;

struct PROCESS {
        uint32_t pid;
        int32_t  priority;
        void     (*program)();
        uint32_t scratch;
        uint32_t regs[32];
        uint32_t runtime;
        uint32_t start_time;
        uint32_t num_switches;
        uint32_t sleep_time;
        uint32_t quantum_multiplier;
        uint8_t  name[32];
        uint32_t state;
        uint8_t  mode;
        uint32_t priv;
        uint32_t mmu_table;
};

enum ProcessMode
{
        USER = 0,
        SUPERVISOR = 1,
        MACHINE = 3
};

enum Syscallno
{
        SYS_EXIT = 0,
        SYS_SET_QUANTUM = 1,
        SYS_DEBUG = 2,
        SYS_SLEEP = 10,
        SYS_NOTHING = 100
};

const uint32_t MAX_PROCESSES = 15;
const uint32_t MMU_TABLE_SIZE = 1024 * MAX_PROCESSES * 10;

#define ecall(x, y) \
	asm volatile("mv s11, %0\nmv s10, %1\necall\n" ::"r"(x), "r"(y) :"s11","s10")

extern "C" {
        //Write the following functions in your .cpp file
        void test();
        void mmu_disable();
        void mmu_enable(PROCESS *p);
        void mmu_map(PROCESS *p);
        void mmu_unmap(PROCESS *p);


        //The following functions have been written for you.
        uint32_t get_satp();
        void set_satp(uint32_t value);
	// Debugging functions
	void write_string(const char *str);
	void write_stringln(const char *str);
	void hex_to_string(char *dest, int src, bool prepend_0x=true);
	void strcpy(char *dest, const char *src);

        //mode = 0 (USER) or 1 (SUPERVISOR) or 3 (MACHINE)
        void new_process(void (*program)(), uint32_t priority, uint32_t quantum_multiplier, ProcessMode mode);
}



#endif
