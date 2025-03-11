#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Preferences.h>  // Library for saving progress

// BLE UUIDs
static BLEUUID serviceUUID("7fbc0737-9a04-4a22-83e0-f4a5da6ded0e");
static BLEUUID charUUID("ca15c078-2a49-4f56-9462-09a35021b10b");

// GPIO Pins
const int buttonPlus = 9;
const int buttonMinus = 10;
const int resetButton = 8;
const int ledPin = 21;
const int stepsPerRevolution = 600;

// Stepper Motor Pins
#define COIL_A1 5  // Coil A+
#define COIL_A2 3  // Coil A-
#define COIL_B1 4  // Coil B+
#define COIL_B2 2  // Coil B-

// OLED Display Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // No reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Preferences for Saving Progress
Preferences preferences;

// Task and Goal Tracking
int taskCount = 0;
int goalCount = 5;

// BLE Connection Tracking
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// **Function Prototypes**
void updateDisplay();
void saveProgress();
bool connectToServer();
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

// **BLE Client Connection Callback**
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("✅ Connected to BLE Server!");
    digitalWrite(ledPin, HIGH);  // Turn LED ON when connected
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("⚠️ Disconnected from BLE Server!");
    digitalWrite(ledPin, LOW);  // Turn LED OFF when disconnected
  }
};

// **BLE Scan Callback**
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("🔍 Found BLE device: ");
    if (advertisedDevice.haveName()) {
        Serial.println(advertisedDevice.getName().c_str());
    } else {
        Serial.println("Unnamed Device");
    }

    Serial.print("📡 Device Address: ");
    Serial.println(advertisedDevice.getAddress().toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
        Serial.println("✅ Found HabitTracker_Sensor, connecting...");
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;
    }
  }
};

// **Function to Update OLED Display**
void updateDisplay() {
    display.clearDisplay();
    
    // **Test Message**
    display.setTextSize(1);
    display.setCursor(10, 10);
    display.print("OLED Working!");
    
    // Show taskCount/goalCount on the first line
    display.setCursor(10, 30);
    display.print("Tasks: ");
    display.print(taskCount);
    display.print("/");
    display.print(goalCount);

    // Calculate and display completion rate
    float completionRate = (goalCount > 0) ? ((float)taskCount / goalCount) * 100 : 0;
    
    display.setCursor(10, 50);
    display.print("Completion: ");
    display.print((int)completionRate);
    display.print("%");

    display.display();
}

void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Starting BLE client...");

    BLEDevice::init("HabitTracker_Display");

    pinMode(buttonPlus, INPUT_PULLUP);
    pinMode(buttonMinus, INPUT_PULLUP);
    pinMode(resetButton, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // **Initialize Stepper Motor Pins**
    pinMode(COIL_A1, OUTPUT);
    pinMode(COIL_A2, OUTPUT);
    pinMode(COIL_B1, OUTPUT);
    pinMode(COIL_B2, OUTPUT);

    // **Initialize OLED Display**
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("⚠️ SSD1306 OLED initialization failed!");
        for (;;);
    }

    // **Load Saved Progress**
    preferences.begin("habitTracker", false);
    taskCount = preferences.getInt("taskCount", 0);
    goalCount = preferences.getInt("goalCount", 5);
    preferences.end();

    Serial.print("📂 Loaded progress: ");
    Serial.print(taskCount);
    Serial.print("/");
    Serial.println(goalCount);

    updateDisplay();

    // **Start BLE Scanning**
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
}

void loop() {
    if (doConnect) {
        if (connectToServer()) {
            Serial.println("✅ Connected to BLE server.");
        } else {
            Serial.println("⚠️ Failed to connect to server.");
        }
        doConnect = false;
    }

    if (!connected && doScan) {
        BLEDevice::getScan()->start(0);
    }

    // **Goal Setting with Buttons**
    if (digitalRead(buttonPlus) == LOW) {
        goalCount++;
        saveProgress();
        updateDisplay();
        delay(300);
    }

    if (digitalRead(buttonMinus) == LOW) {
        if (goalCount > 0) {
            goalCount--;
            saveProgress();
            updateDisplay();
        }
        delay(300);
    }

    // **Reset Button Handling**
    if (digitalRead(resetButton) == LOW) {
        Serial.println("🔄 Reset button pressed! Resetting counters...");
        taskCount = 0;
        goalCount = 5;
        saveProgress();
        updateDisplay();
        delay(500);
    }

    delay(100);
}

// **Function to Save Progress to Flash Memory**
void saveProgress() {
    preferences.begin("habitTracker", false);
    preferences.putInt("taskCount", taskCount);
    preferences.putInt("goalCount", goalCount);
    preferences.end();
    Serial.println("💾 Progress saved!");
}

// **BLE Notification Callback**
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    Serial.print("📩 Received BLE Notification: ");
    int receivedValue = pData[0];

    if (receivedValue == 1) {
        taskCount++;
        saveProgress();
        updateDisplay();
    }
}

// **BLE Server Connection**
bool connectToServer() {
    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    if (pClient->connect(myDevice)) {
        BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
        if (pRemoteService) {
            pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
            if (pRemoteCharacteristic && pRemoteCharacteristic->canNotify()) {
                pRemoteCharacteristic->registerForNotify(notifyCallback);
            }
        }
        connected = true;
        return true;
    }
    return false;
}