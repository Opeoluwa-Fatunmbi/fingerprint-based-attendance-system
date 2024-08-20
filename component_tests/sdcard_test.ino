#include <SPI.h>
#include <SD.h>

#define SD_CS 53   // Chip select pin for the SD card

void setup() {
  Serial.begin(9600);

  // Try to initialize the SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized successfully.");

  // Open a file and write some data
  File dataFile = SD.open("test.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Testing SD card write.");
    dataFile.close();
    Serial.println("Data written to SD card.");
  } else {
    Serial.println("Error opening file for writing.");
  }
}

void loop() {
  // Nothing needed in loop
}
