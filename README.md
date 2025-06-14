# MotionCal
A fork from https://github.com/PaulStoffregen/MotionCal

The tool, as cloned didn't work on my Mac - it wouldn't show the list of ports. So I tried fixing it. Then I found that the list of ports was duplicated, and too narrow. 

I fiddled with the UI to make the ports list wider. Then I got distracted and started playing. 

Here's how it looks now:

![Screenshot](NewUI.jpg)

I've added:
1) Option to set the baud rate. 
2) Option to specify the line ending.
3) Pause button.
4) 'Received Data' panel, which shows:
   * The floats from the 'Raw:' messages.
   * 'Orientation' data received from an 'Ori:' message, which has three floats containing yaw, roll, pitch.
6) Messages panel, used to churn out messages. 'Gaps' data is shown when it has changed from the last data update.

# Internal changes
## `logging.c`
The code now builds a log as it's going: `log.txt`. To add new lines to the log, use the `logMessage` function. `debugPrint` can be used for listing out the content of a `const unsigned char *` to `log.txt`. `logTerminalSettings` logs some members of an `termios` instance. 

## `serialdata.c`
I've split `serialdata.c` into `serialdata.c`, `serialdata.messaging.c` and `serialdata.parsing.c` - as I added new callback routes I found the class was getting too large for my brain.

I've updated the reading of new lines from the serial port - it kept failing as is. This might be to macOS changes - I've no reason to think the code itself was wrong.

## `gui.cpp`
I've split the UI building code out into separate function. When adding the panels, the existing function grew to a few hundred lines.

## Callbacks

When new Raw, Cal1, Cal2 or Ori messages are read from the console, they're now sent back to `gui.cpp` using callbacks. e.g:

```
void fireImuCallback(ImuData data)
{
    if (_imuDataCallback != NULL)
        _imuDataCallback(data);
}
```

The callbacks are set up in `gui.cpp` as:
```
// Set a callback function for when there's grid data to display.
// Doing this enables us to decouple events. It also allows us to 
// reduce the chance of race conditions.
void MyFrame::BuildBufferDisplayCallBack()
{
    MyFrame::instance = this;
    setImuDataCallback(MyFrame::StaticUpdateImuData);
    setOrientationDataCallback(MyFrame::StaticUpdateOrientationData);
    setUnknownMessageCallback(MyFrame::StaticUnknownMessageReceived);
    setOffsetsCalibrationCallback(MyFrame::StaticOffsetCalibrationDataReceived);
    setSoftIronCalibrationCallback(MyFrame::StaticSoftIronCalibrationDataReceived);
}
```

To wire this up, there's a method in `serialdata.messaging.c`: 
```
void setImuDataCallback(imuDataCallback imuDataCallback)
{
    _imuDataCallback = imuDataCallback;
}
```

This is used in `serialdata..c` by calling:
```
void fireImuCallback(ImuData data)
{
    if (_imuDataCallback != NULL)
        _imuDataCallback(data);
}
```

This is called when we've determined that we've received a Raw message.


The static method in `gui.cpp` is:
```
void MyFrame::StaticUpdateImuData(ImuData imuData) {
    if (instance) 
        instance->UpdateImuData(imuData);
}
```

And the actual callback function is:
```
void MyFrame::UpdateImuData(ImuData imuData)
{
    char buffer[20];

    snprintf(buffer,20, "%f", imuData.accelerometer.x);
    _rawDataGrid->SetCellValue(X_ROW, ACCEL_COL,buffer);
    snprintf(buffer,20,"%f", imuData.accelerometer.y);    
    _rawDataGrid->SetCellValue(Y_ROW, ACCEL_COL,buffer);
    snprintf(buffer,20,"%f", imuData.accelerometer.z);    
    _rawDataGrid->SetCellValue(Z_ROW, ACCEL_COL,buffer);

    snprintf(buffer,20,"%f", imuData.gyroscope.x);
    _rawDataGrid->SetCellValue(X_ROW, GYRO_COL,buffer);
    snprintf(buffer,20,"%f", imuData.gyroscope.y);    
    _rawDataGrid->SetCellValue(Y_ROW, GYRO_COL,buffer);
    snprintf(buffer,20,"%f", imuData.gyroscope.z);    
    _rawDataGrid->SetCellValue(Z_ROW, GYRO_COL,buffer);
    
    snprintf(buffer,20,"%f", imuData.magnetometer.x);
    _rawDataGrid->SetCellValue(X_ROW, MAG_COL,buffer);
    snprintf(buffer,20,"%f", imuData.magnetometer.y);    
    _rawDataGrid->SetCellValue(Y_ROW, MAG_COL,buffer);
    snprintf(buffer,20,"%f", imuData.magnetometer.z);    
    _rawDataGrid->SetCellValue(Z_ROW, MAG_COL,buffer);
    
    ProcessImuDataFromCallback(imuData);
    LogImuData(imuData);
}

```


# How to build
I asked ChatGPT how to build MotionCal on a Mac

Building MotionCal on macOS requires setting up the necessary dependencies and compiling the project. Follow these steps:

## 1. Install Dependencies
You'll need:

```
wxWidgets (GUI framework)
CMake (build system)
Xcode or g++ (compiler)

```

Install via Homebrew:
`brew install wxwidgets cmake`

## 2. Clone the Repository
```
git clone https://github.com/PaulStoffregen/MotionCal.git`
cd MotionCal

```

## 3. Configure wxWidgets
Ensure that wx-config is accessible:

```
export PATH="/usr/local/opt/wxwidgets/bin:$PATH"
export LDFLAGS="-L/usr/local/opt/wxwidgets/lib"
export CPPFLAGS="-I/usr/local/opt/wxwidgets/include"
export PKG_CONFIG_PATH="/usr/local/opt/wxwidgets/lib/pkgconfig"

```

## 4. Build MotionCal

Using Make:
`make`
Using CMake:

```
mkdir build
cd build
cmake ..
make
```

## 5. Run MotionCal
`./MotionCal`


# Troubleshooting:
If wx-config is missing, check brew info wxwidgets to see if you need to add additional paths.
If you encounter compilation errors, ensure you have a C++ compiler (g++ or clang) installed.
Let me know if you hit any issues!
