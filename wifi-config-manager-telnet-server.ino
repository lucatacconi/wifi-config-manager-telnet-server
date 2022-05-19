#include "Arduino.h"
#include "WifiCfgMgrTelnetServer.h"

const int RESET_BUTTON_PIN = 3;

WifiCfgMgrTelnetServer WifiCfgMgrTelnetServer( RESET_BUTTON_PIN );



void setup() {

    Serial.begin(9600);


    WifiCfgMgrTelnetServer.deviceStartup();

}

void loop() {


}
