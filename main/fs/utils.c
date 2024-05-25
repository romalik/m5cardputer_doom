/*
 * utils.c
 *
 * Created: 2023-07-08 21:35:23
 *  Author: liviu
 */ 

#include "utils.h"

/*************************************************************
	FUNCTIONS
**************************************************************/
/*______________________________________________________________________________________________
	Integer to string array conversion.
	String arrays must be defined as:
		char string_integer[MAX_NR_OF_DIGITS + 1];
	This allows itoa() to check if the number of digits fits the array.
	
	s[]				array pointer for the returned string
	
	nrOfDigits		Total number of digits desired. If the number to be displayed
					has less digits than "nrOfDigits" then it will be padded with zeros.
					Useful for maintaining the GUI layout and for displaying floats.
_______________________________________________________________________________________________*/
void STRING_itoa(INT_SIZE n, char s[], int8_t nrOfDigits){
	uint8_t str_len = 0;
	uint8_t idx = 0;
	INT_SIZE num_buf = n;
	bool is_negative = false;
	
	// Find number of digits
	while(num_buf != 0){
		str_len++;
		num_buf /= 10;
	}
	
	if(n == 0) str_len = 1;
	
	// Set padding with 0
	nrOfDigits -= str_len;
	if(nrOfDigits < 0) nrOfDigits = 0;
	
	// Check if number is negative and convert it to a positive
	if(n < 0){
		n = -n;
		str_len++; // placeholder for the minus sign
		is_negative = true;
	}

	idx = str_len + nrOfDigits;
	if(idx < MAX_NR_OF_DIGITS) s[idx] = 0; // add null terminator
	
	// Convert int to string array
	while(idx){
		idx--;
		
		// Trim the length of the number if it's bigger than the limit MAX_NR_OF_DIGITS
		if(idx < MAX_NR_OF_DIGITS){
			s[idx] = (n % 10) + '0';
		}

		n /= 10;
	}
	
	if(is_negative)	s[0] = '-';
}



/*______________________________________________________________________________________________
	Float number to string array conversion. The dot and negative sign are not added.
	String arrays must be defined as:
		char string_integer[MAX_NR_OF_DIGITS + 1];
		char string_float[MAX_NR_OF_DIGITS + 1];
	This allows itoa() to check if the number of digits fits the array.
	
	s_int[]			array pointer for the integer part
	s_float[]		array pointer for the decimal part
	nrOfDigits		see STRING_itoa()
	decimals		number of digits after the dot
_______________________________________________________________________________________________*/
void STRING_ftoa(float float_nr, char s_int[], char s_float[], uint8_t nrOfDigits, uint8_t decimals){
	uint8_t float_length = decimals;
	int32_t integer_part = 0;
	float rounding = 0.5;
	
	// Adjust the rounding value
	while(float_length){
		rounding /= 10.0;
		float_length--;
	}
	
	// Restore float length
	float_length = decimals;
	
	// Convert float part to positive
	if(float_nr < 0) float_nr = -float_nr;
	
	// Round up
	float_nr += rounding;
	
	// Get integer part
	integer_part = float_nr;
	
	// Get the decimal portion
	float_nr -= integer_part;
	
	// Convert fractional part to integer
	while(float_length){
		float_nr *= 10;
		float_length--;
	}
	
	STRING_itoa(integer_part, s_int, nrOfDigits);
	STRING_itoa(float_nr, s_float, decimals);
}



/*______________________________________________________________________________________________
	Return the absolute value of i (if the number is negative it will be converted to positive)
_______________________________________________________________________________________________*/
int MATH_abs(int i){
	return i < 0 ? -i : i;
}



/*______________________________________________________________________________________________
	Reverse the order of an array
	
	lenght		length of the array
_______________________________________________________________________________________________*/
void arrayReverse(uint8_t s[], uint8_t lenght){
	uint16_t i, j = lenght - 1;
	uint8_t c;
	
	// Reverse array
	for(i = 0; i < j; i++, j--){
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}