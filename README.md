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