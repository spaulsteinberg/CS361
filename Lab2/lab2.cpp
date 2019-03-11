/*
	Author: Samuel Steinberg
	Date: January 21st, 2019
	Collaborators: Tanner Fry, Samuel Jones	
	This program uses pointer arithmetic to control hardware from the OS. 
*/



char *const UART_BASE = (char*)0x10000000;
const char *const UART_RBR = UART_BASE + 0;
char *const UART_THR = UART_BASE + 0;
char *const UART_DLL = UART_BASE + 0;
char *const UART_DLM = UART_BASE + 1;
char *const UART_LCR = UART_BASE + 3;
const char *const UART_LSR = UART_BASE + 5;
const int OSC = 18000000;
const int BAUD = 115200;

/* The init function first calculates the divisor and stores it in a short (because it is 16 bits). I then set up the
  8BIT, DLAB, and PODD to handle 8 bits, the Divisor Latch Access Bit, and odd parity respectively. To allow 
  myself to access the DLL/DLM, I turn on the DLAB by setting the UART_LCR bit index 7 to 1. I then use the divisor
  to set the DLL for the 8 LSB's, then access the 8 MSB's to set the DLM. I then turn off the DLAB, so I then have access to the 
  RBR/THR from the LCR. */
void init()
{
	short divisor = (OSC / BAUD) - 1;
	*UART_LCR = 0x03 | 0x80 | 0x08; 
	*UART_LCR = *UART_LCR | (1 << 7);
	*UART_DLL = divisor;
	*UART_DLM = (divisor >> 8);
	*UART_LCR = *UART_LCR & ~(1 << 7);

}
/* The read_char function will keep trying to read data until there is data to be read (read into RBR). If the UART RX FIFO is empty
  I return 0. */
char read_char()
{
	if( ((*UART_LSR) & 1))
		return *UART_RBR;
	else return 0;
}
/* The write_char function takes a character argument and checks TEMT to see if TX FIFO is empty. So long as it is NOT empty,
   do nothing. Once it is empty the character can be passed to the THR register */
void write_char(char c)
{
	while ( !((*UART_LSR) & 0x40) )
	{
		;
	}
	*UART_THR = c ;

}
/* The write_string function will take a c-style string and while it is not the end of the string will call
   write_char to write the dereferenced pointer. I then increment the pointer position and write until the
   NULL terminator. */
void write_string(const char *out)
{
	while ( *out != '\0' )
	{
		write_char(*out);
		out++;
	}
}
/* The write_stringln function will take in a c-style string, and write it out using the write_string function (which calls
   the write_char function). Then, I write out the literal '\r' and '\0' chars with write_char. */
void write_stringln(const char *out)
{
	write_string(out);
	write_char('\r');
	write_char('\n');
}
/*This function will extract a single number at a time*/
static int extract_num(int num, bool echo, int last_calc)
{
	char temp;
	char line[30];
	bool sign = false;
	/* Read first char */
	temp = read_char();
	/*If this loop executes; it is because a non-numerical char was entered. If that char was a space,
	  I wrote it. If it was a negative symbol, I wrote the char and flagged the number as a negative. */
	while (temp < '0' || temp > '9')
	{
		if ( (echo == true) && temp == ' ') write_char(temp);
		if ( (echo == true) && temp == '-') 
		{
			write_char(temp);
			sign = true;
		}
		temp = read_char();
	}

	/*write first numerical value */
	line[0] = temp;
	if (echo) write_char(temp);
	int i;
	/*Loop through to get all digits of the number the user inputs. Since read_char frequently returns
	 0's, I skip them.*/
	for(i=1; ;i++)
	{
		while ((temp = read_char()) == 0)
		{
			
		}
		/* If a digit is entered write the character and append to the string (which is the combined digit line) */
		if ( (temp >= '0' && temp <= '9') )
		{
			if (echo) write_char(temp);
			line[i] = temp;
		}
		else
			break;
	}

	line[i] = '\0';
	/*Conversion from a char array to an integer*/
	num = 0;
	for (int j = 0; j < i; j++)
	{
		num = num * 10 + (line[j] - '0');
	}
	
	/*Check for the sign flag, if true then the number is a negative -- so negate the number. Or else do nothing. */
	if (sign) num = num*-1;
	else { }
	/*If this is the right number being extracted, while the user does not hit enter check for spaces. */
	if (last_calc)
	{
		while (temp != 0x0D)
		{
			/*If a space is hit, write it. Then loop through the 0's and break when no spaces. */
			if (temp == ' ') 
			{
				if (echo) write_char(temp);	
			}
			while ((temp = read_char()) == 0)
			{
			
			}
			if(temp == ' ')
			{

			}
			else
				break;
		}
	}
	return num;
}
/* The extract_two_numbers function calls my custom function: I build left first and then right...I also pass echo 
  and a 'last_calc' parameter. This flag is to allow for spaces after right is extracted.*/
void extract_two_numbers(int &left, int &right, bool echo)
{
	int last_calc = 0;
	left = extract_num(left, echo, last_calc);
	if (echo) write_char(' ');
	last_calc = 1;
	right = extract_num(right, echo, last_calc);
}
/*The to_string function will convert the integer back to the screen to be displayed. */
void to_string(char *dest, int value)
{
	int temp = 0, count = 0;
	char t;
	/* If the value is 0 just write it. */
	if (value == 0) write_char('0');
	/* If the value is negative, I convert it to positive then convert it to a string. After the numerical
	   value is converted I add a negative symbol to the end. NOTE: The value is generated backwards. */
	if (value < 0)
	{
		value = value*-1;
		while (value > 0)
		{
			temp = value % 10;
			value = (value / 10);
			dest[count] = (char)('0' + temp);
			count++;
		}
		dest[count] = '-';
		count++;
	}
	/* Same as above but for positive values, without the added sign. */
	else 
	{
		while (value > 0)
		{
			temp = value % 10;
			value = (value / 10);
			dest[count] = (char)('0' + temp);
			count++;
		}
	}
	/* Simple swap function. This is necessary because the string is built backwards above. */
	for (int i = 0; i != count/2; ++i)
	{
		t = dest[i];
		dest[i] = dest[count-i-1];
		dest[count-i-1] = t;
	}
	dest[count] = 0;
	
}

