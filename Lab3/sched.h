#pragma once
#define STUDENT

typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned char uint8_t;
typedef char int8_t;

enum REGISTERS
{
	ZERO = 0,
	RA,
	SP,
	GP,
	TP,
	T0,
	T1,
	S0,
	S1,
	A0,
	A1,
	A2,
	A3,
	A4,
	A5,
	A6,
	A7,
	S2,
	S3,
	S4,
	S5,
	S6,
	S7,
	S8,
	S9,
	S10,
	S11,
	T3,
	T4,
	T5,
	T6,
};

enum PROCESS_STATE
{
	DEAD = 0,
	SLEEPING,
	RUNNING,
};

enum SCHEDULE_ALGORITHM
{
	SCHED_RR,
	SCHED_ML,
	SCHED_MLF,
};

struct PROCESS {
	/* DO NOT CHANGE BELOW THIS LINE */
	uint32_t	pid;
	int32_t		priority;
	void 		(*program)();
	uint32_t	scratch;
	uint32_t	regs[32]; // order specified by enum REGISTERS
	/* DO NOT CHANGE ABOVE THIS LINE */
	uint32_t	runtime;
	uint32_t	start_time;
	uint32_t	num_switches;
	uint32_t	sleep_time; //When state == *_SLEEP
	uint32_t	quantum_multiplier; // This gives a process a longer time-slice
	char 		name[32]; //Including NULL terminator
	PROCESS_STATE	state;
	//Private data, you can use this for any reason you want
	uint32_t	priv;
};

const uint32_t MAX_PROCESSES = 10;
const uint32_t TIMER_FREQ = 10000000;
const uint32_t SWITCHES_PER_SEC = 100;
const uint32_t CT_TIMER = TIMER_FREQ / SWITCHES_PER_SEC;
const uint32_t STACK_ALLOC = 16384;

void add_new_process(int total_processes_added);
void new_process(void (*func)(), const char *name, int32_t priority=0);
void del_process(PROCESS *p);
void sleep_process(PROCESS *p, uint32_t sleep_time);
PROCESS *schedule(PROCESS *current);

//These utility functions are written for you
void wait();
void write_string(const char *output);
void write_stringln(const char *output);
void to_string(char *dst, int value);
void strcpy(char *dst, const char *src);
void recover();
unsigned long get_timer_lo();
unsigned long get_timer_hi();
PROCESS *get_current();
