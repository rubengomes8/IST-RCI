#include "utils.h"

int count_specific_char(char *string, char ch){
		int count = 0;
		int i;

		int length = strlen(string);

		for(i = 0; i < length; i++)
		{
				if(string[i] == ch)
				{
					count++;
				}
		}
		return count;
}
