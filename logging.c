#include "imuread.h"
#include <stdio.h>
#include <unistd.h>

void logMessage(const char *message) 
{
    int fd = open("log.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);  // Open file in append mode

    if (fd < 0) {
        perror("Error opening log file");
        return;
    }

    write(fd, message, strlen(message));  // Write message
    write(fd, "\n", 1);  // Newline for readability

    close(fd);  // Close the file
}

void debugPrint(const char *name, const unsigned char *data, int lengthData, bool showHex)
{
	int i;
    int charWidth = showHex? 6:1; 
	int lengthName = strlen(name);
	
    int len = lengthName + 5 + (lengthData*charWidth);
    char message[len];
	snprintf(message, sizeof(message), "%s ", name);
    char* messagePtr = message + strlen(message);
    
    for (i = 0; i < lengthData; i++) 
    {
    	char thisChar[charWidth + 1];
    		snprintf(thisChar, sizeof(thisChar), 
    			showHex? "%02X[%c] ": "%c", 
    			data[i],(data[i] >= 32 && data[i] <= 126) ? data[i] : '.');	
		if (messagePtr - message + strlen(thisChar) < sizeof(message)) {
            strcpy(messagePtr, thisChar);
            messagePtr += strlen(thisChar);
        } else {
            break; // Avoid buffer overflow
        }
	}	
	logMessage(message);
}

void logTerminalSettings(struct termios termsettings)
{
    char message[60];
	snprintf(message, 60, "    c_iflag: '%lu' ", termsettings.c_iflag);
	logMessage(message);
	snprintf(message, 60, "    c_oflag: '%lu' ", termsettings.c_oflag);
	logMessage(message);	
	snprintf(message, 60, "    c_cflag: '%lu' ", termsettings.c_cflag);
	logMessage(message);		
	snprintf(message, 60, "    c_lflag: '%lu' ", termsettings.c_lflag);
	logMessage(message);
	snprintf(message, 60, "    c_cc: '%s' ", termsettings.c_cc);
	logMessage(message);
}

void print_data(const char *name, const unsigned char *data, int len)
{
	int i;
    char message[60];
    
	snprintf(message, 60, "log data : '%s', %d", name, len);
	logMessage(message);

	for (i=0; i < len; i++) {
		snprintf(message, 60, "    %02X [%c]", data[i], data[i]);
	    logMessage(message);
	}
	snprintf(message, 60, "done %d", len);
	logMessage(message);
}