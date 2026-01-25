#include "imuread.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#define ASCII_STATE_WORD  0
#define ASCII_STATE_RAW   1
#define ASCII_STATE_CAL1  2
#define ASCII_STATE_CAL2  3

int packet_primary_data(const unsigned char *data)
{
	logMessage("into packet_primary_data");
	logMessage((const char *)data);
	current_orientation.q0 = (float)((int16_t)((data[25] << 8) | data[24])) / 30000.0f;
	current_orientation.q1 = (float)((int16_t)((data[27] << 8) | data[26])) / 30000.0f;
	current_orientation.q2 = (float)((int16_t)((data[29] << 8) | data[28])) / 30000.0f;
	current_orientation.q3 = (float)((int16_t)((data[31] << 8) | data[30])) / 30000.0f;
	
	char buffer[255];
	snprintf(buffer,255, "  current_orientation: q0:%f, q1:%f, q2:%f, q3:%f",current_orientation.q0,
		current_orientation.q1, current_orientation.q2, current_orientation.q3);
	logMessage(buffer);
	
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

int packet_magnetic_cal(const unsigned char *data)
{
	int16_t id, x, y, z;
	int n;
	char buffer[255];
	
	id = (data[7] << 8) | data[6];
	x = (data[9] << 8) | data[8];
	y = (data[11] << 8) | data[10];
	z = (data[13] << 8) | data[12];
	
	snprintf(buffer,255, "packet_magnetic_cal  id:%d, x:%d, y:%d, z:%d", id, x, y, z);
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

int packet(const unsigned char *data, int len)
{
	if (len <= 0) return 0;
	print_data("packet:", data, len);
	if (data[0] == 1 && len == 34) {
		logMessage("call packet_primary_data");
		return packet_primary_data(data);
	} else if (data[0] == 6 && len == 14) {
		logMessage("call packet_magnetic_cal");
		return packet_magnetic_cal(data);
	}
	return 0;
}

int packet_encoded(const unsigned char *data, int len)
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

int packet_parse(const unsigned char *data, int len)
{
	static unsigned char packetbuf[BUFFER_SIZE];
	static unsigned int packetlen=0;
	const unsigned char *p;
	int copylen;
	int ret=0;

	while (len > 0) {
		p = memchr(data, 0x7E, len);
		if (p == NULL) {
			if (packetlen + len > sizeof(packetbuf)) {
				packetlen = 0;
				logMessage("would overflow buffer - p == NULL");
				return 0;  // would overflow buffer
			}
			memcpy(packetbuf+packetlen, data, len);
			packetlen += len;
			len = 0;
		} else if (p > data) {
			copylen = p - data;
			if (packetlen + copylen > sizeof(packetbuf)) {
				packetlen = 0;
				logMessage("would overflow buffer - p > data");
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

int ascii_parse(const unsigned char *data, int len)
{
	static int ascii_state=ASCII_STATE_WORD;
	static int ascii_num=0, ascii_neg=0, ascii_count=0;
	static int16_t ascii_raw_data[9];
	static float ascii_cal_data[10];
	static unsigned int ascii_raw_data_count=0;
	const char *p, *end;
	int ret=0;
	char messageBuffer[255];

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
				if (ascii_count > 0) goto fail;
				ascii_neg = 1;
			} else if (isdigit(*p)) {
				ascii_num = ascii_num * 10 + *p - '0';
				ascii_count++;
			} else if (*p == ',') {
				if (ascii_neg) ascii_num = -ascii_num;
				if (ascii_num < -32768 || ascii_num > 32767) goto fail;
				if (ascii_raw_data_count >= 8) goto fail;
				ascii_raw_data[ascii_raw_data_count++] = ascii_num;
				ascii_num = 0;
				ascii_neg = 0;
				ascii_count = 0;
			} else if (*p == 13) {
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
								
			} else if (*p == 10 || *p == 32) {
			} else { 
				snprintf(messageBuffer, 255, "ascii parse error-%d",*p);
				logMessage(messageBuffer);
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
					logMessage("call cal1_data");
					cal1_data(ascii_cal_data);
				} else if (ascii_state == ASCII_STATE_CAL2) {
					logMessage("call cal2_data");
					cal2_data(ascii_cal_data);
				}
				
				ret = 1;
				ascii_raw_data_count = 0;
				ascii_num = 0;
				ascii_neg = 0;
				ascii_count = 0;
				ascii_state = ASCII_STATE_WORD;
			} else if (*p == 10 || *p == 32) {
			} else {
				goto fail;
			}
		}
	}
	return ret;
fail:
	logMessage("ascii FAIL\n");
	ascii_state = ASCII_STATE_WORD;
	ascii_raw_data_count = 0;
	ascii_num = 0;
	ascii_neg = 0;
	ascii_count = 0;
	return 0;
}

