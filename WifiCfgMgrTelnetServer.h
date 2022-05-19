#ifndef WifiCfgMgrTelnetServer_h
#define WifiCfgMgrTelnetServer_h

#include "WiFiNINA.h"

class WifiCfgMgrTelnetServer{

    public:
                WifiCfgMgrTelnetServer( int p_resetButtonPin );

        int     getResetButtonPinValue();   

        //EEPROM management
        String  readPinFromEEPROM();
        String  readSSIDFromEEPROM();
        String  readPasswordFromEEPROM();

        //DEVICE management
        String  readDeviceUniqueID();
        bool    deviceStartup();

    private:

        int     _resetButtonPin;
        
        String  _apSSID;
        IPAddress _apAddress;
        WiFiServer telnetServer;

        String  _aSSID[20];        

        //DEVICE management
        void    reloadDevice();

        //CONNECTION management
        bool    checkWifiModule();
        int     scanNetworksLst();
        bool    wifiConnect(String SSIDconfigured, String wifiPasswordConfigured);    

        //TELNET server
        bool    startTelnetServer(String aPPassword);
        bool    telnetServerMsgManage();            

        //EEPROM management
        String  readEEPROM(int startPosition, int readLength);
        bool    checkDataPresenceInEEPROM(int EEPROMPosition);
        bool    writeString2EEPROM(int startPosition, String string2Write);
        bool    writePin2EEPROM(String pin);
        bool    writeSSID2EEPROM(String SSID);
        bool    writePassword2EEPROM(String password);
        bool    resetPin2EEPROM();
        bool    resetSSID2EEPROM();
        bool    resetPassword2EEPROM();                        

};

#endif
