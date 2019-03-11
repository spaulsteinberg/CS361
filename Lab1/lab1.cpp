/*
	Author: Samuel Steinberg
	Contributors: Samuel Jones
	Date: January 14th, 2019
	This program will have a Test, Set, Clear, and Mask function. This is accomplished using bit arithmetic.
*/
/* These were used for testing 
#include <stdint.h>
#include <iostream>
using namespace std; 
*/
/* Test function will return true when the given bit is 1, and false when the given bit is 0 */
bool Test(const volatile uint32_t *bitset, int bit_index)
{
	if (bit_index >= -32 && bit_index <= 31)
	{
		/*Wrapper for negative indexes */
		if (bit_index < 0) { bit_index = 32 + bit_index; }
		if( ((*bitset) & (1 << bit_index)) )
		{
			return true;
		}
		else
			return false;
	}
	else
		return false;
}
/*Set function sets the given bit index to 1 -- This is after a bounds check.*/
void Set(volatile uint32_t *bitset, int bit_index)
{
	if (bit_index >= -32 && bit_index <= 31) 
	{ 
		/*Wrapper for indexes, below the bit is "turned-on". */
		if (bit_index < 0) { bit_index = 32 + bit_index; }
		*bitset |= (1 << bit_index); 
	}
}
/* Clear function sets the given bit to 0 -- This is after a bounds check.*/
void Clear(volatile uint32_t *bitset, int bit_index)
{
	if (bit_index >= -32 && bit_index <= 31) 
	{ 
		/* Wrapper for indexes, below bit is "turned-off" */
		if (bit_index < 0) { bit_index = 32 + bit_index; }
		*bitset &= ~(1 << bit_index); 
	}
}
/* Mask function gets just the bits specified in the range and sets them to 1's (between inclusive start/end) */
constexpr uint32_t Mask(int bit_start, int bit_end)
{
	/* If the index's are BOTH between -31 and 31 inclusively, enter statement */
	if ( (bit_start <= 31 && bit_end <= 31) && (bit_start >= -32 && bit_end >= -32) )
	{
		/*These two statements wrap the negative values around. So, for example: -1 would go to 31. */
		if (bit_start < 0) bit_start = 32 + bit_start;
		if (bit_end < 0) bit_end = 32 + bit_end;

		/* Set flag to see whether the end index is larger than the start index */
		bool start = false;
		if (bit_start < bit_end) { start = true; }
		/* Get number of bits in range given */
		int num_bits = (bit_end - bit_start);
		/*If neg number of bits, set to positive number */
		if (num_bits < 0) num_bits = num_bits *-1;
	
		/*Number of bit will be off by once because they are inclusive so add 1 here */
		int setter = (1 << (num_bits + 1) ) - 1;
		

		/*Above statement zero's a difference of 32 out. AKA is num_bits is 32 setter gets set to 0 above when should be FFFFFFFF */
		if (num_bits == 31) { setter = 0xFFFFFFFF; } 
		/* If the start/end is EQUAL */
		if (num_bits == 1) { setter = 1; }
		/* If flag true, left shift by the start index */
		if (start) { return setter << bit_start; }
		/*else -- by end index */
		else { return setter << bit_end; }
	} 
	else
		return 0;
}
/*
int main()
{
	int index;
	int bit_start, bit_end, mask_test;
	uint32_t bitset;
// ----------------- Test() Section -----------------
	cout << "Give me two: " << endl;
	cin >> bitset >> index;
	bool result = Test(&bitset,index);
	cout << "Result of test is: " << result << endl;
// ---------------- Set() Section -------------------
	cout << "Give a bitset and an index" << endl;
	cin >> bitset >> index;
	Set(&bitset, index);
	cout << "Result of set is: " << bitset << endl;
// --------------- Clear() Section -----------------
	cout << "Give me a bitset and an index" << endl;
	cin >> bitset >> index;
	Clear(&bitset, index);
	cout << "Result of clear is: " << bitset << endl;
// -------------- Mask() Section -------------------
	cout << "Give a start and end index" << endl;
	cin >> bit_start >> bit_end;
	mask_test = Mask(bit_start, bit_end);
	cout << "Mask result is: " << hex << mask_test << endl;
	return 0;
}*/
