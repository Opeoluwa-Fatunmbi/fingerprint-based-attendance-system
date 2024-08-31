#include <Adafruit_Fingerprint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <SD.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>
#include <Keypad.h>
#include "UUID.h"

// Define the pins for the TFT display and SD card
#define TFT_CS     10
#define TFT_RST    8
#define TFT_DC     9
#define TFT_BACKLIGHT 11
#define SD_CS      53

// Define the pins for the RTC module
#define DS1302_SCLK_PIN 7
#define DS1302_IO_PIN   6
#define DS1302_CE_PIN   5

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);
ThreeWire myWire(DS1302_IO_PIN, DS1302_CE_PIN, DS1302_SCLK_PIN); 
RtcDS1302<ThreeWire> Rtc(myWire);

const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {26, 27, 28, 29}; 
byte colPins[COLS] = {22, 23, 24, 25}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String studentName = "";
String matricNumber = "";
const String MATRIC_PREFIX = "CPE/";

void setup() {
  Serial.begin(9600);
  Serial1.begin(57600);

  pinMode(TFT_BACKLIGHT, OUTPUT);
  analogWrite(TFT_BACKLIGHT, 255);
  tft.init(135, 240);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  digitalWrite(TFT_CS, HIGH);

  if (!SD.begin(SD_CS)) {
    displayMessage("SD card init failed, stopping.");
    while (1);
  }
  displayMessage("SD card initialized");

  digitalWrite(TFT_CS, LOW);

  if (finger.verifyPassword()) {
    displayMessage("Found fingerprint sensor!");
  } else {
    displayMessage("Did not find fingerprint sensor :(");
    while (1);
  }

  Rtc.Begin();

  if (!Rtc.IsDateTimeValid()) {
    Rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__)); 
  }

  if (!Rtc.GetIsRunning()) {
    Rtc.SetIsRunning(true);
  }

  displayWelcomeMessage();
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    if (key == 'A') {
      registerStudent();
    } else if (key == 'B') {
      markAttendance();
    } else if (key == 'C') {
      displayWelcomeMessage();
    }
  }
}

void registerStudent() {
  matricNumber = MATRIC_PREFIX + enterMatricNumber("Enter matric number (e.g., XX/YYYY):");

  displayMessage("Place your finger on the sensor...");
  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz(1);
  displayMessage("Remove finger");

  delay(2000);

  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz(2);

  if (finger.createModel() == FINGERPRINT_OK) {
    int id = generateUniqueID(); 
    if (finger.storeModel(id) == FINGERPRINT_OK) {
      displayMessage("Saved! Returning to home...");
      String uuid = generateUUID();  
      saveStudentData(id, uuid, matricNumber);  // Save the fingerprint ID, UUID, and matric number
    } else {
      displayMessage("Error storing fingerprint!");
    }
  } else {
    displayMessage("Error creating model!");
  }
  
  delay(2500);
  displayWelcomeMessage();
}

int generateUniqueID() {
  static int id = 1; 
  return id++;
}

void markAttendance() {
    int attempts = 4;
    bool matched = false;

    for (int i = 0; i < attempts; i++) {
        displayMessage("Place your finger on the sensor...");

        while (finger.getImage() != FINGERPRINT_OK);
        finger.image2Tz();

        if (finger.fingerSearch() == FINGERPRINT_OK) {
            int id = finger.fingerID;
            String matric = getStudentMatricById(id);

            if (matric != "") {
                if (checkIfAlreadyMarked(id)) {
                    displayMessage("You have already marked attendance!");
                } else {
                    RtcDateTime now = Rtc.GetDateTime();
                    String dateTime = String(now.Month()) + "/" + String(now.Day()) + "/" + String(now.Year()) + 
                                      "  " + String(now.Hour()) + ":" + String(now.Minute());
                    String message = "Attendance taken!\n" + matric + "\n\n" + dateTime;
                    displayMessage(message);
                    logAttendance(id, matric);
                }
                matched = true;
                break;
            } else {
                displayMessage("No match found. Try again.");
            }
        } else {
            displayMessage("No match found. Try again.");
        }
    }

    if (!matched) {
        displayMessage("Attendance failed. Please try again.");
    }

    delay(3000);
    displayWelcomeMessage();
}

bool checkIfAlreadyMarked(int id) {
    File dataFile = SD.open("attendan.csv");
    if (dataFile) {
        RtcDateTime now = Rtc.GetDateTime();
        String today = String(now.Month()) + "/" + String(now.Day()) + "/" + String(now.Year());

        while (dataFile.available()) {
            String line = dataFile.readStringUntil('\n');
            if (line.startsWith(String(id) + ",")) {
                int dateStart = line.indexOf(',', line.indexOf(',') + 1) + 1;
                int dateEnd = line.indexOf(',', dateStart);
                String date = line.substring(dateStart, dateEnd);

                if (date == today) {
                    dataFile.close();
                    return true; // Already marked attendance today
                }
            }
        }
        dataFile.close();
    }
    return false; // Attendance not marked today
}


void saveStudentData(int id, String uuid, String matric) {
  File dataFile = SD.open("students.csv", FILE_WRITE);
  if (dataFile) {
    // Check if the file is empty to write headers
    if (dataFile.size() == 0) {
      dataFile.println("ID,UUID,Matric");
    }
    dataFile.print(id);
    dataFile.print(",");
    dataFile.print(uuid);
    dataFile.print(",");
    dataFile.print(matric);
    dataFile.println(); 
    dataFile.close();
    displayMessage("Data saved to SD");
  } else {
    displayMessage("Error opening file for writing");
  }
}

void logAttendance(int id, String matric) {
  RtcDateTime now = Rtc.GetDateTime();
  File dataFile = SD.open("attendan.csv", FILE_WRITE);
  if (dataFile) {
    // Check if the file is empty to write headers
    if (dataFile.size() == 0) {
      dataFile.println("ID,Matric,Date,Time");
    }
    dataFile.print(id);
    dataFile.print(",");
    dataFile.print(matric);
    dataFile.print(",");
    dataFile.print(now.Month());
    dataFile.print("/");
    dataFile.print(now.Day());
    dataFile.print("/");
    dataFile.print(now.Year());
    dataFile.print(",");
    dataFile.print(now.Hour());
    dataFile.print(":");
    dataFile.print(now.Minute());
    dataFile.println(); 
    dataFile.close();
    displayMessage("Attendance logged: " + matric);
  } else {
    displayMessage("Error opening file for writing");
  }
}


String getStudentMatricById(int id) {
  File dataFile = SD.open("students.csv");
  if (dataFile) {
    while (dataFile.available()) {
      String line = dataFile.readStringUntil('\n');
      if (line.startsWith(String(id) + ",")) {
        dataFile.close();
        int matricStart = line.indexOf(',', line.indexOf(',') + 1) + 1;
        int matricEnd = line.indexOf(',', matricStart);
        if (matricEnd == -1) matricEnd = line.length(); // Handle last column
        String matric = line.substring(matricStart, matricEnd);
        return matric;
      }
    }
    dataFile.close();
  }
  return "";
}


String enterMatricNumber(String prompt) {
  String input = "";
  char key;
  displayMessage(prompt);

  while (input.length() < 7) {
    key = keypad.getKey();
    if (key) {
      if (key >= '0' && key <= '9') {
        if (input.length() == 2) input += "/";
        input += key;
      } else if (key == '#') {  // Use # as Enter key
        break;  // Exit the loop when # is pressed
      } else if (key == 'D') {
        if (input.length() > 0) {
          if (input.endsWith("/")) input = input.substring(0, input.length() - 1);
          input = input.substring(0, input.length() - 1);
        }
      }
      displayMessage(prompt + "\n" + "CPE/" + input);
    }
  }
  return input;
}

void displayMessage(String message) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.println(message);
}

void displayWelcomeMessage() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.println("Student Attendance");
  tft.println();
  tft.println("Welcome Opeoluwa!");
  tft.println();
  tft.println("Press A: Register");
  tft.println("Press B: Attendance");
  tft.println("Press C: Home");
}

String generateUUID() {
  String uuid = "";
  for (int i = 0; i < 8; i++) {
    uuid += String(random(0, 16), HEX);
  }
  return uuid;
}


