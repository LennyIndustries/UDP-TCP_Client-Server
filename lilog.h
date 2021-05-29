/*
 * Created by Leander on 30/04/2021.
 * Log function, use this to log output to a file.
 */

#ifndef PROJECT_TIED_MYLOG_H
#define PROJECT_TIED_MYLOG_H

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

// Definitions
#define LILOG_OUTPUTFILENAME "Output.log"

// Functions
/*
 * Use this to log lines to the output log.
 * void liLog(char logLevel, char * file, int line, char append, const char * message, ...)
 * @param:
 * logLevel:
 * - 1 : INFO : Just normal log info.
 * - 2 : WARN : Warning that did not terminate the program but should not happen.
 * - 3 : CRIT : Critical warning that terminates the program.
 * file: from what file the message is. You can use __FILE__
 * line: from what line the message is. You can use __LINE__
 * append: Whether to append (1) or not (0). If you do not append it will clear to file.
 * message: A log message you would like to put in the log. Do not end with \n, this is added automatically.
 * @return:VOID
 * Output.log example line
 * logLevel :: hh:mm:ss - dd/mm/yyyy :: File: FILE_LOCATION (line: LINE IN CODE) :: message
 * message can be used like printf(), ... "This is a number: %i", 1); will put "This is a number: 1" as message in the log file.
 */
void liLog(char logLevel, char *file, unsigned int line, char append, const char *message, ...);

#endif // PROJECT_TIED_MYLOG_H
