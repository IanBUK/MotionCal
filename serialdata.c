#include "imuread.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#define BUFFER_SIZE 512

// data callback wrapper, inflating data from unsigned char* into ImuData or Vector3D 
// depending on raw data
void sendDataCallback(const unsigned char *data, int len)
{
    if (len <= 0 || data == NULL) return;

    char buffer[len + 1];
    memcpy(buffer, data, len);
    buffer[len] = '\0'; // null-terminate
    if (memcmp(buffer, "Raw", 3) == 0)
    {
        char *token = strtok(buffer, " \r\n"); // "Raw"
        token = strtok(NULL, " \r\n");         // CSV part

        if (!token) {
            logMessage("Malformed Raw data: no CSV payload");
            return;
        }

        ImuData imuData;
        char *val = strtok(token, ",");
        if (!val) { logMessage("Missing accel.x"); return; }
        imuData.accelerometer.x = strtof(val, NULL);

        val = strtok(NULL, ",");
        if (!val) { logMessage("Missing accel.y"); return; }
        imuData.accelerometer.y = strtof(val, NULL);

        val = strtok(NULL, ",");
        if (!val) { logMessage("Missing accel.z"); return; }
        imuData.accelerometer.z = strtof(val, NULL);

        val = strtok(NULL, ",");
        if (!val) { logMessage("Missing gyro.x"); return; }
        imuData.gyroscope.x = strtof(val, NULL);

        val = strtok(NULL, ",");
        if (!val) { logMessage("Missing gyro.y"); return; }
        imuData.gyroscope.y = strtof(val, NULL);

        val = strtok(NULL, ",");
        if (!val) { logMessage("Missing gyro.z"); return; }
        imuData.gyroscope.z = strtof(val, NULL);

        val = strtok(NULL, ",");
        if (!val) { logMessage("Missing mag.x"); return; }
        imuData.magnetometer.x = strtof(val, NULL);

        val = strtok(NULL, ",");
        if (!val) { logMessage("Missing mag.y"); return; }
        imuData.magnetometer.y = strtof(val, NULL);

        val = strtok(NULL, ",");
        if (!val) { logMessage("Missing mag.z"); return; }
        imuData.magnetometer.z = strtof(val, NULL);
        fireImuCallback(imuData);
    }
    else if (memcmp(buffer, "Ori", 3) == 0)
    {
        char *token = strtok(buffer, " "); // "Ori"
        token = strtok(NULL, " ");         // CSV part

        if (!token) {
            logMessage("Malformed Ori data: no CSV payload");
            return;
        }

        YawPitchRoll orientation;

        char *val = strtok(token, ",");
        if (!val) { logMessage("Missing ori.x"); return; }
        orientation.yaw = strtof(val, NULL);

        val = strtok(NULL, ",");
        if (!val) { logMessage("Missing ori.y"); return; }
        orientation.pitch = strtof(val, NULL);

        val = strtok(NULL, ",");
        if (!val) { logMessage("Missing ori.z"); return; }
        orientation.roll = strtof(val, NULL);

        fireOrientationCallback(orientation);
    }
    else if (memcmp(buffer, "Cal1", 4) == 0)
    {
    	#define MAX_OFFSETS 9
		int floatCount = 0;
		// Find the start of the float data after the colon
		char *dataPtr = strchr(buffer, ':');
		if (dataPtr != NULL) {
			OffsetsCalibrationData offsets;
			dataPtr++;  // Skip the colon
			
			// Copy the data part into a local buffer so strtok doesn't modify the original
			char dataCopy[256];
			strncpy(dataCopy, dataPtr, sizeof(dataCopy));
			dataCopy[sizeof(dataCopy) - 1] = '\0';  // Ensure null termination

			char *token = strtok(dataCopy, ",");
			while (token != NULL && floatCount < MAX_OFFSETS) {
				offsets.offsetData[floatCount++] = strtof(token, NULL);
				token = strtok(NULL, ",");
			}
			if (token != NULL)
			{
				offsets.calMag = strtof(token, NULL);
			}
			fireOffsetsCalibrationCallback(offsets);	
		} 
		else 
		{
			logMessage("    Error: Invalid Offsets format, colon not found.\n");
		}   
    }
    else if (memcmp(buffer, "Cal2", 4) == 0)
    {
       	#define MAX_CALDATA 10
		int floatCount = 0;
		// Find the start of the float data after the colon
		char *dataPtr = strchr(buffer, ':');
		if (dataPtr != NULL) {
			SoftIronCalibrationData softIron;
			dataPtr++;  // Skip the colon
	
			// Copy the data part into a local buffer so strtok doesn't modify the original
			char dataCopy[256];
			strncpy(dataCopy, dataPtr, sizeof(dataCopy));
			dataCopy[sizeof(dataCopy) - 1] = '\0';  // Ensure null termination

			char *token = strtok(dataCopy, ",");
			while (token != NULL && floatCount < MAX_CALDATA) {
				softIron.softIronData[floatCount++] = strtof(token, NULL);
				token = strtok(NULL, ",");
			}
			fireSoftIronCalibrationCallback(softIron);	
		} 
		else 
		{
			logMessage("    Error: Invalid SoftIron format, colon not found.\n");
		}  
   }
    else
    {
     	fireUnknownMessageCallback(data, len);
    }
}

// Logging functions
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

static int packet_primary_data(const unsigned char *data)
{
	logMessage("into packet_primary_data");
	logMessage((const char *)data);
	//current_position.x = (float)((int16_t)((data[13] << 8) | data[12])) / 10.0f;
	//current_position.y = (float)((int16_t)((data[15] << 8) | data[14])) / 10.0f;
	//current_position.z = (float)((int16_t)((data[17] << 8) | data[16])) / 10.0f;
	current_orientation.q0 = (float)((int16_t)((data[25] << 8) | data[24])) / 30000.0f;
	current_orientation.q1 = (float)((int16_t)((data[27] << 8) | data[26])) / 30000.0f;
	current_orientation.q2 = (float)((int16_t)((data[29] << 8) | data[28])) / 30000.0f;
	current_orientation.q3 = (float)((int16_t)((data[31] << 8) | data[30])) / 30000.0f;
	
	char buffer[255];
	snprintf(buffer,255, "  current_orientation: q0:%f, q1:%f, q2:%f, q3:%f",current_orientation.q0,
		current_orientation.q1, current_orientation.q2, current_orientation.q3);
	logMessage(buffer);
	/*snprintf(buffer,255, "  current_orientation: w:%f, x:%f, y:%f, z:%f",current_orientation.w,
		current_orientation.x, current_orientation.y, current_orientation.z);
	logMessage(buffer);*/
	/*snprintf(buffer,255, "  current_position: x:%f, y:%f, z:%f",
		current_position.x, current_position.y, current_position.z);
	logMessage(buffer);	*/	
	
	
#if 0
	printf("mag data, %5.2f %5.2f %5.2f\n",
		current_position.x,
		current_position.y,
		current_position.z
	);
#endif
#if 0
	printf("orientation: %5.3f %5.3f %5.3f %5.3f\n",
		current_orientation.w,
		current_orientation.x,
		current_orientation.y,
		current_orientation.z
	);
#endif
	return 1;
}

static int packet_magnetic_cal(const unsigned char *data)
{
	int16_t id, x, y, z;
	int n;
	char buffer[255];
	
	id = (data[7] << 8) | data[6];
	x = (data[9] << 8) | data[8];
	y = (data[11] << 8) | data[10];
	z = (data[13] << 8) | data[12];

	snprintf(buffer,255, "  id:%d, x:%d, y:%d, z:%d", id, x, y, z);
	logMessage(buffer);	
	
	
	if (id == 1) {
		magcal.V[0] = (float)x * 0.1f;
		magcal.V[1] = (float)y * 0.1f;
		magcal.V[2] = (float)z * 0.1f;
		snprintf(buffer,255, "  magcal.V[0]:%f, .V[1]:%f, .V[2]:%f", magcal.V[0], magcal.V[1], magcal.V[2]);
		logMessage(buffer);	
		return 1;
	} else if (id == 2) {
		magcal.invW[0][0] = (float)x * 0.001f;
		magcal.invW[1][1] = (float)y * 0.001f;
		magcal.invW[2][2] = (float)z * 0.001f;
		snprintf(buffer,255, "  magcal.invW[0][0]:%f, invW[1][1]:%f, invW[2][2]:%f", magcal.invW[0][0], magcal.invW[1][1], magcal.invW[2][2]);
		logMessage(buffer);	
		return 1;
	} else if (id == 3) {
		magcal.invW[0][1] = (float)x / 1000.0f;
		magcal.invW[1][0] = (float)x / 1000.0f; // TODO: check this assignment
		magcal.invW[0][2] = (float)y / 1000.0f;
		magcal.invW[1][2] = (float)y / 1000.0f; // TODO: check this assignment
		magcal.invW[1][2] = (float)z / 1000.0f;
		magcal.invW[2][1] = (float)z / 1000.0f; // TODO: check this assignment
		snprintf(buffer,255, "  magcal.invW[0][1]:%f, magcal.invW[1][0]:%f, magcal.invW[0][2]:%f, magcal.invW[1][2]:%f, magcal.invW[1][2]:%f, magcal.invW[2][1]:%f",
			magcal.invW[0][1], magcal.invW[1][0], magcal.invW[0][2], magcal.invW[1][2] , magcal.invW[1][2], magcal.invW[2][1]);
		logMessage(buffer);	
		return 1;
	} else if (id >= 10 && id < MAGBUFFSIZE+10) {
		n = id - 10;
		if (magcal.valid[n] == 0 || x != magcal.BpFast[0][n]
		  || y != magcal.BpFast[1][n] || z != magcal.BpFast[2][n]) {
			magcal.BpFast[0][n] = x;
			magcal.BpFast[1][n] = y;
			magcal.BpFast[2][n] = z;
			magcal.valid[n] = 1;
			//printf("mag cal, n=%3d: %5d %5d %5d\n", n, x, y, z);
			snprintf(buffer,255, "  magcal.BpFast[0][n]: %d, ,magcal.BpFast[1][n]: %d, , magcal.BpFast[2][n]: %d",
				magcal.BpFast[0][n],magcal.BpFast[1][n], magcal.BpFast[2][n]);
		logMessage(buffer);	
		}
		return 1;
	}
	return 0;
}

static int packet(const unsigned char *data, int len)
{
	if (len <= 0) return 0;
	//print_data("packet", data, len);
	logMessage("into packet");
	logMessage((const char *)data);
	if (data[0] == 1 && len == 34) {
		return packet_primary_data(data);
	} else if (data[0] == 6 && len == 14) {
		return packet_magnetic_cal(data);
	}
	return 0;
}

static int packet_encoded(const unsigned char *data, int len)
{
	const unsigned char *p;
	unsigned char buf[BUFFER_SIZE];
	int buflen=0, copylen;
	logMessage("into packet_encoded");
	logMessage((const char *)data);
	//printf("packet_encoded, len = %d\n", len);
	p = memchr(data, 0x7D, len);
	if (p == NULL) {
		return packet(data, len);
	} else {
		//printf("** decoding necessary\n");
		while (1) {
			copylen = p - data;
			if (copylen > 0) {
				//printf("  copylen = %d\n", copylen);
				if (buflen + copylen > sizeof(buf)) return 0;
				memcpy(buf+buflen, data, copylen);
				buflen += copylen;
				data += copylen;
				len -= copylen;
			}
			if (buflen + 1 > sizeof(buf)) return 0;
			buf[buflen++] = (p[1] == 0x5E) ? 0x7E : 0x7D;
			data += 2;
			len -= 2;
			if (len <= 0) break;
			p = memchr(data, 0x7D, len);
			if (p == NULL) {
				if (buflen + len > sizeof(buf)) return 0;
				memcpy(buf+buflen, data, len);
				buflen += len;
				break;
			}
		}
		//printf("** decoded to %d\n", buflen);
		return packet(buf, buflen);
	}
}

static int packet_parse(const unsigned char *data, int len)
{
	static unsigned char packetbuf[BUFFER_SIZE];
	static unsigned int packetlen=0;
	const unsigned char *p;
	int copylen;
	int ret=0;

	logMessage("packet_parse");
	logMessage((const char *)data);
	while (len > 0) {
		p = memchr(data, 0x7E, len);
		if (p == NULL) {
			if (packetlen + len > sizeof(packetbuf)) {
				packetlen = 0;
				return 0;  // would overflow buffer
			}
			memcpy(packetbuf+packetlen, data, len);
			packetlen += len;
			len = 0;
		} else if (p > data) {
			copylen = p - data;
			if (packetlen + copylen > sizeof(packetbuf)) {
				packetlen = 0;
				return 0;  // would overflow buffer
			}
			memcpy(packetbuf+packetlen, data, copylen);
			packet_encoded(packetbuf, packetlen+copylen);
			packetlen = 0;
			data += copylen + 1;
			len -= copylen + 1;
		} else {
			if (packetlen > 0) {
				if (packet_encoded(packetbuf, packetlen)) ret = 1;
				packetlen = 0;
			}
			data++;
			len--;
		}
	}
	return ret;
}

#define ASCII_STATE_WORD  0
#define ASCII_STATE_RAW   1
#define ASCII_STATE_CAL1  2
#define ASCII_STATE_CAL2  3


static int ascii_parse(const unsigned char *data, int len)
{
	static int ascii_state=ASCII_STATE_WORD;
	static int ascii_num=0, ascii_neg=0, ascii_count=0;
	static int16_t ascii_raw_data[9];
	static float ascii_cal_data[10];
	static unsigned int ascii_raw_data_count=0;
	const char *p, *end;
	int ret=0;

	//print_data("ascii_parse", data, len);
	end = (const char *)(data + len);
	for (p = (const char *)data ; p < end; p++) {
		if (ascii_state == ASCII_STATE_WORD) {
			if (ascii_count == 0) {

				if (*p == 'R') {
					ascii_num = ASCII_STATE_RAW;
					ascii_count = 1;
				} else if (*p == 'C') {
					ascii_num = ASCII_STATE_CAL1;
					ascii_count = 1;
				}
			} else if (ascii_count == 1) {
				if (*p == 'a') {
					ascii_count = 2;
				} else {
					ascii_num = 0;
					ascii_count = 0;
				}
			} else if (ascii_count == 2) {
				if (*p == 'w' && ascii_num == ASCII_STATE_RAW) {
					ascii_count = 3;
				} else if (*p == 'l' && ascii_num == ASCII_STATE_CAL1) {
					ascii_count = 3;
				} else {
					ascii_num = 0;
					ascii_count = 0;
				}
			} else if (ascii_count == 3) {

				if (*p == ':' && ascii_num == ASCII_STATE_RAW) {
					ascii_state = ASCII_STATE_RAW;
					ascii_raw_data_count = 0;
					ascii_num = 0;
					ascii_count = 0;
				} else if (*p == '1' && ascii_num == ASCII_STATE_CAL1) {
					ascii_count = 4;
				} else if (*p == '2' && ascii_num == ASCII_STATE_CAL1) {
					ascii_num = ASCII_STATE_CAL2;
					ascii_count = 4;
				} else {
					ascii_num = 0;
					ascii_count = 0;
				}
			} else if (ascii_count == 4) {

				if (*p == ':' && ascii_num == ASCII_STATE_CAL1) {
					ascii_state = ASCII_STATE_CAL1;
					ascii_raw_data_count = 0;
					ascii_num = 0;
					ascii_count = 0;
				} else if (*p == ':' && ascii_num == ASCII_STATE_CAL2) {
					ascii_state = ASCII_STATE_CAL2;
					ascii_raw_data_count = 0;
					ascii_num = 0;
					ascii_count = 0;
				} else {
					ascii_num = 0;
					ascii_count = 0;
				}
			} else {
				goto fail;
			}
		} else if (ascii_state == ASCII_STATE_RAW) {
			if (*p == '-') {
				//printf("ascii_parse negative\n");
				if (ascii_count > 0) goto fail;
				ascii_neg = 1;
			} else if (isdigit(*p)) {
				//printf("ascii_parse digit\n");
				ascii_num = ascii_num * 10 + *p - '0';
				ascii_count++;
			} else if (*p == ',') {
				//printf("ascii_parse comma, %d\n", ascii_num);
				if (ascii_neg) ascii_num = -ascii_num;
				if (ascii_num < -32768 || ascii_num > 32767) goto fail;
				if (ascii_raw_data_count >= 8) goto fail;
				ascii_raw_data[ascii_raw_data_count++] = ascii_num;
				ascii_num = 0;
				ascii_neg = 0;
				ascii_count = 0;
			} else if (*p == 13) {
				//printf("ascii_parse newline\n");
				if (ascii_neg) ascii_num = -ascii_num;
				if (ascii_num < -32768 || ascii_num > 32767) goto fail;
				if (ascii_raw_data_count != 8) goto fail;
				ascii_raw_data[ascii_raw_data_count] = ascii_num;
				raw_data(ascii_raw_data);
				ret = 1;
				ascii_raw_data_count = 0;
				ascii_num = 0;
				ascii_neg = 0;
				ascii_count = 0;
				ascii_state = ASCII_STATE_WORD;
								
			} else if (*p == 10) {
			} else {
				goto fail;
			}
		} else if (ascii_state == ASCII_STATE_CAL1 || ascii_state == ASCII_STATE_CAL2) {
			if (*p == '-') {
				//printf("ascii_parse negative\n");
				if (ascii_count > 0) goto fail;
				ascii_neg = 1;
			} else if (isdigit(*p)) {
				//printf("ascii_parse digit\n");
				ascii_num = ascii_num * 10 + *p - '0';
				ascii_count++;
			} else if (*p == '.') {
				//printf("ascii_parse decimal, %d\n", ascii_num);
				if (ascii_raw_data_count > 9) goto fail;
				ascii_cal_data[ascii_raw_data_count] = (float)ascii_num;
				ascii_num = 0;
				ascii_count = 0;
			} else if (*p == ',') {
				//printf("ascii_parse comma, %d\n", ascii_num);
				if (ascii_raw_data_count > 9) goto fail;
				ascii_cal_data[ascii_raw_data_count] +=
					(float)ascii_num / powf(10.0f, ascii_count);
				if (ascii_neg) ascii_cal_data[ascii_raw_data_count] *= -1.0f;
				ascii_raw_data_count++;
				ascii_num = 0;
				ascii_neg = 0;
				ascii_count = 0;
			} else if (*p == 13) {
				//printf("ascii_parse newline\n");
				if ((ascii_state == ASCII_STATE_CAL1 && ascii_raw_data_count != 9)
				 || (ascii_state == ASCII_STATE_CAL2 && ascii_raw_data_count != 8))
					goto fail;
				ascii_cal_data[ascii_raw_data_count] +=
					(float)ascii_num / powf(10.0f, ascii_count);
				if (ascii_neg) ascii_cal_data[ascii_raw_data_count] *= -1.0f;
				if (ascii_state == ASCII_STATE_CAL1) {
					cal1_data(ascii_cal_data);
				} else if (ascii_state == ASCII_STATE_CAL2) {
					cal2_data(ascii_cal_data);
				}
				
				ret = 1;
				ascii_raw_data_count = 0;
				ascii_num = 0;
				ascii_neg = 0;
				ascii_count = 0;
				ascii_state = ASCII_STATE_WORD;
			} else if (*p == 10) {
			} else {
				goto fail;
			}
		}
	}
	return ret;
fail:
	//printf("ascii FAIL\n");
	ascii_state = ASCII_STATE_WORD;
	ascii_raw_data_count = 0;
	ascii_num = 0;
	ascii_neg = 0;
	ascii_count = 0;
	return 0;
}


static void newdata(const unsigned char *data, int len)
{
	packet_parse(data, len);
	ascii_parse(data, len);
	buildBuffer(data, len);
	sendDataCallback(data, len);
	//fireBufferDisplayCallback(data, len);
}

#if defined(LINUX) || defined(MACOSX)

static int portfd=-1;
typedef enum {
    LINE_ENDING_NOTSET,
    LINE_ENDING_LF,    // "\n"
    LINE_ENDING_CR,    // "\r"
    LINE_ENDING_CRLF   // "\r\n"
} LineEnding;

LineEnding _lineEndingMode = LINE_ENDING_NOTSET;

int port_is_open(void)
{
	if (portfd > 0) return 1;
	return 0;
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