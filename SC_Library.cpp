// SC_Library.cpp
#include "SC_Library.h"

// External EEPROM Helper Functions (Implement these in SC_Library.cpp)
#ifdef USE_EXTERNAL_EEPROM
uint8_t MainControlClass::externalEEPROMReadByte(unsigned int address) {
    Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(EXTERNAL_EEPROM_ADDR, 1);
    return Wire.read();
}

void MainControlClass::externalEEPROMWriteByte(unsigned int address, uint8_t data) {
    Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();
    delay(5); // Small delay to allow EEPROM to complete write cycle
}
// Function to write a string to EEPROM starting at the specified address
void MainControlClass::externalEEPROMWriteString(uint16_t address, String data) {
  Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
  Wire.write((address >> 8) & 0xFF); // MSB of address
  Wire.write(address & 0xFF);        // LSB of address

  for (uint16_t i = 0; i < data.length(); i++) {
    Wire.write(data[i]);
  }
  Wire.endTransmission();
  delay(5);  // Small delay for EEPROM write cycle
}

// Function to read a string from EEPROM starting at the specified address
String MainControlClass::externalEEPROMReadString(uint16_t address, uint16_t length) {
  String result = "";
  Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
  Wire.write((address >> 8) & 0xFF); // MSB of address
  Wire.write(address & 0xFF);        // LSB of address
  Wire.endTransmission();

  Wire.requestFrom(EXTERNAL_EEPROM_ADDR, length);

  while (Wire.available()) {
    char c = Wire.read();
    result += c;
  }

  return result;
}

void MainControlClass::externalEEPROMReadBytes(unsigned int address, byte* buffer, int length) {
    Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(EXTERNAL_EEPROM_ADDR, length);
    for (int i = 0; i < length; i++) {
        if (Wire.available()) {
            buffer[i] = Wire.read();
        }
    }
}

void MainControlClass::externalEEPROMWriteBytes(unsigned int address, const byte* buffer, int length) {
    Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
    Wire.write((int)(address >> 8));   // MSB
    Wire.write((int)(address & 0xFF)); // LSB
    for (int i = 0; i < length; i++) {
        Wire.write(buffer[i]);
    }
    Wire.endTransmission();
    delay(5); // Small delay to allow EEPROM to complete write cycle
}

int MainControlClass::externalEEPROMReadInt(unsigned int address) {
    int value;
    externalEEPROMReadBytes(address, (byte*)&value, sizeof(int));
    return value;
}

void MainControlClass::externalEEPROMWriteInt(unsigned int address, int value) {
    externalEEPROMWriteBytes(address, (const byte*)&value, sizeof(int));
}

template <typename T>
void MainControlClass::externalEEPROMPut(int address, const T& value) {
    externalEEPROMWriteBytes(address, (const byte*)&value, sizeof(T));
}

template <typename T>
void MainControlClass::externalEEPROMGet(int address, T& value) {
    externalEEPROMReadBytes(address, (byte*)&value, sizeof(T));
}
#endif // USE_EXTERNAL_EEPROM

// RTCManager Constructor
#ifdef USE_EXTERNAL_EEPROM
RTCManager::RTCManager(WebServer& serverRef, int relayPin)
    : MainControlClass(serverRef, relayPin), _rtc() {
#else
RTCManager::RTCManager(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef)
    : MainControlClass(serverRef, relayPin, eepromRef), _rtc() {
#endif
    // Constructor
}

bool RTCManager::beginRTC() {
    Wire.begin(); // Start I2C communication
    if (!_rtc.begin()) {
        Serial.println("RTC not found! Please check wiring.");
        return false;
    }
    if (_rtc.lostPower()) {
        Serial.println("RTC lost power, setting time!");
        _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    return true;
}

DateTime RTCManager::now() {
    return _rtc.now(); // Corrected: return actual RTC time
}

void RTCManager::adjustRTC(const DateTime& dateTime) {
    _rtc.adjust(dateTime);
}

// Implement RTCManager's time handlers
void RTCManager::handleGetTime() {
    DateTime now = _rtc.now();
    String response = "{ \"year\": " + String(now.year()) +
                      ", \"month\": " + String(now.month()) +
                      ", \"day\": " + String(now.day()) +
                      ", \"hour\": " + String(now.hour()) +
                      ", \"minute\": " + String(now.minute()) +
                      ", \"second\": " + String(now.second()) + " }";
                      Serial.println(response);
    _server.send(200, "application/json", response);
}

void RTCManager::handleSetTime() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
      deserializeJson(doc, _server.arg("plain"));
      int year = doc["year"];
      int month = doc["month"];
      int day = doc["day"];
      int hour = doc["hour"];
      int minute = doc["minute"];
      int second = doc["second"];
      _rtc.adjust(DateTime(year, month, day, hour, minute, second));
      _server.send(200, "application/json", "{\"status\":\"time updated\"}");
    } else {
      _server.send(400, "application/json", "{\"error\":\"Missing body\"}");
    }
}


void RTCManager::setupRTCEndpoints() {
    _server.on("/api/time/get", HTTP_GET, [this]() { handleGetTime(); });
    _server.on("/api/time/set", HTTP_POST, [this]() { handleSetTime(); });
}


// --- MainControlClass Implementations ---
#ifdef USE_EXTERNAL_EEPROM
MainControlClass::MainControlClass(WebServer& serverRef, int relayPin)
    : _server(serverRef), _relayPin(relayPin) {
#else
MainControlClass::MainControlClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef)
    : _server(serverRef), _relayPin(relayPin), _eeprom(eepromRef) {
#endif
    // Initialize EEPROM (only once in the base class)
}

void MainControlClass::beginAPAndWebServer(const char* ap_ssid, const char* ap_password) {
    Wire.begin(); // Start I2C communication (essential for RTC and external EEPROM)
   // Wire.begin(EEPROM_SDA_PIN, EEPROM_SCL_PIN); // Start I2C communication (essential for RTC and external EEPROM)

#ifndef USE_EXTERNAL_EEPROM
    if (!_eeprom.begin(EEPROM_SIZE)) {
        Serial.println("Failed to initialise EEPROM");
        Serial.println("Restarting...");
        delay(1000);
        ESP.restart();
    }
    Serial.println("EEPROM initialized successfully.");
#else
    // For external EEPROM, Wire.begin() is usually sufficient.
    Serial.println("External EEPROM (24C256) assumed to be initialized via Wire.begin().");
#endif

    // Initialize relay pin
    pinMode(_relayPin, OUTPUT);
    // Set initial relay state from EEPROM
    bool savedState = getRelayStateFromEEPROM();
    digitalWrite(_relayPin, savedState ? HIGH : LOW);
    Serial.print("Initial relay state from EEPROM: ");
    Serial.println(savedState ? "ON" : "OFF");
// saveStringToEEPROM(SSID_ADDR, "hmade", SSID_MAX_LEN);
 //saveStringToEEPROM(PASSWORD_ADDR, "987654321", PASSWORD_MAX_LEN);
//     delay(3000);
    String ssid = readStringFromEEPROM(SSID_ADDR, SSID_MAX_LEN);
    String password = readStringFromEEPROM(PASSWORD_ADDR, PASSWORD_MAX_LEN);
    //String ssid = externalEEPROMReadString(SSID_ADDR, SSID_MAX_LEN);
   // String password = externalEEPROMReadString(PASSWORD_ADDR, PASSWORD_MAX_LEN);
    if (ssid.isEmpty() || password.isEmpty() || ssid == "\0" || password == "\0") { // Added check for empty string from readStringFromEEPROM
        Serial.println("SSID or Password not set in EEPROM. Using default AP credentials.");
        ssid = ap_ssid;
        password = ap_password;
        // Also save defaults to EEPROM if they were empty
        saveStringToEEPROM(SSID_ADDR, ap_ssid, SSID_MAX_LEN);
        saveStringToEEPROM(PASSWORD_ADDR, ap_password, PASSWORD_MAX_LEN);
    } 
        Serial.print("Setting up AP: ");
        Serial.println(ssid);
        Serial.print("password: ");
        Serial.println(password);
    
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ssid, password);
    
        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(IP);

    // Wi-Fi Management
    _server.on("/api/wifi/set_ssid", HTTP_POST, [this]() { handleSetSSID(); });
    _server.on("/api/wifi/get_ssid", HTTP_GET, [this]() { handleGetSSID(); });
    _server.on("/api/wifi/set_password", HTTP_POST, [this]() { handleSetPassword(); });
    _server.on("/api/wifi/get_password", HTTP_GET, [this]() { handleGetPassword(); });
    _server.on("/api/wifi/get_network_info", HTTP_GET, [this]() { handleGetnetworkinfo(); });
    _server.on("/api/wifi/set_network_info", HTTP_POST, [this]() { handleSetnetworkinfo(); });

    // Relay Management
    _server.on("/api/relay/set_state", HTTP_POST, [this]() { handleSetRelayState(); });
    _server.on("/api/relay/get_state", HTTP_GET, [this]() { handleGetRelayState(); });
    _server.on("/api/relay/toggle", HTTP_POST, [this]() { handleToggleRelay(); });

    // Utility
    _server.on("/api/reset", HTTP_POST, [this]() { resetConfigurations(); }); // New API for reset
    _server.on("/api/op_method", HTTP_GET, [this]() { handleGetOperationMethod(); });
    _server.on("/api/op_method", HTTP_POST, [this]() { handleSetOperationMethod(); });

    // Not Found Handler (can be overridden by derived classes if needed)
    _server.onNotFound([this]() { handleNotFound(); });

    _server.begin();
    Serial.println("HTTP server started");
}


void MainControlClass::handleGetOperationMethod() {
    uint8_t method = readOperationMethod();
    String response = "{\"status\":\"success\",\"method\":" + String(method) + "}";
    _server.send(200, "application/json", response);
}

void MainControlClass::handleSetOperationMethod() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<100> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        uint8_t method = doc["method"].as<uint8_t>();
        if (method == 0 || method == 1) {
            writeOperationMethod(method);
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Operation method set to " + String(method) + "\"}");
            Serial.print("Operation method set to: ");
            Serial.println(method);
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"method\":0} or {\"method\":1}\"}");
}


void MainControlClass::handleClient() {
    _server.handleClient();
}

void MainControlClass::resetConfigurations() {
    Serial.println("Resetting configurations...");
#ifdef USE_EXTERNAL_EEPROM
    externalEEPROMWriteInt(USER_TAG_COUNT_ADDR, 0); 
#else
    _eeprom.writeInt(USER_TAG_COUNT_ADDR, 0); 
    _eeprom.commit();
#endif
    saveRelayStateToEEPROM(false);
   // writeLastScheduleId(0);
    //externalEEPROMWriteString(SSID_ADDR, "Smart Timer");
    //externalEEPROMWriteString(PASSWORD_ADDR, "sM@rt123");
    saveStringToEEPROM(SSID_ADDR, "Default", SSID_MAX_LEN);
    saveStringToEEPROM(PASSWORD_ADDR, "12345678", PASSWORD_MAX_LEN);
    saveFixedStringToEEPROM(ADD_CARD_ADDR, "00009890388", USER_TAG_LEN);
    saveFixedStringToEEPROM(REMOVE_CARD_ADDR, "00009870509", USER_TAG_LEN);


    writeOperationMethod(0);

    


    Serial.println("Configurations reset. Restarting ESP...");
    _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"reset done\"}");

    delay(1000);
    ESP.restart();
}

uint8_t MainControlClass::readOperationMethod() {
    uint8_t method;
#ifdef USE_EXTERNAL_EEPROM
    method = externalEEPROMReadByte(OP_METHOD_ADDR);
#else
    method = _eeprom.read(OP_METHOD_ADDR);
#endif
    if (method != 0 && method != 1) {
        return 0; 
    }
    return method;
}

void MainControlClass::writeOperationMethod(uint8_t method) {
#ifdef USE_EXTERNAL_EEPROM
    externalEEPROMWriteByte(OP_METHOD_ADDR, method);
#else
    _eeprom.write(OP_METHOD_ADDR, method);
    _eeprom.commit();
#endif
}

void MainControlClass::saveStringToEEPROM(int address, const String& data, int max_len) {
    int len = data.length();
    if (len > max_len) {
        len = max_len; 
    }
    for (int i = 0; i < len; i++) {
#ifdef USE_EXTERNAL_EEPROM
        externalEEPROMWriteByte(address + i, data.charAt(i));
#else
        _eeprom.write(address + i, data.charAt(i));
#endif
    }
   
#ifdef USE_EXTERNAL_EEPROM
    externalEEPROMWriteByte(address + len, 0);
#else
    _eeprom.write(address + len, 0);
    _eeprom.commit();
#endif
}

void MainControlClass::saveFixedStringToEEPROM(int address, const String& data, int max_len) {
    int len = data.length();
    if (len > max_len) {
        len = max_len; 
    }
    for (int i = 0; i < len; i++) {
#ifdef USE_EXTERNAL_EEPROM
        externalEEPROMWriteByte(address + i, data.charAt(i));
#else
        _eeprom.write(address + i, data.charAt(i));
#endif
    }
   

}

String MainControlClass::readStringFromEEPROM(int address, int max_len) {
    String data = "";
    for (int i = 0; i < max_len; ++i) {
        char c;
#ifdef USE_EXTERNAL_EEPROM
        c = externalEEPROMReadByte(address + i);
#else
        c = _eeprom.read(address + i);
#endif
        if (c == 0) { 
            break;
        }
        data += c;
    }
    return data;
}

void MainControlClass::saveRelayStateToEEPROM(bool state) {
#ifdef USE_EXTERNAL_EEPROM
    externalEEPROMWriteByte(RELAY_STATE_ADDR, state ? 1 : 0);
#else
    _eeprom.write(RELAY_STATE_ADDR, state ? 1 : 0);
    _eeprom.commit();
#endif
}

bool MainControlClass::getRelayStateFromEEPROM() {
#ifdef USE_EXTERNAL_EEPROM
    return externalEEPROMReadByte(RELAY_STATE_ADDR) == 1;
#else
    return _eeprom.read(RELAY_STATE_ADDR) == 1;
#endif
}

void MainControlClass::setRelayPhysicalState(bool state) {
    digitalWrite(_relayPin, state ? HIGH : LOW);
    saveRelayStateToEEPROM(state);
}



// --- Private Handlers Implementations for MainControlClass ---
void MainControlClass::handleSetSSID() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String ssid = doc["ssid"].as<String>();
        if (!ssid.isEmpty()) {
            saveStringToEEPROM(SSID_ADDR, ssid, SSID_MAX_LEN);
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"SSID saved\"}");
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"ssid\":\"your_ssid\"}\"}");
}

void MainControlClass::handleGetSSID() {
    String ssid = readStringFromEEPROM(SSID_ADDR, SSID_MAX_LEN);
    String response = "{\"status\":\"success\",\"ssid\":\"" + ssid + "\"}";
    _server.send(200, "application/json", response);
}

void MainControlClass::handleSetPassword() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String password = doc["password"].as<String>();
        if (!password.isEmpty()) {
            saveStringToEEPROM(PASSWORD_ADDR, password, PASSWORD_MAX_LEN);
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Password saved\"}");
            Serial.print("Password set to: ");
            Serial.println(password);
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"password\":\"your_password\"}\"}");
}

void MainControlClass::handleGetPassword() {
    String password = readStringFromEEPROM(PASSWORD_ADDR, PASSWORD_MAX_LEN);
    String response = "{\"status\":\"success\",\"password\":\"" + password + "\"}";
    _server.send(200, "application/json", response);
}


void MainControlClass::handleGetnetworkinfo() {
    String ssid = readStringFromEEPROM(SSID_ADDR, SSID_MAX_LEN);
    String password = readStringFromEEPROM(PASSWORD_ADDR, PASSWORD_MAX_LEN);
    String response = "{\"status\":\"success\",\"ssid\": \"" + ssid + "\", \"password\":\"" + password + "\"}";
    _server.send(200, "application/json", response);
}
void MainControlClass::handleSetnetworkinfo() {
 if (_server.hasArg("plain")) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, _server.arg("plain"));
      String ssid = doc["ssid"];
      String password = doc["password"];
      Serial.println(ssid);
      Serial.println(password);
      saveStringToEEPROM(SSID_ADDR, ssid, SSID_MAX_LEN);
      saveStringToEEPROM(PASSWORD_ADDR, password, PASSWORD_MAX_LEN);
      _server.send(200, "application/json", "{\"status\":\"network updated\"}");
      delay(1000);
      ESP.restart();
    } else {
      _server.send(400, "application/json", "{\"error\":\"Missing body\"}");
    }
}

void MainControlClass::handleSetRelayState() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<100> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String stateStr = doc["state"].as<String>();
        if (stateStr.equalsIgnoreCase("on")) {
            setRelayPhysicalState(true);
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Relay set to ON\"}");
            Serial.println("Relay set to ON");
            return;
        } else if (stateStr.equalsIgnoreCase("off")) {
            setRelayPhysicalState(false);
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Relay set to OFF\"}");
            Serial.println("Relay set to OFF");
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"state\":\"on\"} or {\"state\":\"off\"}\"}");
}

void MainControlClass::handleGetRelayState() {
    bool state = digitalRead(_relayPin) == HIGH; 
    String stateStr = state ? "on" : "off";
    String response = "{\"status\":\"success\",\"state\":\"" + stateStr + "\"}";
    _server.send(200, "application/json", response);
}

void MainControlClass::handleToggleRelay() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<100> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        int duration = doc["duration"].as<int>();

        if (duration > 0) {
            setRelayPhysicalState(true); 
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Relay toggled ON for " + String(duration) + " seconds\"}");
            Serial.print("Relay toggled ON for ");
            Serial.print(duration);
            Serial.println(" seconds");
            delay(duration * 1000);
            setRelayPhysicalState(false); // Turn off
            Serial.println("Relay turned OFF after toggle");
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"duration\":1} or {\"duration\":5}\"}");
}


void MainControlClass::handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += _server.uri();
    message += "\nMethod: ";
    message += (_server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += _server.args();
    message += "\n";
    for (uint8_t i = 0; i < _server.args(); i++) {
        message += " " + _server.argName(i) + ": " + _server.arg(i) + "\n";
    }
    _server.send(404, "text/plain", message);
}



// --- UserManagementClass Implementations ---
#ifdef USE_EXTERNAL_EEPROM
UserManagementClass::UserManagementClass(WebServer& serverRef, int relayPin)
    : MainControlClass(serverRef, relayPin) {
#else
UserManagementClass::UserManagementClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef)
    : MainControlClass(serverRef, relayPin, eepromRef) {
#endif
}


// Function to generate a random password
String UserManagementClass::generatePassword() {
  //int length = 9;
  String password = "";

  // Ensure at least one character of each type is included
  password += getRandomUpperCase();
  password += getRandomLowerCase();
  password += getRandomSymbol();


  // Fill the remaining length with random characters
  for (int i = 0; i < 6; i++) {
    password += getRandomNumber();
  }

  // Shuffle the password to mix the characters
  //password = shuffleString(password);

  return password;
}

// Functions to get random characters of specific types
char UserManagementClass::getRandomUpperCase() {
  const char upperCaseLetters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  return upperCaseLetters[random(0, sizeof(upperCaseLetters) - 1)];
}

char UserManagementClass::getRandomLowerCase() {
  const char lowerCaseLetters[] = "abcdefghijklmnopqrstuvwxyz";
  return lowerCaseLetters[random(0, sizeof(lowerCaseLetters) - 1)];
}

char UserManagementClass::getRandomNumber() {
  const char numbers[] = "0123456789";
  return numbers[random(0, 9)];
}

char UserManagementClass::getRandomSymbol() {
  const char symbols[] = "!@$&*";
  return symbols[random(0, 4)];
}

// Function to get a random character from the available sets
char UserManagementClass::getRandomCharacter() {
  int charType = random(0, 4);  // 0 = uppercase, 1 = lowercase, 2 = numbers, 3 = symbols

  switch (charType) {
    case 0: return getRandomUpperCase();
    case 1: return getRandomLowerCase();
    case 2: return getRandomNumber();
    case 3: return getRandomSymbol();
  }
  return '!';  // Fallback character
}

// Function to shuffle a string
String UserManagementClass::shuffleString(String str) {
  for (int i = 0; i < str.length(); i++) {
    int randomIndex = random(0, str.length());
    char temp = str[i];
    str[i] = str[randomIndex];
    str[randomIndex] = temp;
  }
  return str;
}

// Assuming _server is a member of UserManagementClass (e.g., an ESP8266WebServer or ESP32WebServer object)
// And generatePassword() is an existing function that generates a password string.

void UserManagementClass::generate_SSIDAndPASS() {

#ifdef ESP32
   uint64_t mac = ESP.getEfuseMac(); // Get 64-bit MAC for ESP32
#elif ESP8266
   uint32_t mac = ESP.getChipId();   // Get 32-bit Chip ID for ESP8266
#endif

  char macStr[13]; // Buffer for the 12-character hex string + null terminator
  sprintf(macStr, "%012llX", mac); // Format to 12-char uppercase hex, zero-padded

  String ssid = String(macStr); // Convert char array to String for use as SSID
  String password = generatePassword(); // Generate a random password

  String jsonResponse = "{\"SSID\":\"" + ssid + "\",\"PASS\":\"" + password + "\"}";
  Serial.println(jsonResponse);
  _server.send(200, "application/json", jsonResponse);
}

void UserManagementClass::setupUserEndpoints() {
    _server.on("/api/users/add_tag", HTTP_POST, [this]() { handleAddUserTag(); });
    _server.on("/api/users/delete_tag", HTTP_POST, [this]() { handleDeleteUserTag(); });
    _server.on("/api/users/delete_all_tags", HTTP_POST, [this]() { handleDeleteAllUserTags(); });
    _server.on("/api/users/check_tag", HTTP_POST, [this]() { handleCheckUserTag(); });
    _server.on("/api/users/get_count", HTTP_GET, [this]() { handleGetUserTagCount(); });
    _server.on("/api/users/use_tag", HTTP_POST, [this]() { handleUseingUserTag(); });
    _server.on("/api/users/remove_card", HTTP_POST, [this]() { removeCard(); });
    _server.on("/api/users/add_card", HTTP_POST, [this]() { addCard(); });
   // _server.on("/api/users/get_statistics", HTTP_GET, [this]() { handleGetStatistics(); });
   // _server.on("/api/users/get_generate_SSIDAndPASS", HTTP_GET, [this]() { generate_SSIDAndPASS(); });


    _server.on("/api/users/get_tags", HTTP_GET, [this]() { handleGettags(); });
}

void UserManagementClass::handleDeleteAllUserTags() {
#ifdef USE_EXTERNAL_EEPROM
    externalEEPROMWriteInt(USER_TAG_COUNT_ADDR, 0); 
#else
_eeprom.writeInt(USER_TAG_COUNT_ADDR, 0); 
_eeprom.commit();
#endif
    _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"delete All done\"}");
}

void UserManagementClass::saveUserTagCountToEEPROM(int count) {
#ifdef USE_EXTERNAL_EEPROM
externalEEPROMWriteInt(USER_TAG_COUNT_ADDR, count);
#else
_eeprom.writeInt(USER_TAG_COUNT_ADDR, count);
    _eeprom.commit();
#endif
}

int UserManagementClass::getUserTagCountFromEEPROM() {
#ifdef USE_EXTERNAL_EEPROM
    return externalEEPROMReadInt(USER_TAG_COUNT_ADDR);
#else
    return _eeprom.readInt(USER_TAG_COUNT_ADDR);
#endif
}

int UserManagementClass::findUserTagAddress(const String& tag) {
    int userCount = getUserTagCountFromEEPROM();
    for (int i = 0; i < userCount; ++i) {
        int currentTagAddr = USER_TAGS_START_ADDR + (i * USER_TAG_LEN);
        String storedTag = readStringFromEEPROM(currentTagAddr, USER_TAG_LEN);
       // storedTag = _trim(storedTag);
        // On first power up, EEPROM might read 0xFF for empty cells.
        // Also check if the first character is not 0xFF to ensure it's a written tag.
        // char firstChar;
        // #ifdef USE_EXTERNAL_EEPROM
        // firstChar = externalEEPROMReadByte(currentTagAddr);
        // #else
        // firstChar = _eeprom.read(currentTagAddr);
        // #endif
        if (storedTag == tag && !storedTag.isEmpty() ) {
            return i;
        }
    }
    return -1;
}


    bool UserManagementClass::storeTag(String tag) {
        // Pad with leading zeros if tag is shorter than USER_TAG_LEN
        String temp = "";
        for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
            temp += "0";
        }
        tag = temp + tag;
        Serial.println(tag);
            if (findUserTagAddress(tag) != -1){
                Serial.println("Tag already exists");
                return false;
            }
     int userCount = getUserTagCountFromEEPROM();
        if (userCount < MAX_USER_TAGS) {
            saveFixedStringToEEPROM(USER_TAGS_START_ADDR + (userCount * USER_TAG_LEN), tag, USER_TAG_LEN);
           //ClearIndexOfStatistics(userCount);
            userCount++;
            saveUserTagCountToEEPROM(userCount);
        }
        return true;
    }
    
    void UserManagementClass::addCard(){
       if (_server.hasArg("plain")) {
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, _server.arg("plain"));
            if (error) {
                _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
                return;
            }
            String card = doc["card"].as<String>();
        if (card.length() > USER_TAG_LEN) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Tag length shuld not exceed 11 digits\"}");
            return;
        } else {
                 // Serial.println(readStringFromEEPROM(REMOVE_CARD_ADDR, USER_TAG_LEN));

            // Pad with leading zeros if tag is shorter than USER_TAG_LEN
            String temp = "";
            for(int i = card.length() ; i < USER_TAG_LEN ; i++){
                temp += "0";
            }
            card = temp + card;
            Serial.println(card);
        }
        saveFixedStringToEEPROM(ADD_CARD_ADDR, card, USER_TAG_LEN);
         _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"ADD card added\"}");
        Serial.print("Add card added done.");
     // Serial.println(readStringFromEEPROM(REMOVE_CARD_ADDR, USER_TAG_LEN));
        ESP.restart();



    }
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"tag\":\"11_digits\"}\"}");
}
    void UserManagementClass::removeCard(){
       if (_server.hasArg("plain")) {
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, _server.arg("plain"));
            if (error) {
                _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
                return;
            }
            String card = doc["card"].as<String>();
        if (card.length() > USER_TAG_LEN) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Tag length shuld not exceed 11 digits\"}");
            return;
        } else {
            // Pad with leading zeros if tag is shorter than USER_TAG_LEN
            String temp = "";
            for(int i = card.length() ; i < USER_TAG_LEN ; i++){
                temp += "0";
            }
            card = temp + card;
            Serial.println(card);
        }
        saveFixedStringToEEPROM(REMOVE_CARD_ADDR, card, USER_TAG_LEN);
         _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Remove card added\"}");
        Serial.print("Remove card added done\"}");
        
        ESP.restart();


    }
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"tag\":\"11_digits\"}\"}");

}
    
    void UserManagementClass::handleAddUserTag() {
        if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String tag = doc["tag"].as<String>();
        if (tag.length() > USER_TAG_LEN) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Tag length shuld not exceed 11 digits\"}");
            return;
        } else {
            // Pad with leading zeros if tag is shorter than USER_TAG_LEN
            String temp = "";
            for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
                temp += "0";
            }
            tag = temp + tag;
            Serial.println(tag);
        }

        if (findUserTagAddress(tag) != -1) {
            _server.send(409, "application/json", "{\"status\":\"error\",\"message\":\"Tag already exists\"}");
            return;
        }

        if(storeTag(tag)){

        // int userCount = getUserTagCountFromEEPROM();
        // if (userCount < MAX_USER_TAGS) {
        //     saveStringToEEPROM(USER_TAGS_START_ADDR + (userCount * USER_TAG_LEN), tag, USER_TAG_LEN);
        //     userCount++;
        //     saveUserTagCountToEEPROM(userCount);

            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"User tag added\"}");
            // Serial.print("User tag added: ");
            // Serial.println(tag);
            // Serial.print("Current user tag count: ");
            // Serial.println(userCount);
            return;
        } else {
            _server.send(507, "application/json", "{\"status\":\"error\",\"message\":\"No empty slots for user tags (max 300) or max tags reached\"}");
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"tag\":\"11_digits\"}\"}");
}

bool UserManagementClass::DeleteTag(String tag) {
        String temp = "";
        for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
            temp += "0";
        }
        tag = temp + tag;
        Serial.println(tag);


        int tagAddr = findUserTagAddress(tag);
                Serial.println(tagAddr);
                if (tagAddr == -1) {
            Serial.println("Tag not found");
            return false;
        }

        if (tagAddr != -1) {
            int Users = getUserTagCountFromEEPROM();
            //int index = (tagAddr - USER_TAGS_START_ADDR) / USER_TAG_LEN;
            
            // Shift subsequent tags to fill the gap
            for (int i = tagAddr; i < Users - 1; i++) {
                String nextTag = readStringFromEEPROM(USER_TAGS_START_ADDR + ((i + 1) * USER_TAG_LEN), USER_TAG_LEN);
                saveFixedStringToEEPROM(USER_TAGS_START_ADDR + (i * USER_TAG_LEN), nextTag, USER_TAG_LEN);
                //int next_count=GetStatistics(i+1);
                //UpdateStatistics(i,next_count);
        
            }
          

            Users--;
            saveUserTagCountToEEPROM(Users);
            return true;
        } else {
            return false;
        }


}
void UserManagementClass::handleDeleteUserTag() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String tag = doc["tag"].as<String>();
        if (tag.length() > USER_TAG_LEN) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Tag length shuld not exceed 11 digits\"}");
            return;
        }
       if( DeleteTag(tag)){
//         // Pad with leading zeros if tag is shorter than USER_TAG_LEN (for consistent lookup)
//         String temp = "";
//         for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
//             temp += "0";
//         }
//         tag = temp + tag;

//         int tagAddr = findUserTagAddress(tag);
//         if (tagAddr != -1) {
//             int Users = getUserTagCountFromEEPROM();
//             int index = (tagAddr - USER_TAGS_START_ADDR) / USER_TAG_LEN;
            
//             // Shift subsequent tags to fill the gap
//             for (int i = index; i < Users - 1; i++) {
//                 String nextTag = readStringFromEEPROM(USER_TAGS_START_ADDR + ((i + 1) * USER_TAG_LEN), USER_TAG_LEN);
//                 saveFixedStringToEEPROM(USER_TAGS_START_ADDR + (i * USER_TAG_LEN), nextTag, USER_TAG_LEN);
//             }
//             // Clear the last slot
//             for (int i = 0; i < USER_TAG_LEN; ++i) {
// #ifdef USE_EXTERNAL_EEPROM
//                 externalEEPROMWriteByte(USER_TAGS_START_ADDR + ((Users - 1) * USER_TAG_LEN) + i, 0xFF);
// #else
//                 _eeprom.write(USER_TAGS_START_ADDR + ((Users - 1) * USER_TAG_LEN) + i, 0xFF);
// #endif
//             }
// #ifndef USE_EXTERNAL_EEPROM
//             _eeprom.commit();
// #endif
//             Users--;
//             saveUserTagCountToEEPROM(Users);

            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"User tag deleted\"}");
            // Serial.print("User tag deleted: ");
            // Serial.println(tag);
            // Serial.print("Current user tag count: ");
            // Serial.println(Users);
            return;
        } else {
            _server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"User tag not found\"}");
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"tag\":\"11_digits\"}\"}");
}
bool UserManagementClass::checkTag(String tag) {
        if (tag.length() > USER_TAG_LEN) {
            Serial.println("Tag must be 11 digits long");
            return false;
        }else{
             String temp = "";
        for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
            temp += "0";
        }
        tag = temp + tag;

        if (findUserTagAddress(tag) != -1) {
            Serial.print("User tag found: ");
            Serial.println(tag);
            return true;
        }
   Serial.print("User tag not found: ");
            Serial.println(tag);
            return false;




        }


}
void UserManagementClass::handleCheckUserTag() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String tag = doc["tag"].as<String>();
        if (tag.length() > USER_TAG_LEN) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Tag must be 11 digits long\"}");
            return;
        }
        // Pad with leading zeros if tag is shorter than USER_TAG_LEN (for consistent lookup)
        String temp = "";
        for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
            temp += "0";
        }
        tag = temp + tag;

        if (findUserTagAddress(tag) != -1) {
            _server.send(200, "application/json", "{\"status\":\"success\",\"found\":true,\"message\":\"User tag found\"}");
            Serial.print("User tag found: ");
            Serial.println(tag);
            return;
        } else {
            _server.send(200, "application/json", "{\"status\":\"success\",\"found\":false,\"message\":\"User tag not found\"}");
            Serial.print("User tag not found: ");
            Serial.println(tag);
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"tag\":\"11_digits\"}\"}");
}

void UserManagementClass::handleUseingUserTag() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String tag = doc["tag"].as<String>();
        if (tag.length() > USER_TAG_LEN) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Tag must be 11 digits long\"}");
            return;
        }
        // Pad with leading zeros if tag is shorter than USER_TAG_LEN (for consistent lookup)
        String temp = "";
        for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
            temp += "0";
        }
        tag = temp + tag;
int index = findUserTagAddress(tag);
        
        if (index != -1) {
            Serial.print("User tag found: ");
            Serial.println(tag);
            setRelayPhysicalState(true);
            _server.send(200, "application/json", "{\"status\":\"success\",\"found\":true,\"message\":\"User tag found\"}");
            delay(5000);
            setRelayPhysicalState(false);
            //SetStatistics(index);
            return;
        } else {
            _server.send(200, "application/json", "{\"status\":\"success\",\"found\":false,\"message\":\"User tag not found\"}");
            Serial.print("User tag not found: ");
            Serial.println(tag);
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request body. Expected {\"tag\":\"11_digits\"}\"}");
}

void UserManagementClass::handleGetUserTagCount() {
    String response = "{\"status\":\"success\",\"count\":" + String(getUserTagCountFromEEPROM()) + "}"; // Read live count
    _server.send(200, "application/json", response);
}

void UserManagementClass::handleGettags() {
    int usercount = getUserTagCountFromEEPROM();
    Serial.println(usercount);
    String users = "";
    for(int i = 0; i < usercount; i++){
        int currentTagAddr = USER_TAGS_START_ADDR + (i * USER_TAG_LEN);
        String storedTag = readStringFromEEPROM(currentTagAddr, USER_TAG_LEN);
        storedTag = _trim(storedTag);
        if (!storedTag.isEmpty() && (
#ifdef USE_EXTERNAL_EEPROM
            externalEEPROMReadByte(currentTagAddr) != 0xFF // Check first byte for unwritten state
#else
            _eeprom.read(currentTagAddr) != 0xFF // Check first byte for unwritten state
#endif
            )) {
            users += storedTag;
            if (i < usercount - 1) {
                users += ",";
            }
        }
    }
    String response = "{\"status\":\"success\",\"users\":\"" + users + "\"}";
    Serial.println(response);
    _server.send(200, "application/json", response);
}

String UserManagementClass::_trim(String& str){
    int count = 0;
    for(int i = 0; i < str.length(); i++){
        if(str[i] == '0')
            count++;
        else
            break;
    }
    Serial.println(count);
    return str.substring(count);
}

/* please check */
// --- UserManagementClass Implementations ---
// #ifdef USE_EXTERNAL_EEPROM
// UserPStatisric::UserPStatisric(WebServer& serverRef, int relayPin)
//     : UserManagementClass(serverRef, relayPin) {
// #else
// UserManagementClass::UserManagementClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef)
//     : MainControlClass(serverRef, relayPin, eepromRef) {
// #endif
// }



