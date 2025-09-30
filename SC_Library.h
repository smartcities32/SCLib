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
#include <ESP8266HTTPUpdateServer.h> // مكتبة OTA
#include <ESP8266mDNS.h>             // مكتبة mDNS
#define WebServer ESP8266WebServer 
// Include for external EEPROM on ESP8266
#include <Wire.h> 
#define EXTERNAL_EEPROM_ADDR 0x50 
#define USE_EXTERNAL_EEPROM 
#endif

#define EEPROM_SDA_PIN 0
#define EEPROM_SCL_PIN 2


// --- Hardware Definitions ---
#define RELAY_PIN 16
#define EEPROM_SIZE 1024 
#define EX_EEPROM_SIZE 32000 

#define SSID_MAX_LEN 15
#define PASSWORD_MAX_LEN 15
#define USER_TAG_LEN 11 

// Core module settings
#define RELAY_STATE_ADDR 0 // bool (1 byte)
#define OP_METHOD_ADDR 1 // uint8_t (1 byte)
#define SSID_ADDR 2
#define PASSWORD_ADDR 18
#define FIRMWARE_VERSION_ADDR 30       // ✨ عنوان جديد لتخزين إصدار البرنامج
#define FIRMWARE_VERSION_MAX_LEN 10    // ✨ أقصى طول لسلسلة الإصدار (مثل "1.0.0")

// User Tag Management
#define ADD_CARD_ADDR 34
#define REMOVE_CARD_ADDR 45
#define Max_Num_OF_USERS_ADD 56
#define USER_TAG_COUNT_ADDR 60 // int (4 bytes)
#define USER_TAGS_START_ADDR 64 // Start address for user tags
#define Statistics_START_ADDR  (USER_TAGS_START_ADDR + (MAX_USER_TAGS * USER_TAG_LEN))

#define FIRMWARE_VERSION "1.0.0" // رقم الإصدار الحالي للبرنامج

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
#ifdef ESP8266
    ESP8266HTTPUpdateServer _httpUpdater; // كائن خادم تحديث OTA
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
    
    // دوال OTA وإدارة النظام
    void setupOTAServer(const char* hostName, const char* otaPath, const char* otaUser, const char* otaPass);
    void handleRoot();
    void handleReboot();
    void handleInfo();
    String getSystemInfoHtml();
    
    // ✨ دوال إدارة الإصدار الجديدة
    void saveFirmwareVersion(const String& version);
    void handleGetFirmwareVersion();

    // External EEPROM specific functions
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
    
};

// --- RTCManager Class (Inherits from MainControlClass) ---
class RTCManager : public MainControlClass {
protected: 
    RTC_DS3231 _rtc;
    DateTime timeNow;

public:
#ifdef USE_EXTERNAL_EEPROM
    RTCManager(WebServer& serverRef, int relayPin); 
#else
    RTCManager(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef); 
#endif

    bool beginRTC();
    DateTime now();
    void adjustRTC(const DateTime& dateTime);

    // RTC-specific handlers and their setup method
    void handleGetTime();
    void handleSetTime();
    void setupRTCEndpoints();
};


// --- UserManagementClass (Derived Class) ---
class UserManagementClass : public MainControlClass {
private:
    int _userTagCount; 

public:
    // Constructor for UserManagementClass, calls base class constructor
#ifdef USE_EXTERNAL_EEPROM
    UserManagementClass(WebServer& serverRef, int relayPin);
#else
    UserManagementClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef);
#endif
   
    void setupUserEndpoints();

public: 
    // ... (بقية دوال إدارة المستخدمين)
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
