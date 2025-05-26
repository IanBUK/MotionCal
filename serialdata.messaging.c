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

