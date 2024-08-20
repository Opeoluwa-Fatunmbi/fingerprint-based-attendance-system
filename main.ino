#include <Adafruit_Fingerprint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <SD.h>

// Define the pins for the TFT display and SD card
#define TFT_CS     10
#define TFT_RST    8
#define TFT_DC     9
#define TFT_BACKLIGHT 11
#define SD_CS      53   // Chip select pin for the SD card

// Initialize the ST7789 TFT display
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Create an instance of the Adafruit_Fingerprint class
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);

// Variable to keep track of the fingerprint ID
int fingerprintID = 1;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial1.begin(57600);

  // Initialize the TFT display
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);  // Turn on backlight
  tft.init(135, 240);    // Initialize the display with 135x240 resolution
  tft.setRotation(3);    // Adjust rotation to correct orientation
  tft.fillScreen(ST77XX_BLACK);  // Clear the screen with black color
  tft.setTextColor(ST77XX_WHITE); // Set text color to white
  tft.setTextSize(2);   // Set text size

  // Disable TFT display before initializing SD card
  digitalWrite(TFT_CS, HIGH);

  // Initialize the SD card
  if (!SD.begin(SD_CS)) {
    displayMessage("SD card init failed, stopping.");
    while (1);  // Stop the program if the SD card isn't found
  }

  displayMessage("SD card initialized");

  // Re-enable TFT display after initializing SD card
  digitalWrite(TFT_CS, LOW);

  // Initialize the fingerprint sensor
  if (finger.verifyPassword()) {
    displayMessage("Found fingerprint sensor!");
  } else {
    displayMessage("Did not find fingerprint sensor :(");
    while (1); // Stop the program if the sensor is not found
  }

  displayMessage("Place your finger on the sensor...");
}

void loop() {
  uint8_t p = finger.getImage(); // Capture image of the fingerprint

  if (p == FINGERPRINT_OK) {
    displayMessage("Fingerprint image taken");
    p = finger.image2Tz(); // Convert image to template
    if (p == FINGERPRINT_OK) {
      displayMessage("Fingerprint template created");
      p = finger.fingerSearch(); // Search for a matching fingerprint
      if (p == FINGERPRINT_OK) {
        displayMessage("Fingerprint matched!");
      } else if (p == FINGERPRINT_NOTFOUND) {
        displayMessage("Fingerprint not matched");

        // Save the new fingerprint with an incremental ID
        saveFingerprint(fingerprintID);
        fingerprintID++;  // Increment the fingerprint ID for the next fingerprint
      } else {
        displayMessage("Error searching for fingerprint");
      }
    } else {
      displayMessage("Error converting fingerprint image");
    }
  } else if (p == FINGERPRINT_NOFINGER) {
    displayMessage("No finger detected");
  } else {
    displayMessage("Error capturing fingerprint");
  }

  delay(2000); // Wait 2 seconds before the next read
}

void saveFingerprint(int id) {
    // No need to reinitialize the SD card here
    File dataFile = SD.open("prints.txt", FILE_WRITE);
    if (dataFile) {
        dataFile.print("ID: ");
        dataFile.println(id);
        dataFile.println("Fingerprint data saved.");
        dataFile.close();
        displayMessage("Fingerprint saved to SD");
        Serial.println("Fingerprint saved to SD");
    } else {
        Serial.println("Failed to open file for writing. Possible reasons:");
        Serial.println("- File name too long or invalid?");
        Serial.println("- SD card locked?");
        Serial.println("- Corrupted SD card?");
        Serial.println("- Check wiring and power?");
        displayMessage("Error opening file for writing");
    }
}

void displayMessage(String message) {
  tft.fillScreen(ST77XX_BLACK);  // Clear the screen with black color
  tft.setCursor(10, 10); // Set cursor position
  tft.print(message); // Print message to the display
}
