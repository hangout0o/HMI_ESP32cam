# HMI_ESP32CAM
## Overview
Here Esp32cam take pictures(RGB656) continuously and detects human face with the help of Esp-dl Library 
If human face detected it will be saved image(jpg format wil be converted from RGB656) to SDcard attacted 
to esp32 cam. If any uart input is received to Esp32cam, It will send image to uart.

## Get Started with the Project
### Hardware
This project is based on ESP32camera moudle (AI_THINKER)            
For other development boards designed by Espressif
- Modify [sdkconfig.defaults](/sdkconfig.defaults) and [Partiions.csv](./partitions.csv) 
-  Modify pins for Camera, SDcard, Uart

### Software
Get ESP-IDF
For details on getting ESP-IDF, please refer to [ESP-IDF Extension](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md)

Next Clone this Repo or Download and Extract the zip file
```bash
git clone https://github.com/hangout0o/HMI_ESP32cam.git
```
### Library
[ESP32-CAMERA DRIVER](https://github.com/espressif/esp32-camera)

This is included in [components](/components) folder, path of the library has been defined in [CMakeList.TXT](/CMakeLists.txt)

[ESP_DL](https://github.com/espressif/esp-dl)

This is included in [components](/components) folder, path fo the libary has been defined in [CMakeList.TXT](/CMakeLists.txt)
> When compiling the code if you face error regarding esp-dl dependcy version v4.4*
You have Two ways to solve this Error
- Install the latest ESP-IDF on the [release/v4.4](https://github.com/espressif/esp-idf/tree/release/v4.4) branch.
- Remove the depency line on [idf_component.yml](/components/esp-dl/idf_component.yml)

    ![image](https://github.com/hangout0o/HMI_ESP32cam/assets/92089548/0449f4bf-51c7-4c92-8dbb-c9ea52f9d14c)

## Repo Strucuture
```bash
HMI_ESP32cam
├── components                    // Libraries
│   ├── eso-dl                    
│   └── esp32-camera-master                  
├── main            
│   ├── CmakeLists.txt        
│   └──main.cpp
├── CMakeLists.txt              //In this path of Libraries is defined 
├── README.md                  // you are here
├── partitions.csv
├── sdkconfig.defaults
└── sdkconfig.defaults.esp32            
```
## Code Flow

![Code_Flow](https://github.com/hangout0o/HMI_ESP32cam/assets/92089548/2660fd2e-9a4f-4293-83e0-bac0acf9eb1c)


