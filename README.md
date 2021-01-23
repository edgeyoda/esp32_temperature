# esp32_temperature
This is an Edge Impulse Anomaly Detection demo using temperature sensing (and the ESP32). To re-create this proof-of-concept demo you will need to have the following:

- ESP32 DevKitC board
- Adafruit MAX31855 Thermocouple board
- Adafruit SSD1331 OLED board (optional, but useful for visual debugging)
- K-Type Industrial Thermocouple probe
- Arduino IDE
- Edge Impulse account

This example can be easily ported to other embedded targets with the Arduino IDE and Edge Impulse SDK.

Steps to follow for trying out this example:

1. Create an Edge Impulse account at https://www.edgeimpulse.com
2. Clone the public project at https://studio.edgeimpulse.com/studio/16389/
3. Generate an Arduino .zip on the Deployment page (by clicking 'Build')
4. After downloading and opening the .ino file, go to 'Sketch' -> 'Include Library' -> 'Add .ZIP Library' and point it to the Arduino .zip file from step 3.
5. Compile and Upload the Sketch
