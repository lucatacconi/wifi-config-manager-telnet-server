#include "Arduino.h"
#include "EEPROM.h"
#include "ArduinoUniqueID.h"
#include "WiFiNINA.h"
#include "WifiCfgMgrTelnetServer.h"

int const _SSIDStartPosition = 0;
int const _passwordStartPosition = 33;
int const _SSIDMaxLength = 32;
int const _passwordMaxLength = 64;

String _apSSID;
IPAddress _apAddress;

WiFiServer telnetServer(23);


void(* reloadDevice) (void) = 0;

WifiCfgMgrTelnetServer::WifiCfgMgrTelnetServer( int p_resetButtonPin ) : telnetServer(23)
{
    _resetButtonPin = p_resetButtonPin;

    pinMode(_resetButtonPin, INPUT);

    _apSSID = "WifiCfgManager";
    _apAddress = IPAddress(192, 168, 1, 1);
}

//Configuration getter
int WifiCfgMgrTelnetServer::getResetButtonPinValue(){
    return _resetButtonPin;
}

//====================================================================
//DEVICE management

bool WifiCfgMgrTelnetServer::deviceStartup(){

    bool result = true;
    int numSsid;
    String SSID;
    String wifiPassword;
    int numAttempt;


    Serial.println("Starting device");


    //Wifi module check
    result = checkWifiModule();
    if(result){
        Serial.println("WiFi module present and working");
    }else{
        Serial.println("ERROR - WiFi module error");
        return false;
    }


    //Check if an SSID and password have already been configured
    bool SSIDConfigPresent = checkDataPresenceInEEPROM(_SSIDStartPosition);
    bool wifiPasswordConfigPresent = checkDataPresenceInEEPROM(_passwordStartPosition);

    if(SSIDConfigPresent && wifiPasswordConfigPresent){

        Serial.println("Device configured. Accessing selected WIFI.");


    }else{
        //Enable Telnet configuration server

        Serial.println("Device not configured yet. Activating Telnet configuration server.");


        //Configuring password equal to device Unique ID
        String APpass = readDeviceUniqueID();

        //Reading wifi in range
        numSsid = scanNetworksLst();

        //Starting Telnet server
        bool aPRunning = false;
        numAttempt = 0;

        //At least performed once
        //Keep trying every 10 seconds
        while(!aPRunning){

            numAttempt++;
            if(numAttempt > 1) delay (10000);

            aPRunning = startTelnetServer(APpass);

            if(aPRunning){
                Serial.println("Access point stared and running");
                Serial.println("Use Ip address: > " + String(_apAddress[0]) + "." + String(_apAddress[1]) + "." + String(_apAddress[2]) + "." + String(_apAddress[3]) + " <");
                Serial.println("Use password: > " + APpass + " <");
                break;
            }else{
                Serial.println("ERROR - Access point startup error");
            }
        }

        //Command reading and their execution
        telnetServerMsgManage();
    }

    return true;
}

String WifiCfgMgrTelnetServer::readDeviceUniqueID(){

    String uniqueIDStr = "";

    for(size_t i = 0; i < UniqueIDsize; i++){
        uniqueIDStr += String(UniqueID[i], HEX);
    }

    return uniqueIDStr;
}

//END DEVICE management


//====================================================================
//TELNET server management

// 0: WL_IDLE_STATUS
// 1: WL_NO_SSID_AVAIL
// 2: WL_SCAN_COMPLETED
// 3: WL_CONNECTED
// 4: WL_CONNECT_FAILED
// 5: WL_CONNECTION_LOST
// 6: WL_DISCONNECFTED
// 7: WL_AP_LISTENING
// 8: WL_AP_CONNECTED

bool WifiCfgMgrTelnetServer::startTelnetServer(String aPPassword){

    char charAPSSIDconfigured[32];
    char charAPPasswordConfigured[64];

    _apSSID.toCharArray(charAPSSIDconfigured, 32);
    aPPassword.toCharArray(charAPPasswordConfigured, 64);

    WiFi.config(_apAddress);

    WiFi.beginAP(charAPSSIDconfigured, charAPPasswordConfigured);
    delay(10000);

    telnetServer.begin();

    if ( WiFi.status() != WL_AP_LISTENING) { //Access point created correctly
        return false;
    }

    return true;
}

bool WifiCfgMgrTelnetServer::telnetServerMsgManage(){

    bool firstAccess = true;
    bool commnadPhase = true; //Indicates whether the configurator is in the state of receiving the command or message. Set to true at the beginning to recive first command
    String commandID = "1";

     String welcomeMessage = "\r\nCommand list:\r\n";
     welcomeMessage += "1 - Show help\r\n";
     welcomeMessage += "2 - Show UniqueID\r\n";
     welcomeMessage += "3 - Show WiFi networks in range\r\n";
     welcomeMessage += "4 - Set target Wifi\r\n";
     welcomeMessage += "5 - Set wifi password\r\n";
     welcomeMessage += "0 - Exit config mode and reboot\r\n";
     welcomeMessage += "ESC - Back to selection\r\n";

    char incomingChar;
    String message = "";
    String displayMessage = "";

    while(true){
        if(WiFi.status() == WL_AP_CONNECTED){   //Entity connected IP Address
            WiFiClient client = telnetServer.available();

            if (client) { //Client connected to telnet server

                client.flush(); //Clean read buffer.. Probably the c statement is useless and could even be removed.

                if(firstAccess){

                    displayMessage = "Client connected to Telnet Server";

                    Serial.println(displayMessage);
                    client.println("\r\n" + displayMessage);
                    client.println(welcomeMessage);
                    firstAccess = false;
                }

                while(client.connected()) {
                    while( client.available() ) { //There is something to read on buffer

                        incomingChar = client.read();
                        if (' ' <= incomingChar && incomingChar <= '~' && isPrintable(incomingChar)) message += incomingChar; //Icoming char cleaning

                        if(incomingChar == 0x0d){ //Pressed the ENTER key
                            message.trim();

                            if(commnadPhase){

                                commandID = message.charAt(message.length() - 1);
                                message = "";

                                Serial.println("Command to execute: " + commandID);

                                if(commandID == "1"){
                                    client.println(welcomeMessage);
                                }else if(commandID == "2"){
                                    client.println("\r\nDevice UniqueID: " + readDeviceUniqueID());
                                }else if(commandID == "3"){
                                    client.println("\r\nNetwork List:");

                                   for (int thisNet = 0; thisNet < 20; thisNet++) {
                                        if(_aSSID[thisNet] != ""){
                                            client.println(String(thisNet) + ": "+_aSSID[thisNet]);
                                        }
                                   }
                                }else if(commandID == "4"){
                                    client.println("\r\nSelect target Wifi for connection (1-20) and press Enter");
                                    client.println("Network List:");

                                    for (int thisNet = 0; thisNet < 20; thisNet++) {
                                        if(_aSSID[thisNet] != ""){
                                            client.println(String(thisNet) + ": "+_aSSID[thisNet]);
                                        }
                                    }

                                    commnadPhase = false;

                                }else if(commandID == "5"){
                                    commnadPhase = false;
                                    client.println("Write wifi password and press Enter");
                                }else if(commandID == "0"){
                                    commnadPhase = false;
                                    client.println("Switch off config mode and device reboot");
                                    //@TODO reboot
                                    message = "";
                                    commnadPhase = true;
                                    firstAccess = true;
                                    commandID = "1";                                    
                                    client.stop();
                                }else{
                                    displayMessage = "Client connected to Telnet Server";
                                    Serial.println("Command not recognized");
                                }

                            }else{

                                if(commandID == "4"){
                                    Serial.println("Param for command 4: "+message);
                                    client.println("Target wifi selected");
                                }else if(commandID == "5"){
                                    Serial.println("Param for command 5: "+message);
                                    client.println("Wifi password saved");
                                }else{
                                    Serial.println("Param input not recognized");
                                }

                                commnadPhase = true;
                                message = "";
                                commandID = "1";

                            }

                            client.println("");
                        }

                        if(incomingChar == 0x1b){ //Pressed the ESC key
                             Serial.println("Pressed the ESC key");
                             message = "";
                             commnadPhase = true;
                             firstAccess = true;
                             commandID = "1";
                             client.stop();
                        }
                    }
                }
            }
        }
    }

    return true;
}





//END TELNET server management

//====================================================================
//CONNECTION management

bool WifiCfgMgrTelnetServer::checkWifiModule(){

    //Check WiFi module presence
    if (WiFi.status() == WL_NO_MODULE){
        return false;
    }

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        return false;
    }

    return true;
}

int WifiCfgMgrTelnetServer::scanNetworksLst(){

    // Scan for nearby networks:
    int numSsid = WiFi.scanNetworks();
    delay(10000);

    if(numSsid > 0){
        for (int thisNet = 0; thisNet < numSsid; thisNet++) {
            _aSSID[thisNet]= WiFi.SSID(thisNet);
        }
    }

    return numSsid;
}

bool WifiCfgMgrTelnetServer::wifiConnect(String SSIDconfigured, String wifiPasswordConfigured){

    char charSSIDconfigured[32];
    char charWifiPasswordConfigured[64];

    SSIDconfigured.toCharArray(charSSIDconfigured, 32);
    wifiPasswordConfigured.toCharArray(charWifiPasswordConfigured, 64);

    WiFi.begin(charSSIDconfigured, charWifiPasswordConfigured);
    delay(5000); //Time needed for connection

    if ( WiFi.status() != WL_CONNECTED) {
        return false;
    }
    return true;
}



//END CONNECTION management

//====================================================================
//EEPROM management

String WifiCfgMgrTelnetServer::readEEPROM(int startPosition, int readLength) {

    String result = "";
    int eepromSize = 255;

    if(startPosition >= 0 || startPosition <= eepromSize){
        for (int position = 0; position < readLength; position++){
            char char2Read = EEPROM.read(startPosition + position);
            if(char2Read == '\0') break;
            result += String(char2Read);
        }
    }

    return result;
}

bool WifiCfgMgrTelnetServer::checkDataPresenceInEEPROM(int EEPROMPosition) {

    float testedAddress;
    EEPROM.get( EEPROMPosition, testedAddress );

    if (isnan(testedAddress)) return false;
    else{
        if(String(testedAddress) == '\0') return false;
    }
    return true;
}

bool WifiCfgMgrTelnetServer::writeString2EEPROM(int startPosition, String string2Write) {

    int eepromSize = 10;

    if(startPosition < 0 || startPosition >= eepromSize) return false;
    int startWrt = startPosition;

    int writeLength = string2Write.length();
    if(writeLength > eepromSize) writeLength = eepromSize;

    int strPosition;
    int wrtPosition;
    for (strPosition = 0; strPosition < writeLength; strPosition++){

        char char2Write = string2Write[strPosition];
        wrtPosition = startWrt + strPosition;
        EEPROM.update(wrtPosition, char2Write);
    }

    wrtPosition = startWrt + strPosition;
    if(wrtPosition < eepromSize){
        EEPROM.update(wrtPosition, '\0');
    }

    return true;
}

String WifiCfgMgrTelnetServer::readSSIDFromEEPROM() {
    String result = readEEPROM(_SSIDStartPosition, _SSIDMaxLength);
    return result;
}
String WifiCfgMgrTelnetServer::readPasswordFromEEPROM() {

    String result = readEEPROM(_passwordStartPosition, _passwordMaxLength);
    return result;
}


bool WifiCfgMgrTelnetServer::writeSSID2EEPROM(String SSID) {

    if(SSID.length() > _SSIDMaxLength) return false;

    bool result = writeString2EEPROM(_SSIDStartPosition, SSID);
    return result;
}
bool WifiCfgMgrTelnetServer::writePassword2EEPROM(String password) {

    if(password.length() > _passwordMaxLength) return false;

    bool result = writeString2EEPROM(_passwordStartPosition, password);
    return result;
}

bool WifiCfgMgrTelnetServer::resetSSID2EEPROM() {

    bool result = writeString2EEPROM(_SSIDStartPosition, "\0");
    return result;
}
bool WifiCfgMgrTelnetServer::resetPassword2EEPROM() {

    bool result = writeString2EEPROM(_passwordStartPosition, "\0");
    return result;
}

//END - EEPROM management
