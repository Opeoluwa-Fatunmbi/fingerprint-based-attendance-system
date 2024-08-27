#include <Adafruit_Fingerprint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <SD.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>
#include <Keypad.h>

// Define the pins for the TFT display and SD card
#define TFT_CS     10
#define TFT_RST    8
#define TFT_DC     9
#define TFT_BACKLIGHT 11
#define SD_CS      53   // Chip select pin for the SD card

// Define the pins for the RTC module
#define DS1302_SCLK_PIN 7  // CLK
#define DS1302_IO_PIN   6  // DAT
#define DS1302_CE_PIN   5  // RST

// Initialize the ST7789 TFT display
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Initialize the fingerprint sensor
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);

// Initialize the RTC module
ThreeWire myWire(DS1302_IO_PIN, DS1302_CE_PIN, DS1302_SCLK_PIN); 
RtcDS1302<ThreeWire> Rtc(myWire);

// Variable to keep track of the fingerprint ID
int fingerprintID = 1;

// Define the keypad with your pin layout
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Your specified pin connections
byte rowPins[ROWS] = {26, 27, 28, 29}; // Connect to the row pinouts of the keypad
byte colPins[COLS] = {22, 23, 24, 25}; // Connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String studentName = "";
String matricNumber = "";

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial1.begin(57600);

  // Initialize the TFT display
  pinMode(TFT_BACKLIGHT, OUTPUT);
  analogWrite(TFT_BACKLIGHT, 255); // Set to maximum brightness (255)
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

  // Initialize the RTC
  Rtc.Begin();

  // Display the welcome message with time and date
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
  studentName = enterText("Enter Name:");
  matricNumber = enterText("Enter Matric No:");

  displayMessage("Place your finger on the sensor...");
  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz();
  finger.createModel();
  finger.storeModel(fingerprintID);
  saveStudentData(fingerprintID, studentName, matricNumber);
  fingerprintID++;
}

void markAttendance() {
  displayMessage("Place your finger on the sensor...");
  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz();
  if (finger.fingerSearch() == FINGERPRINT_OK) {
    int id = finger.fingerID;
    String name = getStudentNameById(id);
    if (name != "") {
      displayMessage("Welcome, " + name);
      logAttendance(name);
    } else {
      displayMessage("ID not found");
    }
  } else {
    displayMessage("Fingerprint not matched");
  }
}

void saveStudentData(int id, String name, String matric) {
  File dataFile = SD.open("students.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.print("ID: ");
    dataFile.println(id);
    dataFile.print("Name: ");
    dataFile.println(name);
    dataFile.print("Matric: ");
    dataFile.println(matric);
    dataFile.close();
    displayMessage("Data saved to SD");
  } else {
    displayMessage("Error opening file for writing");
  }
}

String getStudentNameById(int id) {
  File dataFile = SD.open("students.txt");
  if (dataFile) {
    while (dataFile.available()) {
      String line = dataFile.readStringUntil('\n');
      if (line.startsWith("ID: " + String(id))) {
        line = dataFile.readStringUntil('\n');
        line = line.substring(6); // Skip "Name: "
        dataFile.close();
        return line;
      }
    }
    dataFile.close();
  }
  return "";
}

void logAttendance(String name) {
  RtcDateTime now = Rtc.GetDateTime();
  File dataFile = SD.open("attendance.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.print("Name: ");
    dataFile.print(name);
    dataFile.print(" - Date: ");
    dataFile.print(now.Month());
    dataFile.print("/");
    dataFile.print(now.Day());
    dataFile.print("/");
    dataFile.print(now.Year());
    dataFile.print(" - Time: ");
    dataFile.print(now.Hour());
    dataFile.print(":");
    dataFile.println(now.Minute());
    dataFile.close();
    displayMessage("Attendance logged");
  } else {
    displayMessage("Error opening file for writing");
  }
}

void displayWelcomeMessage() {
  RtcDateTime now = Rtc.GetDateTime();
  String date = String(now.Day()) + "/" + String(now.Month()) + "/" + String(now.Year());
  String time = String(now.Hour()) + ":" + (now.Minute() < 10 ? "0" : "") + String(now.Minute());
  
  tft.fillScreen(ST77XX_BLACK);  // Clear the screen with black color
  tft.setCursor(10, 10); // Set cursor position
  tft.println("Welcome!");
  tft.println("Press A to register,");
  tft.println("Press B to mark");
  tft.println("attendance.");
  tft.println();
  tft.println(date + " " + time); // Display date and time
}

void displayMessage(String message) {
  tft.fillScreen(ST77XX_BLACK);  // Clear the screen with black color
  tft.setCursor(10, 10); // Set cursor position
  tft.print(message); // Print message to the display
}

String enterText(String prompt) {
  displayMessage(prompt);
  String input = "";
  char lastKey = 0;
  unsigned long lastPressTime = 0;
  int keyPressCount = 0;
  const int timeout = 1000;  // 1 second timeout for keypress

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') {
        break;
      } else if (key == 'D') {
        input += ' ';
      } else if (key == '*') {
        if (input.length() > 0) {
          input.remove(input.length() - 1);
        }
      } else {
        if (key == lastKey && millis() - lastPressTime < timeout) {
          keyPressCount++;
        } else {
          keyPressCount = 1;
        }
        
        lastPressTime = millis();
        lastKey = key;

        char letter = keyToLetter(key, keyPressCount);
        if (keyPressCount == 1 || keyPressCount == 0) {
          input += letter;
        } else {
          input[input.length() - 1] = letter;
        }
      }

      displayMessage(prompt + "\n" + input);
    }
  }
  
  return input;
}

char keyToLetter(char key, int pressCount) {
  switch (key) {
    case '2':
      if (pressCount % 4 == 1) return 'A';
      if (pressCount % 4 == 2) return 'B';
      if (pressCount % 4 == 3) return 'C';
      return '2';
    case '3':
      if (pressCount % 4 == 1) return 'D';
      if (pressCount % 4 == 2) return 'E';
      if (pressCount % 4 == 3) return 'F';
      return '3';
    case '4':
      if (pressCount % 4 == 1) return 'G';
      if (pressCount % 4 == 2) return 'H';
      if (pressCount % 4 == 3) return 'I';
      return '4';
    case '5':
      if (pressCount % 4 == 1) return 'J';
      if (pressCount % 4 == 2) return 'K';
      if (pressCount % 4 == 3) return 'L';
      return '5';
    case '6':
      if (pressCount % 4 == 1) return 'M';
      if (pressCount % 4 == 2) return 'N';
      if (pressCount % 4 == 3) return 'O';
      return '6';
    case '7':
      if (pressCount % 5 == 1) return 'P';
      if (pressCount % 5 == 2) return 'Q';
      if (pressCount % 5 == 3) return 'R';
      if (pressCount % 5 == 4) return 'S';
      return '7';
    case '8':
      if (pressCount % 4 == 1) return 'T';
      if (pressCount % 4 == 2) return 'U';
      if (pressCount % 4 == 3) return 'V';
      return '8';
    case '9':
      if (pressCount % 5 == 1) return 'W';
      if (pressCount % 5 == 2) return 'X';
      if (pressCount % 5 == 3) return 'Y';
      if (pressCount % 5 == 4) return 'Z';
      return '9';
    case '0':
      if (pressCount % 2 == 1) return ' ';
      return '0';
    default:
      return key;
  }
}
