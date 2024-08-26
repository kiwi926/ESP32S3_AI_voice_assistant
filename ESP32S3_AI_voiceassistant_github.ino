#include <Arduino.h>
#include "base64.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include <ESP_I2S.h>
#include <ArduinoJson.h>
#include <UrlEncode.h>

const uint8_t key = 3;        // Push key
const uint8_t I2S_LRC = 4;    //ESP32 speaker pins setup
const uint8_t I2S_BCLK = 5;
const uint8_t I2S_DOUT = 6;  
const uint8_t I2S_DIN = 41;   //ESP32 MIC pins setup
const uint8_t I2S_SCK = 42; 

// WIFI. Please change to your wifi account name and password
const char *ssid = "Your Wifi account name";
const char *password = "Your Wifi password";

// BAIDUYUN API of STT and TTS
const int DEV_PID = 1537;  // Chinese
const char *CUID = "Your Baidu Cloud AppID";
const char *CLIENT_ID = "Your Baidu Cloud API Key";
const char *CLIENT_SECRET = "Your Baidu Cloud Secret Key";
const char* baidutts_url = "https://tsn.baidu.com/text2audio";
String url = baidutts_url;

// TONGYIQIANWEN API for LLM
const char* apikey = "Your Aliyun API-KEY";
String llm_inputText = "Hello, TYQW!";
String apiUrl = "https://dashscope.aliyuncs.com/api/v1/services/aigc/text-generation/generation";

String token;

I2SClass I2S;

// Audio DAC parameters
i2s_mode_t PDM_RX_mode = I2S_MODE_PDM_RX;
const int sampleRate = 16000;  // sample rate in Hz
i2s_data_bit_width_t bps = I2S_DATA_BIT_WIDTH_16BIT;
i2s_slot_mode_t slot = I2S_SLOT_MODE_MONO;

// MIC recording buffer
static const uint32_t sample_buffer_size = 96000;
static signed short sampleBuffer[sample_buffer_size];

// STT JSON
char *data_json;
const int recordTimesSeconds = 3;
const int adc_data_len = 16000 * recordTimesSeconds;
const int data_json_len = adc_data_len * 2 * 1.4;

static uint8_t start_record = 0;
static uint8_t record_complete = 0;

// HTTP audio file buffer
const int AUDIO_FILE_BUFFER_SIZE  = 600000;
char* audio_file_buffer;

const int Timeout = 10000;
uint32_t audio_index = 0;
HTTPClient http_tts;

// BAIDU TTS
void get_voice_answer(String llm_answer) {
    String encodedText;
    
    encodedText = urlEncode(urlEncode(llm_answer));
    
    const char *headerKeys[] = {"Content-type", "Content-Length"};

    url = baidutts_url;
    url += "?tex=" + encodedText;
    url += "&lan=zh";
    url += "&cuid=Your Baidu Cloud AppID";
    url += "&ctp=1";
    url += "&aue=3";
    url += "&tok=" + token; ; 

    Serial.println("Requesting URL: " + url);

    HTTPClient http_tts;
    http_tts.setTimeout(Timeout);
    http_tts.begin(url);
    http_tts.collectHeaders(headerKeys, 2);

    int httpResponseCode_tts = http_tts.GET();
    
    
    if (httpResponseCode_tts == HTTP_CODE_OK) {
        
        String contentType = http_tts.header("Content-Type");
        Serial.print("\nContent-Type = ");
        Serial.println(contentType);
        
        if (contentType.startsWith("audio")) {
            
            Serial.println("The synthesis is successful, and the result is an audio file.");

            memset(audio_file_buffer, 0, AUDIO_FILE_BUFFER_SIZE);
            
            int32_t wait_cnt = 0;
            
            while (1) {
                size_t data_available = 0;
                char *read_buffer;
                
                data_available = http_tts.getStream().available();
                    
                if (0 != data_available) {
                    read_buffer = (char *)ps_malloc(data_available); 

                    // Check if the buffer will overfolw
                    if((audio_index + data_available) >= AUDIO_FILE_BUFFER_SIZE) {
                        Serial.println("Audio buffer overflow.");
                        free(read_buffer);
                        break;
                    } 

                    // Read the data from the stream
                    http_tts.getStream().readBytes(read_buffer, data_available);

                    // Copy the data to audio file buffer
                    memcpy(audio_file_buffer + audio_index, read_buffer, data_available);
                    audio_index += data_available;

                    free(read_buffer);
                        
                    wait_cnt = 0;
                
                } 
                else {
                    wait_cnt++;
                    
                    delay(10);
                    
                    if (wait_cnt > 200) {
                        wait_cnt = 0;
                        break;  // Break the loop if the wait time exceeds 1s
                    }
                }
            }
            Serial.printf("MP3 file length = %d\n", audio_index);
            
            I2S.playMP3((uint8_t *)audio_file_buffer, audio_index);
        
        } 
        else if (contentType.equals("application/json")) {
            Serial.println("The synthesis encountered an error, and the returned result is a JSON text.");
        } 
        else {
            Serial.println("Unknown Content-Type");
        }

        http_tts.end();

    } 
    else {
        Serial.println("Failed to receive audio file. HTTP response code: ");
        Serial.println(httpResponseCode_tts);
    }
}

void setup_mic_pins() {

    I2S.setPinsPdmRx(I2S_SCK, I2S_DIN);

    if (!I2S.begin(PDM_RX_mode, sampleRate, bps, slot, -1)) {
        Serial.println("Failed to initialize I2S for MIC!");
        while (1); 
    }
}

void setup_speaker_pins() {

    I2S.setPins(I2S_BCLK, I2S_LRC, I2S_DOUT);

    if (!I2S.begin(I2S_MODE_STD, sampleRate, bps, slot, I2S_STD_SLOT_BOTH)) {
        Serial.println("Failed to initialize I2S for speaker!");
        while (1); 
    }
}

void setup(){
    Serial.begin(115200);

    pinMode(key, INPUT_PULLUP);

    setup_mic_pins();

    // WiFi connect
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid,password);
    Serial.print("Connecting to Wifi ..");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }

    Serial.println(WiFi.localIP());
    Serial.printf("\r\n-- wifi connect success! --\r\n");

    token = get_token();
    Serial.println("Press the button to talk to me.");

    // Allocate memory for the MIC data JSON buffer
    data_json = (char *)ps_malloc(data_json_len * sizeof(char));  
    if (!data_json) {
        Serial.println("Failed to allocate memory for data_json");
    }

    // Allocate memory for the audio file buffer
    audio_file_buffer = (char *)ps_malloc(AUDIO_FILE_BUFFER_SIZE * sizeof(char));

    // Create a new task for MIC data reading
    xTaskCreate(capture_samples, "CaptureSamples", 1024 * 32, (void*)sample_buffer_size, 10, NULL);

}

void loop() {
    String LLM_answer;

    if (digitalRead(key) == 0) {
        delay(30);
        if (digitalRead(key) == 0) {
            Serial.printf("Start recording\r\n");

            start_record = 1;
            while(record_complete != 1) {
                delay(10);
            }
            record_complete = 0;
            start_record = 0;

            memset(data_json, '\0', data_json_len * sizeof(char));
            strcat(data_json,"{");
            strcat(data_json,"\"format\":\"pcm\",");
            strcat(data_json,"\"rate\":16000,");
            strcat(data_json,"\"dev_pid\":1537,");
            strcat(data_json,"\"channel\":1,");
            strcat(data_json,"\"cuid\":\"Your Baidu Cloud AppID\",");
            strcat(data_json, "\"token\":\"");
            strcat(data_json, token.c_str());
            strcat(data_json,"\",");
            sprintf(data_json + strlen(data_json), "\"len\":%d,", adc_data_len * 2);
            strcat(data_json, "\"speech\":\"");
            strcat(data_json, base64::encode((uint8_t *)sampleBuffer, adc_data_len * sizeof(uint16_t)).c_str());
            strcat(data_json, "\"");
            strcat(data_json, "}");

            String baidu_response = send_to_stt();
            Serial.println("Recognition complete");

            DynamicJsonDocument baidu_jsondoc(1024);
            deserializeJson(baidu_jsondoc, baidu_response);
            String talk_question = baidu_jsondoc["result"][0];
            Serial.println("Input: " + talk_question);

            LLM_answer = get_GPT_answer(talk_question);
            Serial.println("\nAnswer: " + LLM_answer);

            //Change I2S pins setup from MIC to speaker
            I2S.end(); 
            setup_speaker_pins();
            get_voice_answer(LLM_answer);

            I2S.end();
            setup_mic_pins();
            Serial.println("\nPress the button to talk to me.");

        }
    }
    delay(1);
}


static void capture_samples(void* arg) {

    size_t bytes_read = 0;

    while (1) {
        if (start_record == 1) {
            I2S.readBytes((char*)sampleBuffer, sample_buffer_size);
            record_complete = 1;
            start_record = 0;
        }
        else {
            delay(100);
        }
    }
    vTaskDelete(NULL);
}

//Get BAIDU token
String get_token() {
    HTTPClient http_baidu;

    String url = String("https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=") + CLIENT_ID + "&client_secret=" + CLIENT_SECRET;
    Serial.println(url);

    http_baidu.begin(url);

    int httpCode = http_baidu.GET();

    if (httpCode > 0) {
        String payload = http_baidu.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        token = doc["access_token"].as<String>();
        Serial.println(token);
    } 
    else {
        Serial.println("Error on HTTP request for token");
    }
    http_baidu.end();

    return token;
}

String send_to_stt() {
    HTTPClient http_stt;

    http_stt.begin("http://vop.baidu.com/server_api");
    http_stt.addHeader("Content-Type", "application/json");

    int httpResponseCode = http_stt.POST(data_json);

    if (httpResponseCode > 0) {
        if (httpResponseCode == HTTP_CODE_OK) {
        String payload = http_stt.getString();
        Serial.println(payload);
        return payload;
        }
    } 
    else {
        Serial.printf("[HTTP] POST failed, error: %s\n", http_stt.errorToString(httpResponseCode).c_str());
    }
    http_stt.end();
}

//TONGYIQIANWEN LLM
String get_GPT_answer(String llm_inputText) {
    HTTPClient http_llm;

    http_llm.setTimeout(Timeout);
    http_llm.begin(apiUrl);

    http_llm.addHeader("Content-Type", "application/json");
    http_llm.addHeader("Authorization", String(apikey));

    String payload_LLM = "{\"model\":\"qwen-turbo\",\"input\":{\"messages\":[{\"role\": \"system\",\"content\": \"要求下面的回答严格控制在256字符以内\"},{\"role\": \"user\",\"content\": \"" + llm_inputText + "\"}]}}";
    
    int httpResponseCode = http_llm.POST(payload_LLM);

    if (httpResponseCode == 200) {
        String response = http_llm.getString();
        http_llm.end();
        Serial.println(response);

        // Parse JSON response
        DynamicJsonDocument jsonDoc(1024);
        deserializeJson(jsonDoc, response);
        String outputText = jsonDoc["output"]["text"];

        return outputText;
    
    } 
    else {
        http_llm.end();
        Serial.printf("Error %i \n", httpResponseCode);
        return "<error>";
    }
}

