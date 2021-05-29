/*
 * Created by Leander on 07/05/2021.
 * Lenny Industries Library Header
 */

#ifndef PROJECT_TIED_LILIB_H
#define PROJECT_TIED_LILIB_H

// Libraries
/*
 * This header contains the following headers:
 * - stdio.h
 * - stdlib.h
 * - time.h
 * - stdarg.h
 */
#include "lilog.h"
#include <string.h>
#include <ctype.h>

// Definitions
// Allows for binary notation: B(00101101)
#define B(x) S_to_binary_(#x) // https://stackoverflow.com/a/15114188/4620857
#define sizeOfArray(x) (sizeof(x) / sizeof((x)[0]))

// Functions
/*
 * Allows binary notation
 * https://stackoverflow.com/a/15114188/4620857
 */
unsigned long long S_to_binary_(const char *s);

/*
 * Converts a string to all lowercase, string is edited directly if you would like to keep a copy of the original string you need to make it yourself.
 * @param: char * (str) a pointer to the string
 * @return: void
 */
void strToLower(char *str);

/*
 * Gets the FIRST position of a char in a string.
 * @param: const char * (str) string to check, char (character) the character to look for
 * @return: int (index) the LAST position of the character in the string
 */
int indexOf(const char *str, char character);

/*
 * Gets the LAST position of a char in a string.
 * @param: const char * (str) string to check, char (character) the character to look for
 * @return: int (index) the LAST position of the character in the string
 */
int lastIndexOf(const char *str, char character);

/*
 * Creates a substring from a string.
 * @param: char * (str_source) the source string, char * (str_destination) the destination string, int (startPos) the starting position, int (length) how long to go from start
 * @return: void
 */
void subString(char *str_source, char *str_destination, int startPos, int length);

#endif // PROJECT_TIED_LILIB_H
