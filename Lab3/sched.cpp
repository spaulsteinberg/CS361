/*
	Author: Samuel Steinberg
	Date: February 8th, 2019
	Contributers: Tanner Fry 
	This program implements a scheduler. The algorithms round-robin (RR), multi-level (ML) and multi-level feedback (MLF) are implemented.
*/
//////////////////////////////////
//
// Do not modify anything between
// this box and the one below
//
/////////////////////////////////
#include <sched.h>


/////////////////////////////////
//
// Implement the following below
//
////////////////////////////////

extern PROCESS process_list[MAX_PROCESSES];
extern SCHEDULE_ALGORITHM scheduling_algorithm;
extern char stack_space[STACK_ALLOC * MAX_PROCESSES];

#if defined(STUDENT)
static void proc_10_secs_run()
{
	uint32_t END_AFTER = get_timer_lo() / TIMER_FREQ + 10;
	write_stringln("\r\nI do nothing but quit after 10 seconds.\r\n");
	while (get_timer_lo() / TIMER_FREQ < END_AFTER);
}

static void proc_10_secs_sleep()
{
	write_stringln("\r\nI'm going to sleep for 10 seconds, then quitting.\r\n");
	sleep_process(get_current(), 10);
	wait();
}
/*This function adds the process to the schedule. Passes the function pointer, name of the process, and priority*/
void add_new_process(int padd)
{
	switch (padd) {
		case 1:
			new_process(proc_10_secs_run, "10 second run process", 10);
		break;
		default:
			new_process(proc_10_secs_sleep, "10 second sleep process", -5);
		break;
	}
}
#endif
/*This function gets the current time. */
static uint32_t get_time()
{
		uint32_t timer = get_timer_lo()/ TIMER_FREQ;
		return timer;
}
/*This function creates a new process. If a spot is found in the list, break and initialize the process. */
void new_process(void (*func)(), const char *name, int32_t priority)
{
	uint32_t pid_index;
	bool sleep_run = false;
	PROCESS *p;
	/*Traverse array to find a state that is dead */
	for (pid_index = 0; pid_index < MAX_PROCESSES; pid_index++)
	{
		p = &process_list[pid_index];
		if ( p->state == DEAD)
		{
			sleep_run = true;
			break;
		}
	}
	/*If outside of priority range clamp */
	if (priority > 10) priority = 10;
	if (priority < -10) priority = -10;
/* Initialization of process. A major part of the architecture of the rest of the code is setting the pid to the process_list index 
   plus one. Allows for easy offsetting. Priority set to -10 for each new process in MLF*/
	if(sleep_run)
	{
		p = &process_list[pid_index];
		p->pid = pid_index + 1;
		p->priority = priority;
		if (scheduling_algorithm == SCHED_MLF)
		{
			p->priority = -10;
			p->quantum_multiplier = 0;
		}
		p->program = func;
		p->scratch = 0;
		p->regs[SP] = (uint32_t)(stack_space + (STACK_ALLOC * p->pid) - 1); 
		p->regs[RA] = (uint32_t)recover;
		strcpy(p->name, name);
		p->state = RUNNING;
		p->priv = 0;
	}
}
/*This function deletes a process merely by setting its state to DEAD. When a new process is looking for a spot in new_process,
  this process's info will be overwritten. NOTE***: Since the process is dead and will be overwritten in the new_process
  at its spot, i reset the priority and QM there. */
void del_process(PROCESS *p)
{
	p->state = DEAD;
}
/* This function sets the state to SLEEPING and updates its sleep time with the time its sleeping plus the current time.
   For ML and MLF the processes get a priority of -10 and quantum multiplier of 0.*/
void sleep_process(PROCESS *p, uint32_t sleep_time)
{
	p->state = SLEEPING;
	p->sleep_time = sleep_time + get_time();
	if (scheduling_algorithm == SCHED_MLF)
	{
		p->priority = -10;
		p->quantum_multiplier = 0;
	}
}
/*Goes in order of PID, if the process is running put it in the scheduler. Processes scheduled in ascending order from PID. */
static PROCESS *sched_round_robin(PROCESS *p)
{
	uint32_t i;
	for (i = p->pid; i < MAX_PROCESSES; i++)
	{
		if ( (process_list + i)->state == RUNNING)
		{
			return process_list + i;
		}
	}
	return process_list;

}
/* This function executes the ML algorithm. Starting at the lowest priority it loops through each level and executes the process.
   This causes starvation as processes might never get enough resources to finish. If the priority of a process is equal to the process
   level and the state is running, return the process. Default returns the process_list at 0, a.k. init()*/
static PROCESS *sched_ML(PROCESS *current)
{
	int i = -10;
	while(i <= 10)
	{
		for (uint32_t j = current->pid; j < MAX_PROCESSES; j++)
		{
			if ( (process_list + j)->priority == i  && (process_list + j)->state == RUNNING)
			{
				return (process_list + j);
			}
		}
		for (uint32_t j = 0; j < current->pid; j++)
		{
			if ( (process_list + j)->priority == i  && (process_list + j)->state == RUNNING)
			{
				return (process_list + j);
			}
		}
		i++;
	}
	return process_list;
}
/* This scheduler function schedules a process in either a round-robin, multi-level, or multi-level feedback way. */
PROCESS *schedule(PROCESS *current)
{
	/* Run through each process and wake up any sleeping processes */
	for (uint32_t i = 0; i < MAX_PROCESSES; i++)
	{
		if ( (process_list + i)->state == SLEEPING && get_time() >= (process_list + i)->sleep_time)
		{
			(process_list + i)->state = RUNNING; 
		}
	}
	/* RR is default, and can also be called explicitly. ML explained above as well, MLF gets its process from the 
	   ML algorithm. Then, it lowers its priority and updates its quantum multiplier and returns the process. The
	   quantum multiplier ensures that each process will be assigned resources and keeps off starvation. Process time
	   and priority are inversely related.*/
	switch (scheduling_algorithm) 
	{
		case SCHED_RR:
			return sched_round_robin(current);
		case SCHED_ML:
			return sched_ML(current);
		case SCHED_MLF:
			{
				PROCESS *ml = sched_ML(current);
				ml->priority++;
				if ( (ml->priority) > 10) ml->priority = 10;
				ml->quantum_multiplier = ml->priority + 10;
				return ml;
			}
		default:
			sched_round_robin(current);
	}
}
