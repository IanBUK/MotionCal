#include "imuread.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#define BUFFER_SIZE 512

// Define Callbacks
typedef void (*displayBufferCallback)(const unsigned char *serialBufferMessage, int bytesRead);
typedef void (*imuDataCallback)(ImuData rawData);
typedef void (*orientationDataCallback)(YawPitchRoll orientation);
typedef void (*unknownMessageCallback)(const unsigned char *serialBufferMessage, int bytesRead);
typedef void (*calibrationOffsetsCallback)(OffsetsCalibrationData calibrationOffsets);
typedef void (*calibrationSoftIronCallback)(SoftIronCalibrationData calibrationSoftIron);

// Define local references to callbacks
displayBufferCallback _displayBufferCallback;
imuDataCallback _imuDataCallback;
orientationDataCallback _orientationDataCallback;
unknownMessageCallback _unknownMessageCallback;
calibrationOffsetsCallback _offsetsCallback;
calibrationSoftIronCallback _softIronCallback;

// Setters to callbacks
void setDisplayBufferCallback(displayBufferCallback displayBufferCallback)
{
	_displayBufferCallback = displayBufferCallback;
}

void setImuDataCallback(imuDataCallback imuDataCallback)
{
	_imuDataCallback = imuDataCallback;
}

void setOrientationDataCallback(orientationDataCallback orientationDataCallback)
{
	_orientationDataCallback = orientationDataCallback;
}

void setUnknownMessageCallback(unknownMessageCallback unknownMessageCallback)
{
	_unknownMessageCallback = unknownMessageCallback;
}

void setOffsetsCalibrationCallback(calibrationOffsetsCallback offsetsCallback)
{
	_offsetsCallback = offsetsCallback;
}

void setSoftIronCalibrationCallback(calibrationSoftIronCallback softIronCallback)
{
	_softIronCallback = softIronCallback;
}

// Wrappers around callbacks, adding null-safety tests
void fireBufferDisplayCallback(const unsigned char *data, int len)
{
	if(_displayBufferCallback != NULL)
		_displayBufferCallback(data, len);
}

void fireImuCallback(ImuData data)
{
	if (_imuDataCallback != NULL)
		_imuDataCallback(data);
}

void fireOrientationCallback(YawPitchRoll orientation)
{
	if (_orientationDataCallback != NULL)
		_orientationDataCallback(orientation);
}

void fireUnknownMessageCallback(const unsigned char *data, int len)
{
	if (_unknownMessageCallback != NULL)
		_unknownMessageCallback(data, len);
}

void fireOffsetsCalibrationCallback(OffsetsCalibrationData calibrationOffsets)
{
	if (_offsetsCallback != NULL)
	{
		_offsetsCallback(calibrationOffsets);
	}
	else
		logMessage("offsets callback not set");
}

void fireSoftIronCalibrationCallback(SoftIronCalibrationData calibrationSoftIron)
{
	if (_softIronCallback != NULL)
		_softIronCallback(calibrationSoftIron);
	else
		logMessage("softIronCallback not set");
}

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


