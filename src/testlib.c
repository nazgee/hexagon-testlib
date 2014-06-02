/*******************************************************************
 *
 * Name        	: testlib.cpp
 * Author      	: stawjmic
 * Version     	:
 * Description 	:
 *
 ********************************************************************/

#include <stdio.h>
#include <HAP_farf.h>
#include <testlib.h>

int testlib_function(const int* array, int arrayLength, int64* result) {

	int i, j;
	FARF(ALWAYS, "===============     DSP: Entering function testlib_function ===============");

	*result = 0;

	for (i = 0; i < 100; ++i) {
		for (j = 0; j < arrayLength; ++j) {
			*result += array[i];
		}
	}

	return 0;
}
