/*This code is for ESP32 - AI_THINKER board for any other board change the  camera pins
Application -- it get camera feed(image format - RGB565) from esp cam and sends it to face detection alog done from esp_dl lib,
if face is detected it will convert to JPG format and save it to sd card, in case of any uart input signal it received will 
send saved image from sd card to uart
major library used - Esp_camera , esp_dl
*/

#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/*for camera stuff like setting camera pins ,configuration,taking pic and converting pic format etc*/
#include "esp_camera.h"

/*for image processing and face detetion in image*/
#include "dl_image.hpp"
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"

/*For sd card setup and usage*/
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

/*uart setup and usage*/
#include "driver/uart.h"


//camera pins for AI_THINKER
#define CAMERA_PIN_PWDN 32
#define CAMERA_PIN_RESET -1
#define CAMERA_PIN_XCLK 0
#define CAMERA_PIN_SIOD 26
#define CAMERA_PIN_SIOC 27
#define CAMERA_PIN_D7 35
#define CAMERA_PIN_D6 34
#define CAMERA_PIN_D5 39
#define CAMERA_PIN_D4 36
#define CAMERA_PIN_D3 21
#define CAMERA_PIN_D2 19
#define CAMERA_PIN_D1 18
#define CAMERA_PIN_D0 5
#define CAMERA_PIN_VSYNC 25
#define CAMERA_PIN_HREF 23
#define CAMERA_PIN_PCLK 22

//Uart pins 
#define TX_PIN 1
#define RX_PIN 3
//Hardware flow control disabled so no rtc adn cts pins
#define RTS_PIN (UART_PIN_NO_CHANGE)
#define CTS_PIN (UART_PIN_NO_CHANGE)
#define UART_PORT_NUM      UART_NUM_0
#define UART_BAUD_RATE     115200
#define TASK_STACK_SIZE    2048

#define BUF_SIZE (1024)

//for logging Tag
static const char *TAG = "sd_testing";

/*below two are buffer and buffer lenght variables for image converting form RGB565 to jpeg*/
size_t jpg_buf_len = 0;
uint8_t *jpg_buf = NULL;

//face detection constants with default values
HumanFaceDetectMSR01 detector(0.3F, 0.3F, 10, 0.3F);
HumanFaceDetectMNP01 detector2(0.4F, 0.3F, 10);

/*for setting delay constant in milliseconds*/
#define portTICK_RATE_MS portTICK_PERIOD_MS


//function for setting up cameras and it's pins
void register_camera();
//funciton for setting up sd card
static esp_err_t init_sd_card(void);
//uart setup
void uart_Setup(void);
//function for face detection
void face_detection();
//saving image to sdcard
void save_in_sd(camera_fb_t *frame);
//function for sending image in uart 
void send_it_to_uart();


extern "C" void app_main(void)
{
    //starting camera
    register_camera();
    printf("cam started with out error");
    //starting sd card
    init_sd_card();
    printf("sd card initiated without error");
    //setting up uart
    uart_Setup();

    while(1){
        //this function gets the pic from camera and check for face if yes will get to save_in_sd function
        face_detection();

        //length of input signal set as 0
        int signal = 0;
        //buffer for input signal
        char *input = (char *) malloc(BUF_SIZE);
        //reading input from uart
        signal = uart_read_bytes(UART_PORT_NUM, input, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        //if input length is more we will send imgae from sd card to uart
        if(signal != 0){
        //free the input pointer
        free(input);
        input = NULL;
        //sends image from uart to sd card
        send_it_to_uart();}
    }
}

void register_camera(){
    //configuring all the pins and mode
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAMERA_PIN_D0;
    config.pin_d1 = CAMERA_PIN_D1;
    config.pin_d2 = CAMERA_PIN_D2;
    config.pin_d3 = CAMERA_PIN_D3;
    config.pin_d4 = CAMERA_PIN_D4;
    config.pin_d5 = CAMERA_PIN_D5;
    config.pin_d6 = CAMERA_PIN_D6;
    config.pin_d7 = CAMERA_PIN_D7;
    config.pin_xclk = CAMERA_PIN_XCLK;
    config.pin_pclk = CAMERA_PIN_PCLK;
    config.pin_vsync = CAMERA_PIN_VSYNC;
    config.pin_href = CAMERA_PIN_HREF;
    config.pin_sscb_sda = CAMERA_PIN_SIOD;
    config.pin_sscb_scl = CAMERA_PIN_SIOC;
    config.pin_pwdn = CAMERA_PIN_PWDN;
    config.pin_reset = CAMERA_PIN_RESET;
    config.xclk_freq_hz = 20000000;
    //used PIX format RGB565 as this format required for face detection
    config.pixel_format = PIXFORMAT_RGB565;
    /* from esp32 cam lib --> QVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. 
    The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.*/
    //but for experimentation can try to increase quality
    //highest quality i can go without resetting and buffer error is VGA - 640*480
    config.frame_size = FRAMESIZE_VGA;
    //0-63, for OV series camera sensors, lower number means higher quality
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    //initiating esp camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {   //log in case of error
        ESP_LOGE(TAG, "Camera init failed with error");
    
    }
}

static esp_err_t init_sd_card(void){

    // configuring default sd card setup
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
    };

    sdmmc_card_t *card;
    //sd card setup
    esp_err_t err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK)
    {
        return err;
    }
    return ESP_OK;
}

void uart_Setup(void){
    //uart config with software flow control
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

    //Install UART driver and set the UART to the default configuration.
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    //Set UART configuration parameters.
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    //Assign signals of a UART peripheral to GPIO pins
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TX_PIN, RX_PIN, RTS_PIN, CTS_PIN));
}

void face_detection(void){

        //getting camera buffer from get function and having in frame pointer
        camera_fb_t *frame = esp_camera_fb_get();
        
        //detects the face from the frame buffer
        std::list<dl::detect::result_t> &detect_candidates = detector.infer((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3});
        std::list<dl::detect::result_t> &detect_results = detector2.infer((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3}, detect_candidates);
       
       //if more than one face is deteced in the image will be saved in sd card
        if (detect_results.size() > 0){   
        save_in_sd(frame);}
        //return the pointer to free the memory
        esp_camera_fb_return(frame);
    }

void save_in_sd(camera_fb_t *frame){

    //changing pixel format from RGB565 to jpg
    //image buffer is now stored in jpg_buf
    frame2jpg(frame , 80, &jpg_buf ,&jpg_buf_len);

    char name[20];
    sprintf(name , "/sdcard/pic.jpg");
    //opening the file
    FILE *file = fopen(name , "w");

    if (file ==NULL){
    //in case of pointer is null will log the error    
    ESP_LOGI(TAG, "jpg file failed to open in sdcard"); 
    //free the image buffer
    free(jpg_buf);
    jpg_buf = NULL;}
    else{
    //writes the image buffer in sd card
    fwrite(jpg_buf, 1, jpg_buf_len, file);
    //close the file , and free the image buffer
    fclose(file);
    free(jpg_buf);
    jpg_buf = NULL;
    //logging this stage
    ESP_LOGE(TAG, "pic is saved in sdcard");}}

void send_it_to_uart(){
    
    char name[20];
    int len = BUF_SIZE;
    sprintf(name , "/sdcard/pic.jpg");
    //read in binary format
    FILE *file = fopen(name, "rb");

    if (file == NULL) {
        //in case of null return
        fclose(file);
        return;}

    //check for file length
    fseek(file,0,SEEK_END);
    long long int size = ftell(file);
    fseek(file,0,SEEK_SET);

    //1024 sized pointer to send uart data in loop
    uint8_t* file_data = (uint8_t*)malloc(BUF_SIZE);
    if (file_data == NULL) {
        // in case of malloc failure return
        fclose(file);
        return;}
    
    //this loop will send buf_size data each time in uart and wait for it to be done 
    for (int i =0; i <= size;i+=BUF_SIZE){
    
    //this "if statement" is for case when data need to send is less than buf_size
    if((size-i ) < BUF_SIZE){ len = (size -i);}

    //reads the file 
    fread(file_data, 1, len , file);
    //sends it in uart 
    uart_write_bytes(UART_PORT_NUM,(const uint8_t *)file_data ,len);
    //wait for uart to finish
    uart_wait_tx_done(UART_PORT_NUM, 100);

    }    fclose(file);}