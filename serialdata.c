#include "imuread.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

LineEnding _lineEndingMode = LINE_ENDING_NOTSET;

int _bufferOffset = 0;

unsigned char* buildBuffer(const unsigned char *data, int len)
{
    int firstLine = 1;
	unsigned char serialBuffer[BUFFER_SIZE];
	for(int i = 0; i< len; i++)
	{
		if (firstLine == 1)
			serialBuffer[i] = data[i];
		if (data[i] == '\n' || data[i] =='\r')
		{
			serialBuffer[i] = '\0';
			firstLine = 0;
			// hit a newline
			//_serialBuffer[_bufferOffset] ='\0';
			//strcpy((char *)serialBuffer, (char *)_serialBuffer);
			//_bufferOffset = 0;
		}		
	}
	return serialBuffer;
}

static void newdata(const unsigned char *data, int len)
{
	packet_parse(data, len);
	ascii_parse(data, len);
	sendDataCallback(data, len);
}

speed_t getBaudRateFromString(const char *baudrate_str) {
    if (strcmp(baudrate_str, "0") == 0) return B0;
    else if (strcmp(baudrate_str, "300") == 0) return B300;
    else if (strcmp(baudrate_str, "1200") == 0) return B1200;
    else if (strcmp(baudrate_str, "2400") == 0) return B2400;
    else if (strcmp(baudrate_str, "4800") == 0) return B4800;
    else if (strcmp(baudrate_str, "9600") == 0) return B9600;
    else if (strcmp(baudrate_str, "19200") == 0) return B19200;
    else if (strcmp(baudrate_str, "38400") == 0) return B38400;
    else if (strcmp(baudrate_str, "57600") == 0) return B57600;
    else if (strcmp(baudrate_str, "115200") == 0) return B115200;
    else if (strcmp(baudrate_str, "230400") == 0) return B230400;
    else {
        return (speed_t)-1; // invalid or unsupported baud rate
    }
}

#if defined(LINUX) || defined(MACOSX)

static int portfd=-1;

int port_is_open(void)
{
	if (portfd > 0) return 1;
	return 0;
}

int open_port_by_name(const char *name)
{
	struct termios termsettings;
	int r;  

	logMessage("into open_port");
	portfd = open(name, O_RDWR | O_NONBLOCK);
	if (portfd < 0) 
	{
		logMessage("open_port failed");
		return 0;
	}
	r = tcgetattr(portfd, &termsettings);
	if (r < 0) {
		logMessage("couldn't get terminal settings");
		close_port();
		return 0;
	}
	logTerminalSettings(termsettings);
	
	cfmakeraw(&termsettings);
	cfsetspeed(&termsettings, B115200);
	
	r = tcsetattr(portfd, TCSANOW, &termsettings);
	if (r < 0) {
	    logMessage("tcsetattr failed");
		close_port();
		return 0;
	}
	
	r = tcgetattr(portfd, &termsettings);
	if (r < 0) {
		logMessage("couldn't get terminal settings");
		close_port();
		return 0;
	}
	logTerminalSettings(termsettings);	
	return 1;
}

int open_port(const char *name, const char *baud, const char *lineEnding)
{
	struct termios termsettings;
	int r;
    char message[60];
    
	logMessage("into open_port");
	portfd = open(name, O_RDWR | O_NONBLOCK);
	snprintf(message, 60, "    portfd: '%d' ", portfd);
	logMessage(message);
	if (portfd < 0) 
	{
		logMessage("open_port failed");
		return -2;
	}
	r = tcgetattr(portfd, &termsettings);
	if (r < 0) {
		logMessage("couldn't get terminal settings");
		close_port();
		return -1;
	}
	
	if (strcmp(lineEnding, "LF") == 0) _lineEndingMode = LINE_ENDING_LF;
	if (strcmp(lineEnding, "CR") == 0) _lineEndingMode = LINE_ENDING_CR;
	if (strcmp(lineEnding, "CRLF") == 0) _lineEndingMode = LINE_ENDING_CRLF;
	
	logTerminalSettings(termsettings);	
	
	cfmakeraw(&termsettings);
	speed_t realBaudRate = getBaudRateFromString(baud);
	cfsetspeed(&termsettings, realBaudRate);
	
	r = tcsetattr(portfd, TCSANOW, &termsettings);
	if (r < 0) {
	    logMessage("tcsetattr failed");
		close_port();
		return -3;
	}
	
	r = tcgetattr(portfd, &termsettings);
	if (r < 0) {
		logMessage("couldn't get terminal settings");
		close_port();
		return -4;
	}
	logTerminalSettings(termsettings);	
	return 1;
}

int read_serial_data(void)
{
    static unsigned char line[BUFFER_SIZE];
    static int lineOffset = 0;
    static unsigned char buffer[BUFFER_SIZE];
    static int bufferIndex = 0;
    static int bytesReadFromSerial = 0;
    static int bytesRemainingToProcess = 0;
    static int nodata_count = 0;

    unsigned char newReadCharacter, lastReadCharacter = 0;
    char message[256];

    if (portfd < 0) {
        logMessage("    portfd < 0");
        return -1;
    }

    if (_lineEndingMode == LINE_ENDING_NOTSET) {
        logMessage("    _lineEndingMode not set");
        return -1;
    }

    // If nothing left to process, read from serial
    if (bytesRemainingToProcess <= 0) {
        bytesReadFromSerial = read(portfd, buffer, BUFFER_SIZE);

        if (bytesReadFromSerial < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000); // 10 ms wait
                return 0;      // no data, not an error
            } else {
                snprintf(message, sizeof(message), "read error: %s", strerror(errno));
                logMessage(message);
                return -1;
            }
        }

        if (bytesReadFromSerial == 0) {
            if (++nodata_count > 6) {
                logMessage("    nodata_count hit 6 — closing port");
                close_port();
                nodata_count = 0;
                return -1;
            }
            return 0;
        }

        // Reset buffer processing state
        nodata_count = 0;
        bufferIndex = 0;
        bytesRemainingToProcess = bytesReadFromSerial;
    }

    while (bufferIndex < bytesReadFromSerial) {
        newReadCharacter = buffer[bufferIndex++];
        bytesRemainingToProcess--;

        // Add char to line
        if (lineOffset < BUFFER_SIZE - 1) 
        {
            line[lineOffset++] = newReadCharacter;
        } else 
        {
            logMessage("line buffer overflow, resetting>>>");
       		line[BUFFER_SIZE-1] = '\0';
        	logMessage((const char *)line);
            logMessage("<<<line buffer overflow, resetting");
            lineOffset = 0;
            continue;
        }

        // Line ending check
        bool lineComplete = false;

        if ((_lineEndingMode == LINE_ENDING_LF && newReadCharacter == '\n') ||
        	(_lineEndingMode == LINE_ENDING_CR && newReadCharacter == '\r'))
        {
            lineComplete = true;
        }
        else if (_lineEndingMode == LINE_ENDING_CRLF && lastReadCharacter == '\r' && newReadCharacter == '\n') {
            line[lineOffset - 2] = '\0'; // strip CR
            lineComplete = true;
        }

        if (lineComplete) {
            line[lineOffset - 1] = '\0'; // strip line ending
            /*logMessage("lineComplete>>>");
            logMessage(line);
            logMessage("<<<lineComplete");*/
            newdata(line, lineOffset);
            lineOffset = 0;
            return lineOffset;  // Successfully read a line
        }

        lastReadCharacter = newReadCharacter;
    }

    return 0;  // Buffer exhausted, but no complete line yet
}

int write_serial_data(const void *ptr, int len)
{
	int n, written=0;
	fd_set wfds;
	struct timeval tv;

	//printf("Write %d\n", len);
	if (portfd < 0) return -1;
	while (written < len) {
		n = write(portfd, (const char *)ptr + written, len - written);
		if (n < 0 && (errno == EAGAIN || errno == EINTR)) n = 0;
		//printf("Write, n = %d\n", n);
		if (n < 0) return -1;
		if (n > 0) {
			written += n;
		} else {
			tv.tv_sec = 0;
			tv.tv_usec = 5000;
			FD_ZERO(&wfds);
			FD_SET(portfd, &wfds);
			n = select(portfd+1, NULL, &wfds, NULL, &tv);
			if (n < 0 && errno == EINTR) n = 1;
			if (n <= 0) return -1;
		}
	}
	return written;
}

void close_port(void)
{
	if (portfd >= 0) {
		close(portfd);
		portfd = -1;
	}
}

#elif defined(WINDOWS)

static HANDLE port_handle=INVALID_HANDLE_VALUE;

int port_is_open(void)
{
	if (port_handle == INVALID_HANDLE_VALUE) return 0;
	return 1;
}

int open_port(const char *name)
{
	COMMCONFIG port_cfg;
	COMMTIMEOUTS timeouts;
	DWORD len;
	char buf[64];
	int n;

	if (strncmp(name, "COM", 3) == 0 && sscanf(name + 3, "%d", &n) == 1) {
		snprintf(buf, sizeof(buf), "\\\\.\\COM%d", n);
		name = buf;
	}
	port_handle = CreateFile(name, GENERIC_READ | GENERIC_WRITE,
		0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (port_handle == INVALID_HANDLE_VALUE) {
		return 0;
	}
	len = sizeof(COMMCONFIG);
	if (!GetCommConfig(port_handle, &port_cfg, &len)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	port_cfg.dcb.BaudRate = 115200;
	port_cfg.dcb.fBinary = TRUE;
	port_cfg.dcb.fParity = FALSE;
	port_cfg.dcb.fOutxCtsFlow = FALSE;
	port_cfg.dcb.fOutxDsrFlow = FALSE;
	port_cfg.dcb.fDtrControl = DTR_CONTROL_DISABLE;
	port_cfg.dcb.fDsrSensitivity = FALSE;
	port_cfg.dcb.fTXContinueOnXoff = TRUE;  // ???
	port_cfg.dcb.fOutX = FALSE;
	port_cfg.dcb.fInX = FALSE;
	port_cfg.dcb.fErrorChar = FALSE;
	port_cfg.dcb.fNull = FALSE;
	port_cfg.dcb.fRtsControl = RTS_CONTROL_DISABLE;
	port_cfg.dcb.fAbortOnError = FALSE;
	port_cfg.dcb.ByteSize = 8;
	port_cfg.dcb.Parity = NOPARITY;
	port_cfg.dcb.StopBits = ONESTOPBIT;
	if (!SetCommConfig(port_handle, &port_cfg, sizeof(COMMCONFIG))) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	if (!EscapeCommFunction(port_handle, CLRDTR | CLRRTS)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
        timeouts.ReadIntervalTimeout            = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier     = 0;
        timeouts.ReadTotalTimeoutConstant       = 0;
        timeouts.WriteTotalTimeoutMultiplier    = 0;
        timeouts.WriteTotalTimeoutConstant      = 0;
        if (!SetCommTimeouts(port_handle, &timeouts)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	if (!EscapeCommFunction(port_handle, SETDTR)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	return 1;
}


int open_port(const char *name, const char *baud, const char *lineEnding)
{
	COMMCONFIG port_cfg;
	COMMTIMEOUTS timeouts;
	DWORD len;
	char buf[64];
	int n;
	speed_t realBaudRate = getBaudRateFromString(baud);	
	if (strcmp(lineEnding, "LF") == 0) _lineEndingMode = LINE_ENDING_LF;
	if (strcmp(lineEnding, "CR") == 0) _lineEndingMode = LINE_ENDING_CR;
	if (strcmp(lineEnding, "CRLF") == 0) _lineEndingMode = LINE_ENDING_CRLF;
	
	if (strncmp(name, "COM", 3) == 0 && sscanf(name + 3, "%d", &n) == 1) {
		snprintf(buf, sizeof(buf), "\\\\.\\COM%d", n);
		name = buf;
	}
	port_handle = CreateFile(name, GENERIC_READ | GENERIC_WRITE,
		0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (port_handle == INVALID_HANDLE_VALUE) {
		return 0;
	}
	len = sizeof(COMMCONFIG);
	if (!GetCommConfig(port_handle, &port_cfg, &len)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	port_cfg.dcb.BaudRate = realBaudRate;
	port_cfg.dcb.fBinary = TRUE;
	port_cfg.dcb.fParity = FALSE;
	port_cfg.dcb.fOutxCtsFlow = FALSE;
	port_cfg.dcb.fOutxDsrFlow = FALSE;
	port_cfg.dcb.fDtrControl = DTR_CONTROL_DISABLE;
	port_cfg.dcb.fDsrSensitivity = FALSE;
	port_cfg.dcb.fTXContinueOnXoff = TRUE;  // ???
	port_cfg.dcb.fOutX = FALSE;
	port_cfg.dcb.fInX = FALSE;
	port_cfg.dcb.fErrorChar = FALSE;
	port_cfg.dcb.fNull = FALSE;
	port_cfg.dcb.fRtsControl = RTS_CONTROL_DISABLE;
	port_cfg.dcb.fAbortOnError = FALSE;
	port_cfg.dcb.ByteSize = 8;
	port_cfg.dcb.Parity = NOPARITY;
	port_cfg.dcb.StopBits = ONESTOPBIT;
	if (!SetCommConfig(port_handle, &port_cfg, sizeof(COMMCONFIG))) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	if (!EscapeCommFunction(port_handle, CLRDTR | CLRRTS)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
        timeouts.ReadIntervalTimeout            = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier     = 0;
        timeouts.ReadTotalTimeoutConstant       = 0;
        timeouts.WriteTotalTimeoutMultiplier    = 0;
        timeouts.WriteTotalTimeoutConstant      = 0;
        if (!SetCommTimeouts(port_handle, &timeouts)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	if (!EscapeCommFunction(port_handle, SETDTR)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	return 1;
}


int read_serial_data(void)
{
	COMSTAT st;
	DWORD errmask=0, num_read, num_request;
	OVERLAPPED ov;
	unsigned char buf[BUFFER_SIZE];
	int r;
	logMessage("read_serial_data: line 753");

	if (port_handle == INVALID_HANDLE_VALUE) return -1;
	while (1) {
		if (!ClearCommError(port_handle, &errmask, &st)) {
			r = -1;
			break;
		}
		//printf("Read, %d requested, %lu buffered\n", count, st.cbInQue);
		if (st.cbInQue <= 0) {
			r = 0;
			break;
		}
		// now do a ReadFile, now that we know how much we can read
		// a blocking (non-overlapped) read would be simple, but win32
		// is all-or-nothing on async I/O and we must have it enabled
		// because it's the only way to get a timeout for WaitCommEvent
		if (st.cbInQue < (DWORD)sizeof(buf)) {
			num_request = st.cbInQue;
		} else {
			num_request = (DWORD)sizeof(buf);
		}
		ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (ov.hEvent == NULL) {
			close_port();
			return -1;
		}
		ov.Internal = ov.InternalHigh = 0;
		ov.Offset = ov.OffsetHigh = 0;
		if (ReadFile(port_handle, buf, num_request, &num_read, &ov)) {
			// this should usually be the result, since we asked for
			// data we knew was already buffered
			//printf("Read, immediate complete, num_read=%lu\n", num_read);
			r = num_read;
		} else {
			if (GetLastError() == ERROR_IO_PENDING) {
				if (GetOverlappedResult(port_handle, &ov, &num_read, TRUE)) {
					//printf("Read, delayed, num_read=%lu\n", num_read);
					r = num_read;
				} else {
					//printf("Read, delayed error\n");
					r = -1;
				}
			} else {
				//printf("Read, error\n");
				r = -1;
			}
		}
		CloseHandle(ov.hEvent);
		if (r <= 0) break;
		newdata(buf, r);
	}
	if (r < 0) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
	}
        return r;
}

int write_serial_data(const void *ptr, int len)
{
	DWORD num_written;
	OVERLAPPED ov;
	int r;

	ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ov.hEvent == NULL) return -1;
	ov.Internal = ov.InternalHigh = 0;
	ov.Offset = ov.OffsetHigh = 0;
	if (WriteFile(port_handle, ptr, len, &num_written, &ov)) {
		//printf("Write, immediate complete, num_written=%lu\n", num_written);
		r = num_written;
	} else {
		if (GetLastError() == ERROR_IO_PENDING) {
			if (GetOverlappedResult(port_handle, &ov, &num_written, TRUE)) {
			//printf("Write, delayed, num_written=%lu\n", num_written);
			r = num_written;
			} else {
				//printf("Write, delayed error\n");
				r = -1;
			}
		} else {
			//printf("Write, error\n");
			r = -1;
		}
	};
	CloseHandle(ov.hEvent);
	return r;
}

void close_port(void)
{
	CloseHandle(port_handle);
	port_handle = INVALID_HANDLE_VALUE;
}


#endif