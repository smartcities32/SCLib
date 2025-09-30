// SC_Library.h
#ifndef SC_LIBRARY_H
#define SC_LIBRARY_H

#include <Arduino.h>
#include <Wire.h> 
#include <EEPROM.h>
#include "RTClib.h" 
#include <ArduinoJson.h> 

#ifdef ESP32
#include <WiFi.h>
#include <WebServer.h>
#elif ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h> // NEW: Added for OTA
#include <ESP8266mDNS.h>             // NEW: Added for mDNS
#define WebServer ESP8266WebServer 
// Include for external EEPROM on ESP8266
#include <Wire.h> // Already included, but good to keep in mind
#define EXTERNAL_EEPROM_ADDR 0x50 // Default I2C address for 24C256 EEPROM
#define USE_EXTERNAL_EEPROM // Define this to conditionally use external EEPROM
#endif

#define EEPROM_SDA_PIN 0
#define EEPROM_SCL_PIN 2

// --- OTA/Device Definitions ---
#define FIRMWARE_VERSION "2.0.0"      // NEW: Firmware version
#define OTA_USERNAME "admin"          // NEW: OTA Credentials
#define OTA_PASSWORD "admin"          // NEW: OTA Credentials


// --- Hardware Definitions ---
#define RELAY_PIN 16
#define EEPROM_SIZE 1024 // This will be used for both internal and external (if defined)
#define EX_EEPROM_SIZE 32000 // This will be used for both internal and external (if defined)

#define SSID_MAX_LEN 15
#define PASSWORD_MAX_LEN 15
#define USER_TAG_LEN 11 
//#define MAX_USER_TAGS 300

// Core module settings
#define RELAY_STATE_ADDR 0 // bool (1 byte)
#define OP_METHOD_ADDR 1 // uint8_t (1 byte)
#define SSID_ADDR 2
#define PASSWORD_ADDR 18

// User Tag Management
#define ADD_CARD_ADDR 34
#define REMOVE_CARD_ADDR 45
#define Max_Num_OF_USERS_ADD 56
#define USER_TAG_COUNT_ADDR 60 // int (4 bytes)
#define USER_TAGS_START_ADDR 64 // Start address for user tags
#define Statistics_START_ADDR  (USER_TAGS_START_ADDR + (MAX_USER_TAGS * USER_TAG_LEN))

extern WebServer server; 

// --- MainControlClass (Base Class) ---
class MainControlClass {
protected: // Protected members are accessible by derived classes
    WebServer& _server; // Reference to the main WebServer instance
#ifdef USE_EXTERNAL_EEPROM
    // No direct EEPROMClass reference for external, we'll use Wire directly or a custom class
#else
    EEPROMClass& _eeprom; // Reference to EEPROM
#endif
    int _relayPin; // Relay pin

#ifdef ESP8266 // NEW: OTA Server and Hostname for ESP8266
    ESP8266HTTPUpdateServer _httpUpdater;
    const char* _hostname = "esp-control"; // Default mDNS hostname
#endif

public:
    // Constructor for MainControlClass
#ifdef USE_EXTERNAL_EEPROM
    MainControlClass(WebServer& serverRef, int relayPin); // No EEPROMClass reference
#else
    MainControlClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef);
#endif

    void beginAPAndWebServer(const char* ap_ssid, const char* ap_password);
    void handleClient();
    void resetConfigurations();
    void setRelayPhysicalState(bool state);
    String readStringFromEEPROM(int address, int max_len);
    void saveStringToEEPROM(int address, const String& data, int max_len);
    void saveFixedStringToEEPROM(int address, const String& data, int max_len);
    uint8_t readOperationMethod();
    void writeOperationMethod(uint8_t method);
    void saveRelayStateToEEPROM(bool state);
    bool getRelayStateFromEEPROM();
    
    // NEW: Function to set up OTA (made public for external call if needed, but called internally)
    void setupOTA(); 
    
    // External EEPROM specific functions (remains unchanged)
#ifdef USE_EXTERNAL_EEPROM
  template <typename T>
    void externalEEPROMPut(int address, const T& value) {
        externalEEPROMWriteBytes(address, (const byte*)&value, sizeof(T));
    } // **Definition included in header**

    template <typename T>
    void externalEEPROMGet(int address, T& value) {
        externalEEPROMReadBytes(address, (byte*)&value, sizeof(T));
    }
    uint8_t externalEEPROMReadByte(unsigned int address);
    void externalEEPROMWriteByte(unsigned int address, uint8_t data);
    void externalEEPROMReadBytes(unsigned int address, byte* buffer, int length);
    void externalEEPROMWriteBytes(unsigned int address, const byte* buffer, int length);
    int externalEEPROMReadInt(unsigned int address);
    void externalEEPROMWriteInt(unsigned int address, int value);
    String externalEEPROMReadString(uint16_t address, uint16_t length);
    void externalEEPROMWriteString(uint16_t address, String data);
#endif
    int MAX_USER_TAGS = 300;
private: 
    // ... (Private handlers remain the same) ...
    // --- Wi-Fi Management Handlers ---
    void handleSetSSID();
    void handleGetSSID();
    void handleSetPassword();
    void handleGetPassword();

    // --- Relay Management Handlers ---
    void handleSetRelayState();
    void handleGetRelayState();
    void handleToggleRelay();

    // --- General Handlers ---
    void handleNotFound();
	void handleGetnetworkinfo();
    void handleSetnetworkinfo();
    void handleGetOperationMethod();
    void handleSetOperationMethod();  
	void handleGetSystemInfo();
    void handleSetSystemInfo();
  // NEW: Handlers for OTA/System Info (Cleaned from LED control)
    void handleRoot();
    void handleStatus();
    void handleReboot();
    void handleInfo();
    
};

// --- RTCManager Class (Inherits from MainControlClass - No Change) ---
class RTCManager : public MainControlClass {
protected: 
    RTC_DS3231 _rtc;
    DateTime timeNow;
// ... (rest of RTCManager remains the same) ...
public:
#ifdef USE_EXTERNAL_EEPROM
    RTCManager(WebServer& serverRef, int relayPin); // Constructor with base class parameters
#else
    RTCManager(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef); // Constructor with base class parameters
#endif

    bool beginRTC();
    DateTime now();
    void adjustRTC(const DateTime& dateTime);

    // RTC-specific handlers and their setup method
    void handleGetTime();
    void handleSetTime();
    void setupRTCEndpoints();
};


// --- UserManagementClass (Derived Class - No Change) ---
class UserManagementClass : public MainControlClass {
private:
    int _userTagCount; // Internal variable to keep track of the count

public:
    // Constructor for UserManagementClass, calls base class constructor
#ifdef USE_EXTERNAL_EEPROM
    UserManagementClass(WebServer& serverRef, int relayPin);
#else
    UserManagementClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef);
#endif
// ... (rest of UserManagementClass remains the same) ...
    void setupUserEndpoints();
public: 
    void saveUserTagCountToEEPROM(int count);
    int getUserTagCountFromEEPROM();
    int findUserTagAddress(const String& tag);
    int findEmptyUserTagSlot();

    // --- User Management Handlers ---
    void handleAddUserTag();
    void handleDeleteUserTag();
    void handleCheckUserTag();
    void handleGetUserTagCount();
    void handleUseingUserTag();
    void handleGettags();
    String _trim(String& str);
    void handleDeleteAllUserTags();
    bool storeTag(String tag);
    bool DeleteTag(String tag);
    bool checkTag(String tag);
    void addCard();
    void removeCard();
    String generatePassword() ;
    String shuffleString(String str) ;
    char getRandomUpperCase();
    char getRandomLowerCase();
    char getRandomCharacter();
    char getRandomSymbol();
    char getRandomNumber();
    void generate_SSIDAndPASS();

};

#endif // SC_LIBRARY_H
