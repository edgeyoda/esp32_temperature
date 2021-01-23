/*
 *  This sketch shows how to use an Edge Impulse Anomaly detection model 
 *  for temperature sensing (ESP32, MAX31855 thermocouple amplifier, and OLED display)
 *
 */
#include <esp32_temperature_inference.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>

// You can use any (4 or) 5 pins
#define sclk 22  
#define mosi 21  
#define cs   15 
#define rst  4 
#define dc   0 

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

// Bit-bang using gpio to drive OLED display
Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, mosi, sclk, rst);

#define MAXCS   5
Adafruit_MAX31855 thermocouple(MAXCS);

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

// WiFi network name and password:
const char * networkName = "winzin311";
const char * networkPswd = "pepe1150";

//IP address to send UDP data to:
// either use the ip address of the server or 
// a network broadcast address
const char * udpAddress = "192.168.0.255";
const int udpPort = 3333;

//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;

void setup(){
  // Initilize hardware serial:
  Serial.begin(115200);

  display.begin();

  Serial.println(" oled display init");

  delay(2000);
  display.fillScreen(BLACK);
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  //display.println("Hello World!");
  display.println("Welcome to the");
  display.println("Edge Impulse");
  display.println("Temp Sensing");
  display.println("Anomaly");
  display.println("Detection Demo");
  delay(1000);
  
  //Connect to the WiFi network
  connectToWiFi(networkName, networkPswd);

   Serial.println("MAX31855 test");
  // wait for MAX chip to stabilize
  delay(500);
  Serial.print("Initializing sensor...");
  if (!thermocouple.begin()) {
    Serial.println("ERROR.");
    while (1) delay(10);
  }
  Serial.println("DONE.");
}

void connectToWiFi(const char * ssid, const char * pwd){
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  
  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case SYSTEM_EVENT_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
 
          display.fillScreen(BLACK);
          display.setCursor(0, 0);
          display.setTextColor(WHITE);
          display.setTextSize(1);
          display.println("WiFi Connected");
          display.println("IP Address: ");
          display.println(WiFi.localIP());
          display.fillScreen(BLACK);

          
          //initializes the UDP state
          //This initializes the transfer buffer
          udp.begin(WiFi.localIP(),udpPort);
          connected = true;
          break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
      default: break;
    }
}

void loop(){
  //only send data when connected
  if(connected){
    //Send a packet
    udp.beginPacket(udpAddress,udpPort);
    udp.printf("Seconds since boot: %lu", millis()/1000);
    udp.endPacket();
   
    // basic readout test, just print the current temp
    // Serial.print("Internal Temp = ");
    // Serial.println(thermocouple.readInternal());

    double c = thermocouple.readCelsius();
    if (isnan(c)) {
      Serial.println("Something wrong with thermocouple!");
    } 
    else {
    // Serial.print("C = ");
    // Serial.println(c);
    }
    // Serial.print("F = ");
    // Serial.println(thermocouple.readFahrenheit());

    display.fillScreen(BLACK);
    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.println("Temperature");
    display.print(thermocouple.readFahrenheit());
    display.println(" degrees F");
    display.println("");
    display.println("");

    Serial.println(thermocouple.readFahrenheit());
    delay(250);
        
    ei_printf("\nStarting inferencing in 2 seconds...\n");

    ei_printf("Sampling...\n");

    // Allocate a buffer here for the values we'll read from the temp sensor
    float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };

    for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix++)
    {
      buffer[ix] = thermocouple.readFahrenheit();
      delay(200);
    }

    for (size_t i = 0; i < 10; i++)
    {
      ei_printf("%.3f\n", buffer[i]);
    }

    // Turn the raw buffer in a signal which we can the classify
    signal_t signal;
    int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
        ei_printf("Failed to create signal from buffer (%d)\n", err);
        return;
    }

    // Run the classifier
    ei_impulse_result_t result = { 0 };

    err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        return;
    }

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
   
    display.println("Anomaly Score");
    display.println(result.anomaly);

    delay(1000);
#endif
}
  
}



/**
* @brief      Printf function uses vsnprintf and output using Arduino Serial
*
* @param[in]  format     Variable argument list
*/
void ei_printf(const char *format, ...) {
   static char print_buf[1024] = { 0 };

   va_list args;
   va_start(args, format);
   int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
   va_end(args);

   if (r > 0) {
       Serial.write(print_buf);
   }
}
