/*
 * Created by Leander on 07/05/2021.
 * Lenny Industries Library
 */

#include "lilib.h"

unsigned long long S_to_binary_(const char *s)
{
	unsigned long long i = 0;
	while (*s)
	{
		i <<= 1;
		i += *s++ - '0';
	}
	return i;
}

void strToLower(char *str)
{
	for (size_t i = 0; i < strlen(str); i++)
	{
		str[i] = (char) tolower(str[i]);
	}
}

int indexOf(const char *str, char character)
{
	int index = -1;
	char tmpCharacterStorage = '\0';
	for (int i = 0; i < (int) strlen(str); i++)
	{
		tmpCharacterStorage = str[i];
		if (tmpCharacterStorage == character) // !strcmp(&tmpCharacterStorage, &character) <== this does not work, use == instead.
		{
			index = i;
			break;
		}
	}
	liLog(1, __FILE__, __LINE__, 1, "Index of: \"%c\" in: %s: %i.", character, str, index);
	return index;
}

int lastIndexOf(const char *str, char character)
{
	int index = -1;
	char tmpCharacterStorage = '\0';
	for (int i = 0; i < (int) strlen(str); i++)
	{
		tmpCharacterStorage = str[i];
		if (tmpCharacterStorage == character) // !strcmp(&tmpCharacterStorage, &character) <== this does not work, use == instead.
		{
			index = i;
		}
	}
	liLog(1, __FILE__, __LINE__, 1, "Index of: \"%c\" in: %s: %i.", character, str, index);
	return index;
}

void subString(char *str_source, char *str_destination, int startPos, int length)
{
	memcpy(str_destination, &str_source[startPos], length + 1);
	liLog(1, __FILE__, __LINE__, 1, "Substring from: %s: %s.", str_source, str_destination);
}
