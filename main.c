#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {13, 12, 11, 10};
byte colPins[COLS] = {9, 7, 6, 5};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int powPin = 8;
const int soilPin = A0;
const int lightPin = A1;

const int minValue = 514;
const int maxValue = 894;

int numPlants = 0;
bool inputReceived = false;
int currentPlant = 1;

float soilMoistureValues[10]; // Adjust size as needed
float lightIntensityValues[10]; // Adjust size as needed

unsigned long lastKeyPressTime = 0;
const unsigned long debounceDelay = 200; // Debounce delay
unsigned long lastUpdateTime = 0;  // Last time the LCD was updated
const unsigned long updateInterval = 1000; // Update interval (1 second)

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  pinMode(powPin, OUTPUT);
  digitalWrite(powPin, LOW); // Ensure sensor is initially off

  DisplayWelcome();
  RequestNumPlants();
}

void loop() {
  if (!inputReceived) {
    char key = getKeyWithDebounce();
    if (key != NO_KEY) {
      if (key >= '0' && key <= '9') {
        numPlants = numPlants * 10 + (key - '0');
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("No. of plants: ");
        lcd.print(numPlants);
      } else if (key == '#') {
        inputReceived = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Plant 1:");
        delay(500); // Delay to stabilize display
      }
    }
  } else {
    unsigned long currentTime = millis();
    
    if (currentTime - lastUpdateTime >= updateInterval) {
      // Update LCD with new values
      float soilMoisture = readSoil();
      float lightIntensity = readLight();

      
      soilMoistureValues[currentPlant - 1] = soilMoisture;
      lightIntensityValues[currentPlant - 1] = lightIntensity;

      // Display values
      lcd.clear();
      lcd.setCursor(0, 0);
      if (currentPlant == numPlants) {
        lcd.print("Last:");
      } else {
        lcd.print("Plant ");
        lcd.print(currentPlant);
        lcd.print(":");
      }
      lcd.setCursor(0, 1);
      lcd.print("D:");
      lcd.print(int(soilMoisture));
      lcd.print("% L:");
      lcd.print(int(lightIntensity));
      lcd.print("%");

      // Update last update time
      lastUpdateTime = currentTime;
    }

    char key = getKeyWithDebounce();
    if (key == '#') {
      currentPlant++;
      if (currentPlant <= numPlants) {
        lcd.clear();
        lcd.setCursor(0, 0);
        if (currentPlant == numPlants) {
          lcd.print("Last:");
        } else {
          lcd.print("Plant ");
          lcd.print(currentPlant);
          lcd.print(":");
        }
        delay(500); // Adjust delay as needed
      } else {
        DisplayResults();
        inputReceived = false; // Reset input flag to allow restarting
      }
    }
  }

  delay(100); // Adjust delay as needed
}

void DisplayWelcome() {
  lcd.setCursor(0, 0);
  lcd.print("Hydration Hero");
  lcd.setCursor(0, 1);
  lcd.print("At your service!");
  delay(2000);
  lcd.clear();
}

void RequestNumPlants() {
  lcd.setCursor(0, 0);
  lcd.print("No. of plants?");
}

float readSoil() {
  digitalWrite(powPin, HIGH);
  delay(10);
  int soil_val = analogRead(soilPin);
  delay(10);
  digitalWrite(powPin, LOW);

  // Calculate percentage based on min and max values
  float percentage = map(soil_val, minValue, maxValue, 100, 0); // Inverted mapping
  percentage = constrain(percentage, 0, 100); // Ensure percentage is within 0-100
  
  return percentage;
}

float readLight() {
  int lightValue = analogRead(lightPin);
  float voltagePercent = (float(lightValue) / 1023.0) * 100.0;
  return voltagePercent;
}

void DisplayResults() {
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Done!");
  delay(500);
  lcd.setCursor(0, 1);
  lcd.print("Results:");
  delay(1000);

  // Determine which plants need water and which need light
  bool needWater[10]; // Assuming up to 10 plants
  bool needLight[10]; // Assuming up to 10 plants
  int waterCount = 0;
  int lightCount = 0;

  for (int i = 0; i < numPlants; i++) {
    if (soilMoistureValues[i] < 40.0) {
      needWater[i] = true;
      waterCount++;
    } else {
      needWater[i] = false;
    }

    if (lightIntensityValues[i] < 30.0) {
      needLight[i] = true;
      lightCount++;
    } else {
      needLight[i] = false;
    }
  }

  // Display plants needing water
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water:");
  bool firstWaterPlant = true;
  int waterLineLength = 6; // Initial length of "Water:"

  if (waterCount == 0) {
    lcd.print(" NA");
  } else {
    for (int i = 0; i < numPlants; i++) {
      if (needWater[i]) {
        String plantStr = " P" + String(i + 1);
        if (waterLineLength + plantStr.length() > 16) {
          lcd.setCursor(0, 1);
          waterLineLength = 0; // Reset line length counter
        }
        lcd.print(plantStr);
        waterLineLength += plantStr.length();
      }
    }
  }

  delay(2000); // Delay to show water results

  // Clear screen and display plants needing light
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Light:");
  bool firstLightPlant = true;
  int lightLineLength = 6; // Initial length of "Light:"

  if (lightCount == 0) {
    lcd.print(" NA");
  } else {
    for (int i = 0; i < numPlants; i++) {
      if (needLight[i]) {
        String plantStr = " P" + String(i + 1);
        if (lightLineLength + plantStr.length() > 16) {
          lcd.setCursor(0, 1);
          lightLineLength = 0; // Reset line length counter
        }
        lcd.print(plantStr);
        lightLineLength += plantStr.length();
      }
    }
  }

  delay(2000); // Delay to show light results

  // Final message and power off
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("OK,");
  lcd.setCursor(0, 1);
  lcd.print("see you!");

  // Power off
  digitalWrite(powPin, LOW);
  delay(1000); // Ensure message is displayed before powering off
}

char getKeyWithDebounce() {
  char key = keypad.getKey();
  if (key != NO_KEY) {
    unsigned long currentTime = millis();
    if (currentTime - lastKeyPressTime > debounceDelay) {
      lastKeyPressTime = currentTime;
      return key;
    }
  }
  return NO_KEY;
}
