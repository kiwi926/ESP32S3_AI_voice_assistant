# ESP32S3 AI voice assistant
ESP32S3 AI voice assistant is a voice interaction system based on ESP32S3, implemented with Arduino IDE. In this program, we used Baidu Cloud to do STT(Speech to Text) and TTS(Text to Speech). The LLM(Large Language Model) used in the system is TONGYIQIANWEN from Aliyun.

Arduino IDE version: V2.3.2 <br>
Arduino ESP32 version: V3.0.4 (Setting in Tools > Board > Boards Manager...)<br>

libraies needed:
```
#include <Arduino.h>
#include "base64.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include <ESP_I2S.h>
#include <ArduinoJson.h>
#include <UrlEncode.h>
```

Code Execution Result:
![alt text](./img/image-23.png)

## Hardware
1. Seeed Studio XIAO ESP32S3 Sense with digital MIC.

    Link:https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
![alt text](<./img/Seeed Studio ESP32S3 Sense.jpg>)
2. MAX98357A I2S audio amplifier module + Speaker.

![alt text](<./img/MAX98357 module.png>)

3. Button:  https://item.szlcsc.com/580410.html

## Pins connection
### Schematic

Please note that the pins name "D3.D4.D5" on ESP32S3 board are corrosponding to GPIO4. GPIO5 and GPIO6. 
![alt text](<./img/pin connection.001.jpeg>)
![alt text](./img/DSC00963.JPG)

Pins setup code:
```
const uint8_t key = 3;        
const uint8_t I2S_LRC = 4;    
const uint8_t I2S_BCLK = 5;
const uint8_t I2S_DOUT = 6;  
const uint8_t I2S_DIN = 41;   
const uint8_t I2S_SCK = 42; 
```

## Arduino IDE
1. Arduino IDE version: 
<span style="color: red;"> **V2.3.2** </span>

    Download link: https://www.arduino.cc/en/software, choose the right option according to you compuer, simply follow the instructions to install. My option: Windows Win10 and newer, 64 bits.
    ![alt text](./img/image-22.png)
2. Add additional boards manager URLs(to add ESP32 packages in next step).

    Navigate to File > Preferences..., and fill "Additional Boards Manager URLs" with the url below:

    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
![alt text](./img/image-24.png)

3. esp32 board version: 
<span style="color: red;"> **V3.0.4** </span>

    Navigate to Tools > Board > Boards Manager..., type the keyword esp32 in the search box, select 3.0.4, and install it.
![alt text](<./img/ESP32S3 board version-1.png>)

4. Select board and port.

    Navigate to Tools > Board > esp32, and select " XIAO_ESP32S3 "; then Tools > Port.
![alt text](<./img/select board.png>)

5. Install library: Arduinojson and UrlEncode.
     Navigate to Tools > Manager Libraries..., type the keyword "Arduinojson" in the search box, select 7.1.0, and install it. For the "UrlEncode", the same process.
![alt text](./img/image-20.png)
![alt text](./img/image-21.png)

## Obtain and Configure API Keys
### Baidu cloud voice technology: STT and TTS

Step1: Login Baidu cloud. Link: https://cloud.baidu.com/. If you don't have an account, please register one.

    Navigate to Product > Voice technology > Standard Edition of Short Voice Recognition,
![alt text](./img/image-1.png)
![alt text](./img/image-2.png)

Step2: Create application, fill the Application name, choose all items of Voice technology.
![alt text](./img/image-3.png)
![alt text](./img/image-4.png)
![alt text](./img/image-5.png)

Step3: After application creatation, check the details of the application. In this step, we get the AppID, API Key and Secret Key. These 3 items will be used in code to get access_token.
![alt text](./img/image-9.png)

Step4: This step is not mandatory, but strongly recommended. Technical documentation and reference code are very useful.

Choose "API online debugging", then click "Debug", you can get the access_token.
![alt text](./img/image-7.png)
![alt text](./img/image-8.png)

Choose "STT" and "TTS" to read the technical documentation and reference code.

In the program, for STT fucntion, POST request is used to upload recorded voice in JSON format.
![alt text](./img/image-12.png)

For TTS function, GET request is used to get the audio data(in MP3 format).
![alt text](./img/image-11.png)


### Alibaba LLM: TONGYIQIANWEN
Step1: Login Aliyun DashScope:https://dashscope.aliyun.com/. Choose "Control Center".
![alt text](./img/image-13.png)

Step2: Activate DashScope service
![alt text](./img/image-15.png)
![alt text](./img/image-16.png)

Step3: Navigate to Management Center > API-KEY management, click "Create new API-KEY", then copy and save the API-KEY.
![alt text](./img/image-17.png)
![alt text](./img/image-18.png)

Now the API Keys are all configured. You can change the platform freely, such as implemented with Doubao's STT and TTS + Baidu LLM: Wenxinyiyan or all of them are done with Aliyun. The processes and codes are quite similar. Additionally, the platforms also provide excellent technical documentation. Try them yourself.


## Modify code based on your Wifi settings and API keys

### 1. Wifi ssid and password
Fill in your own Wi-Fi account and password within the quotation marks.

```
const char *ssid = "Your Wifi account name";
const char *password = "Your Wifi password";
```
### 2. Baidu Cloud AppID, API Key and Secret Key
Fill in your Baidu Cloud AppID, API Key and Secret Key.
```
const char *CUID = "Your Baidu Cloud AppID";
const char *CLIENT_ID = "Your Baidu Cloud API Key";
const char *CLIENT_SECRET = "Your Baidu Cloud Secret Key";
```
### 3. Aliyun API-KEY
Fill in your Aliyun API-KEY.

```
const char* apikey = "Your Aliyun API-KEY";
```
### 4. Baidu Cloud voice technolgy TTS and STT URL address
Fill in your Baidu Cloud AppID in TTS URL address.

```
url = baidutts_url;
url += "?tex=" + encodedText;
url += "&lan=zh";
url += "&cuid=Your Baidu Cloud AppID";
url += "&ctp=1";
```
Fill in your Baidu Cloud AppID in STT URL address.
```
strcat(data_json,"\"channel\":1,");
strcat(data_json,"\"cuid\":\"Your Baidu Cloud AppID\",");
strcat(data_json, "\"token\":\"");
```
### 5. Other settings
Please compare carefully and choose the right one. Especially note that item PSRAM: <span style="color: red;"> **OPI PSRAM** </span>
![alt text](./img/86a2c49d71fa28682652cdebf8c341d.png)


Navigator to Tools > Serial Monitor, then choose "115200 baud" to Check the code execution result.
![alt text](./img/image-25.png)

