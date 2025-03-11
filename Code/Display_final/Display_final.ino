#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Updated UUIDs
static BLEUUID serviceUUID("7fbc0737-9a04-4a22-83e0-f4a5da6ded0e");
static BLEUUID charUUID("ca15c078-2a49-4f56-9462-09a35021b10b");

// Button GPIOs
const int buttonPlus = 9;
const int buttonMinus = 10;
const int resetButton = 8;  // Reset button GPIO
const int ledPin = 21;
const int stepsPerRevolution = 600;

// Task and goal counters
int taskCount = 0;
int goalCount = 5;

// OLED display settings
#define COIL_A1 5  // Coil A+
#define COIL_A2 3  // Coil A-
#define COIL_B1 4  // Coil B+
#define COIL_B2 2  // Coil B-
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // No reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// Function Prototypes
void updateDisplay();
bool connectToServer();
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

// BLE Client Connection Callback
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connected to BLE Server!");
    digitalWrite(ledPin, HIGH);  // Turn LED ON
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from BLE Server!");
    digitalWrite(ledPin, LOW);   // Turn LED OFF
  }
};

// BLE Scan Callback
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Found BLE device: ");
    if (advertisedDevice.haveName()) {
        Serial.println(advertisedDevice.getName().c_str());
    } else {
        Serial.println("Unnamed Device");
    }

    Serial.print("Device Address: ");
    Serial.println(advertisedDevice.getAddress().toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
        Serial.println("Found HabitTracker_Sensor, connecting...");
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;
    }
  }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE client...");

    BLEDevice::init("HabitTracker_Display");

    pinMode(buttonPlus, INPUT_PULLUP);
    pinMode(buttonMinus, INPUT_PULLUP);
    pinMode(resetButton, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // Initialize stepper motor pins
    pinMode(COIL_A1, OUTPUT);
    pinMode(COIL_A2, OUTPUT);
    pinMode(COIL_B1, OUTPUT);
    pinMode(COIL_B2, OUTPUT);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("⚠️ SSD1306 OLED initialization failed!");
        for (;;);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    updateDisplay();

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());  
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
}

// **Function to step the X27 motor**
void stepMotor(int step) {
    switch (step % 4) {
        case 0:
            digitalWrite(COIL_A1, HIGH);
            digitalWrite(COIL_A2, LOW);
            digitalWrite(COIL_B1, HIGH);
            digitalWrite(COIL_B2, LOW);
            break;
        case 1:
            digitalWrite(COIL_A1, LOW);
            digitalWrite(COIL_A2, HIGH);
            digitalWrite(COIL_B1, HIGH);
            digitalWrite(COIL_B2, LOW);
            break;
        case 2:
            digitalWrite(COIL_A1, LOW);
            digitalWrite(COIL_A2, HIGH);
            digitalWrite(COIL_B1, LOW);
            digitalWrite(COIL_B2, HIGH);
            break;
        case 3:
            digitalWrite(COIL_A1, HIGH);
            digitalWrite(COIL_A2, LOW);
            digitalWrite(COIL_B1, LOW);
            digitalWrite(COIL_B2, HIGH);
            break;
    }
}

// **Function to move the X27 stepper motor back and forth three times**
void celebrateTaskCompletion() {
    Serial.println("🎉 Moving X27 stepper motor for celebration!");

    for (int j = 0; j < 3; j++) {  // Repeat movement 3 times
        Serial.print("🎉 Celebration Cycle: ");
        Serial.println(j + 1);

        // Move forward
        for (int i = 0; i < stepsPerRevolution / 4; i++) {
            stepMotor(i);
            delay(2);  // Adjust delay for speed
        }

        // Move backward
        for (int i = stepsPerRevolution / 4; i > 0; i--) {
            stepMotor(i);
            delay(2);
        }
    }

    Serial.println("🎉 Celebration movement complete!");
}

void loop() {
    if (doConnect) {
        if (connectToServer()) {
            Serial.println("Connected to BLE server.");
        } else {
            Serial.println("Failed to connect to server.");
        }
        doConnect = false;
    }

    if (!connected && doScan) {
        BLEDevice::getScan()->start(0);
    }

    // Handle goal setting buttons
    if (digitalRead(buttonPlus) == LOW) {
        goalCount++;
        Serial.print("Goal increased! New goal: ");
        Serial.println(goalCount);
        updateDisplay();
        delay(300);
    }

    if (digitalRead(buttonMinus) == LOW) {
        if (goalCount > 0) {
            goalCount--;
            Serial.print("Goal decreased! New goal: ");
            Serial.println(goalCount);
            updateDisplay();
        } else {
            Serial.println("Goal cannot be negative!");
        }
        delay(300);
    }

    // **Reset button handling**
    if (digitalRead(resetButton) == LOW) {
        Serial.println("🔄 Reset button pressed! Resetting counters...");
        taskCount = 0;
        goalCount = 5;  // Default goal count
        updateDisplay();
        delay(500);  // Debounce delay
    }

    delay(100);
}

// **Display Update Function**
void updateDisplay() {
    display.clearDisplay();
    
    // Show taskCount/goalCount on the first line
    display.setCursor(10, 10);
    display.print("Tasks: ");
    display.print(taskCount);
    display.print("/");
    display.print(goalCount);

    // Calculate and display completion rate
    float completionRate = (goalCount > 0) ? ((float)taskCount / goalCount) * 100 : 0;
    
    display.setCursor(10, 30);  // Move to the next line
    display.print("Completion: ");
    display.print((int)completionRate);  // Convert to int for clean display
    display.print("%");

    display.display();
}

// **BLE Notification Callback**
void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

    Serial.print("Received BLE Notification: ");
    int receivedValue = pData[0];
    Serial.print("Token Detection Value: ");
    Serial.println(receivedValue);

    if (receivedValue == 1) {
        taskCount++;
        Serial.print("✅ Task completed! Total tasks completed: ");
        Serial.println(taskCount);
        updateDisplay();

        // **Trigger Stepper Celebration**
        celebrateTaskCompletion();
    } else {
        Serial.println("⚠️ Unexpected value received.");
    }
}

// **BLE Server Connection**
bool connectToServer() {
    Serial.print("Connecting to BLE Server at ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created BLE client");

    pClient->setClientCallbacks(new MyClientCallback());

    if (pClient->connect(myDevice)) {
        Serial.println(" - Successfully connected to BLE server!");

        if (myDevice->haveName()) {
            Serial.print(" - Connected to server name: ");
            Serial.println(myDevice->getName().c_str());
        } else {
            Serial.println(" - Connected to unnamed BLE server.");
        }

        pClient->setMTU(517);

        BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
        if (pRemoteService == nullptr) {
            Serial.println("Failed to find service UUID");
            pClient->disconnect();
            return false;
        }
        Serial.println(" - Found service");

        pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
        if (pRemoteCharacteristic == nullptr) {
            Serial.println("Failed to find characteristic UUID");
            pClient->disconnect();
            return false;
        }
        Serial.println(" - Found characteristic");

        if (pRemoteCharacteristic->canNotify()) {
            pRemoteCharacteristic->registerForNotify(notifyCallback);
        }

        connected = true;
        return true;
    } else {
        Serial.println(" - Failed to connect to BLE server.");
        return false;
    }
}