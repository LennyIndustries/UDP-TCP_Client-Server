/*
 * Created by Leander on 30/04/2021.
 * Log function, use this to log output to a file.
 */

// Libraries
#include "lilog.h"

void liLog(char logLevel, char *file, unsigned int line, char append, const char *message, ...)
{
	// A big ball of wibbly wobbly, timey wimey stuff.
	char dateTime[22] = {'\0'};
	time_t myTime = time(NULL);
	// Log file.
	FILE *myLogFile = NULL;
	char *logLevelString = NULL;
	// Arguments
	va_list arg;

	switch (logLevel) // Sets the log level from given number
	{
		case 1:
			logLevelString = "INFO";
			break;
		case 2:
			logLevelString = "WARN";
			break;
		case 3:
			logLevelString = "CRIT";
			break;
		default:
			liLog(2, file, line, 1, "Could not resolve log level: %i.", logLevel);
			return;
	}

	myLogFile = fopen(LILOG_OUTPUTFILENAME, append ? "a" : "w"); // Opens the log file

	if (myLogFile == NULL) // Check if the file is opened
	{
		printf("-!!!- CRITICAL ERROR -!!!-\nCan not open log file!\nTerminating program.\n");
		exit(EXIT_FAILURE);
	}

	strftime(dateTime, 22, "%H:%M:%S - %d/%m/%Y", localtime(&myTime)); // Creates date and time string for log file

	fprintf(myLogFile, "%s :: %s :: File: %s (line: %u) :: ", logLevelString, dateTime, file, line); // Prints all needed info to the log file

	va_start(arg, message); // Prints the message to the log file
	vfprintf(myLogFile, message, arg);
	va_end(arg);
	fprintf(myLogFile, "\n"); // Prints a new line at the end

	fclose(myLogFile); // Closes the log file
}
