/**
 * @package Portable WiFi Repeater
 * @author WizLab.it
 * @version 20240228.066
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <CRC32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include "ConfigWebServerPages.h"


/**
 * System Parameters
 */

//Low-battery
#define LOWBATTERY_PIN            A0
#define LOWBATTERY_COMPENSATION   2.10

//LED
// - [runtime] 3 mid-speed blinks every 15 seconds: battery low
// - [boot] 10 fast blinks: configuration deleted
// - [config] 10 fast blinks: configuration saved
// - [config] 20 fast blinks: identify device
// - [repeater + BTN1 pressed] 3 fast + 0.5 seconds delay + N slow blinks: STA signal quality (N = signal quality, 0 to 5)
#define LED_PIN 2 //GPIO2, D4
#define LED_ON LOW //Led is on when the pin is low
#define LED_OFF HIGH //Led is off when the pin is high

//OLED
#define OLED_ADDRESS 0x3C //OLED I2C Address
#define OLED_WIDTH 128 //OLED display width, in pixels
#define OLED_HEIGHT 32 //OLED display height, in pixels

//Buttons
#define BTN1_PIN 14 //GPIO14, D5

//Repeater
#define REPEATER_MODE_INIT 0
#define REPEATER_MODE_CONFIG 1
#define REPEATER_MODE_REPEATER 2
#define REPEATER_NAPT 1000
#define REPEATER_NAPT_PORT 10
#define REPEATER_CONFIG_SSID "PortWiFiRepeater-"

//Other generic constants
#define EEPROM_CONFIG_ADDRESS 0
#define SSID_LENGTH 40
#define PSK_LENGTH 30


/**
 * Structs
 */

//Repeater configuration
typedef struct {
  char staSSID[SSID_LENGTH + 1];
  char staPSK[PSK_LENGTH + 1];
  char apSSID[SSID_LENGTH + 4 + 1];
  char apPSK[PSK_LENGTH + 1];
} __RepeaterConfiguration;

//Repeater configuration package
typedef struct {
  __RepeaterConfiguration config;
  uint32_t crc;
} __RepeaterConfigurationPackage;

//Timers
struct {
  unsigned long batteryCheck = 0;
  unsigned long connectedStationsCheck = 0;
  unsigned long staStatus = 0;
} __Timers;


/**
 * Global Variables
 */

__RepeaterConfigurationPackage REPEATER_CONFIG;
uint8_t REPEATER_MODE = REPEATER_MODE_INIT;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
WiFiEventHandler apStationConnectedHandler;
WiFiEventHandler apStationDisconnectedHandler;
ESP8266WebServer configWebServer(80);
DNSServer configDnsServer;


//===================================================================================


/**
 * Setup
 */
void setup() {
  //Serial monitor
  Serial.begin(115200);
  Serial.println(F("\n\n\n\n\n"));
  Serial.println(F("************************************************************************"));
  Serial.println(F("***           ~  Portable Wi-Fi Repeater - by WizLab.it  ~           ***"));
  Serial.println(F("************************************************************************"));
  Serial.println(F("[~~~~~] Setup:"));

  //GPIO pins configuration
  pinMode(LOWBATTERY_PIN, INPUT);
  pinMode(BTN1_PIN, INPUT_PULLUP);

  //Led
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

  //OLED
  if(display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F(" [+] OLED activated"));
  } else {
    Serial.println(F(" [-] OLED init failed"));
  }

  //Splash screen
  display.clearDisplay();
  delay(250);
  display.cp437(true); // Use full 256 char 'Code Page 437'
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(43, 0);
  display.print("Portable");
  display.setCursor(24, 10);
  display.print("Wi-Fi Repeater");
  display.setCursor(30, 24);
  display.print("by WizLab.it");
  display.display();
  delay(2000);

  //After splash screen: clear display and check battery status
  display.clearDisplay();
  checkBattery();
  display.display();

  //Try to load a repeater configuration from EEPROM
  Serial.print(F(" [+] Read repeater configuration from EEPROM: "));
  bool hasValidConfig = repeaterConfigLoad();
  if(hasValidConfig) {
    REPEATER_MODE = REPEATER_MODE_REPEATER;
    Serial.println(F("OK"));
  } else {
    REPEATER_MODE = REPEATER_MODE_CONFIG;
    Serial.println(F("no valid configuration found."));
  }

  //Run the repeater mode
  if(REPEATER_MODE == REPEATER_MODE_CONFIG) {
    repeaterModeConfigRun();
  } else if(REPEATER_MODE == REPEATER_MODE_REPEATER) {
    repeaterModeRepeaterRun();
  }

  //Setup completed
  Serial.println(F("[~~~~~] Setup complete."));
  Serial.println(F("[~~~~~] Running:"));
}


/**
 * Loop
 */
void loop() {
  //Check battery
  checkBattery();

  //Check connected stations
  checkConnectedStations();

  //Loop actions when in Config mode
  if(REPEATER_MODE == REPEATER_MODE_CONFIG) {
    configDnsServer.processNextRequest();
    configWebServer.handleClient();
  }

  //Loop actions when in Repeater mode
  if(REPEATER_MODE == REPEATER_MODE_REPEATER) {
    //Check STA status
    checkStatusSTA();
  }
}


//===================================================================================


/**
 * Load repeater configuration from EEPROM
 */
bool repeaterConfigLoad() {
  //Init EEPROM
  EEPROM.begin(sizeof(__RepeaterConfigurationPackage));

  //Check if to reset configuration
  if(digitalRead(BTN1_PIN) == LOW) {
    repeaterConfigClear(true);
    Serial.print(F("\n [+] Configuration reset"));
    oledPrintLine(1, F("  * Configuration *"));
    oledPrintLine(2, F("  *     reset     *"));
    blinkLed(10, 1);
    delay(3000);
    ESP.restart();
  } else {
    //Read configuration and calculate CRC
    EEPROM.get(EEPROM_CONFIG_ADDRESS, REPEATER_CONFIG);
    uint32_t configCrc = CRC32::calculate((const uint8_t *)&REPEATER_CONFIG.config, sizeof(__RepeaterConfiguration));

    //Check if configuration is valid
    if(REPEATER_CONFIG.crc == configCrc) {
      return true;
    }

    //If config is not valid, then clear it
    repeaterConfigClear(false);
  }

  return false;
}

bool repeaterConfigSave() {
  //Validate new configuration CRC
  uint32_t configCrc = CRC32::calculate((const uint8_t *)&REPEATER_CONFIG.config, sizeof(__RepeaterConfiguration));
  if(REPEATER_CONFIG.crc == configCrc) {
    EEPROM.put(EEPROM_CONFIG_ADDRESS, REPEATER_CONFIG);
    EEPROM.commit();
    return true;
  }
  return false;
}

/**
 * Clear repeater configuration
 */
void repeaterConfigClear(bool resetEeprom) {
  memset((void *)&REPEATER_CONFIG, 0x00, sizeof(__RepeaterConfigurationPackage));

  if(resetEeprom) {
    EEPROM.put(EEPROM_CONFIG_ADDRESS, REPEATER_CONFIG);
    EEPROM.commit();
  }
}

/**
 * Repeater mode
 */
void repeaterModeRepeaterRun() {
  Serial.println(F(" [+] Activating repeater mode..."));

  //Connect to Access Point that has to be repeated
  WiFi.mode(WIFI_STA);
  WiFi.begin(REPEATER_CONFIG.config.staSSID, REPEATER_CONFIG.config.staPSK);
  Serial.print(F("    [+] connecting to network to be repeated: "));
  oledPrintLine(2, "    Connecting...");
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(500);
  }
  Serial.printf(" OK! (IP: %s)\n", WiFi.localIP().toString().c_str());
  oledPrintLine(2, "     Connected!");

  //Configure repeater network
  Serial.print(F("    [+] configuring repeater network: "));
  IPAddress localIp = IPAddress(10, random(0, 255), random(0, 255), 1);
  WiFi.softAPConfig(localIp, localIp, IPAddress(255, 255, 255, 0));
  auto &dhcpserver = WiFi.softAPDhcpServer();
  dhcpserver.setDns(WiFi.dnsIP(0));
  Serial.printf("OK! (IP: %s)\n", WiFi.softAPIP().toString().c_str());

  //Activate repeater network
  Serial.print(F("    [+] activate repeater network: "));
  WiFi.softAP(REPEATER_CONFIG.config.apSSID, REPEATER_CONFIG.config.apPSK);
  Serial.printf("OK! (SSID: %s)\n", WiFi.softAPSSID().c_str());

  //Activate NAPT
  Serial.print(F("    [+] activate NAPT: "));
  err_t naptStatus = ip_napt_init(REPEATER_NAPT, REPEATER_NAPT_PORT);
  if(naptStatus == ERR_OK) {
    naptStatus = ip_napt_enable_no(SOFTAP_IF, 1);
  }
  if(naptStatus == ERR_OK) {
    Serial.println(F("OK!\n [+] Repeater mode activated successfully!"));
    Serial.printf("    [i] repeated network: %s\n", REPEATER_CONFIG.config.staSSID);
    Serial.printf("    [i] repeater network: %s\n", REPEATER_CONFIG.config.apSSID);
    oledPrintLine(1, REPEATER_CONFIG.config.staSSID);
    oledPrintLine(2, "        <===>");
    oledPrintLine(3, REPEATER_CONFIG.config.apSSID);

    //Connection handlers
    apStationConnectedHandler = WiFi.onSoftAPModeStationConnected(&apOnStationConnected);
    apStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&apOnStationDisconnected);
  } else {
    Serial.println(F(" Failed."));
  }
}

/**
 * Config mode
 */
void repeaterModeConfigRun() {
  Serial.println(F(" [+] Activating configuration mode..."));

  //Activate access point
  Serial.print(F("    [+] activate config WiFi network: "));
  String configModeSSID = REPEATER_CONFIG_SSID + String(random(1000, 9999));
  IPAddress localIp = IPAddress(192, 168, 1, 1);
  WiFi.softAPConfig(localIp, localIp, IPAddress(255, 255, 255, 0));
  auto &dhcpserver = WiFi.softAPDhcpServer();
  dhcpserver.setDns(localIp);
  WiFi.softAP(configModeSSID);
  Serial.printf("OK! (SSID: %s)\n", WiFi.softAPSSID().c_str());

  //Connection handlers
  apStationConnectedHandler = WiFi.onSoftAPModeStationConnected(&apOnStationConnected);
  apStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&apOnStationDisconnected);

  //Activate configuration web server
  Serial.print(F("    [+] activate config web server: "));
  configWebServerEngine();
  configWebServer.begin();
  char configURL[30];
  sprintf(configURL, "http://%s/", WiFi.softAPIP().toString().c_str());
  Serial.printf("OK! (URL: %s)\n", configURL);

  //Activate DNS for local resolution to configuration portal
  Serial.print(F("    [+] activate config DNS server: "));
  configDnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println(F("OK!"));

  //Print configuration portal info on OLED
  oledPrintLine(1, F(" *** CONFIG MODE ***"));
  oledPrintLine(2, configModeSSID);
  oledPrintLine(3, String(configURL));
}

/**
 * Web Server Engine (configure pages)
 */
void configWebServerEngine() {
  //Configuration portal
  configWebServer.on("/", configWebServerRoot);

  //Captive portal URLs
  configWebServer.on("/portal", configWebServerRoot);
  configWebServer.on("/portal/index.php", configWebServerRoot);
  configWebServer.on("/connecttest.txt", configWebServerRoot);
  configWebServer.on("/ncsi.txt", configWebServerRoot);
  configWebServer.on("/generate_204", configWebServerRoot);

  //Identify device
  configWebServer.on("/identify", []() {
    Serial.println(F("Config Web Server: device identified"));
    blinkLed(20, 1);
    configWebServer.send(200, "text/html", F("OK"));
  });

  //Reboot device
  configWebServer.on("/reboot", []() {
    ESP.restart();
  });

  //Page not found
  configWebServer.onNotFound([]() {
    configWebServer.send(404, "text/plain", "Page Not Found (" + configWebServer.uri() + ")");
  });
}

/**
 * Web Server Page: configuration portal
 */
void configWebServerRoot() {

  //If POST, then check info and save configuration
  if(configWebServer.method() == HTTP_POST) {

    //Save Repeated Network
    configWebServer.arg("staSSID").toCharArray(REPEATER_CONFIG.config.staSSID, SSID_LENGTH);
    configWebServer.arg("staPSK").toCharArray(REPEATER_CONFIG.config.staPSK, PSK_LENGTH);

    //Save Repeater Network
    configWebServer.arg("apSSID").toCharArray(REPEATER_CONFIG.config.apSSID, SSID_LENGTH);
    configWebServer.arg("apPSK").toCharArray(REPEATER_CONFIG.config.apPSK, PSK_LENGTH);

    //If Repeater SSID is empty, then create one based on the Repeated Network
    if(strncmp(REPEATER_CONFIG.config.apSSID, "", SSID_LENGTH) == 0) {
      sprintf(REPEATER_CONFIG.config.apSSID, "%s_ext", REPEATER_CONFIG.config.staSSID, SSID_LENGTH);
    }

    //If Repeater PSK is empty, then use the same PSK of the Repeated Network
    if(strncmp(REPEATER_CONFIG.config.apPSK, "", PSK_LENGTH) == 0) {
      strncpy(REPEATER_CONFIG.config.apPSK, REPEATER_CONFIG.config.staPSK, PSK_LENGTH);
    }

    //Calculate configuration CRC
    REPEATER_CONFIG.crc = CRC32::calculate((const uint8_t *)&REPEATER_CONFIG.config, sizeof(REPEATER_CONFIG.config));

    //Save new configuration on EEPROM
    if(repeaterConfigSave()) {
      Serial.println(F(" [+] New configuration saved on EEPROM"));
      oledPrintLine(3, F("   * CONFIG OK! *"));
      blinkLed(10, 1);
    } else {
      Serial.println(F(" [-] New configuration not saved on EEPROM: invalid configuration"));
      oledPrintLine(3, F("   * CONFIG KO. *"));
    }

    //Output HTML
    String html = String(configurationPortalHtmlLayout);
    html.replace("%BODY_CONTENT%", configurationPortalHtmlConfigSaved);
    configWebServer.send(200, "text/html", html);

  } else {

    //Build available Wi-Fi Networks list
    String availableNetworks = "";
    int numberOfNetworks = WiFi.scanNetworks();
    for(int i=0; i<numberOfNetworks; i++) {
      availableNetworks += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + " (Quality: " + String(getSignalLevelFromRSSI(WiFi.RSSI(i))) + ")</option>";
    }

    //Output HTML
    String html = String(configurationPortalHtmlLayout);
    html.replace("%BODY_CONTENT%", configurationPortalHtmlConfigForm);
    html.replace("%AVAILABLE_NETWORKS%", availableNetworks);
    configWebServer.send(200, "text/html", html);

  }
}

/**
 * Check battery status
 */
void checkBattery() {
  //Timer check
  if(__Timers.batteryCheck > millis()) return;
  __Timers.batteryCheck = millis() + 60000;

  //Read battery voltage
  long batteryVoltageRaw = analogRead(LOWBATTERY_PIN);
  float batteryVoltageVolt = (map(batteryVoltageRaw, 10, 1014, 0, 3000) / 1000.0) * LOWBATTERY_COMPENSATION; //Map is 10-1014 due to A/D error. Value is doubled because of the voltage divider, plus compensation due to voltage divider

  //Battery level on serial monitor
  Serial.printf(" [i] Battery voltage: %0.2f V (Raw: %d)\n", batteryVoltageVolt, batteryVoltageRaw);

  //Battery level on OLED
  display.fillRect((OLED_WIDTH - 20), 0, 20, 8, BLACK);
  display.drawRoundRect((OLED_WIDTH - 20), 0, 18, 7, 2, WHITE);
  display.fillRoundRect((OLED_WIDTH - 3), 2, 3, 3, 2, WHITE);
  if(batteryVoltageVolt > 3.60) display.fillRect((OLED_WIDTH - 18), 2, 4, 3, WHITE);
  if(batteryVoltageVolt > 3.75) display.fillRect((OLED_WIDTH - 13), 2, 4, 3, WHITE);
  if(batteryVoltageVolt > 3.90) display.fillRect((OLED_WIDTH - 8), 2, 4, 3, WHITE);
  display.display();

  //Battery level on led (3 mid-speed blinks if low-battery)
  if(batteryVoltageVolt < 3.55) blinkLed(3, 2);
}

/**
 * Check connected stations
 */
void checkConnectedStations() {
  //Timer check
  if(__Timers.connectedStationsCheck > millis()) return;
  __Timers.connectedStationsCheck = millis() + 5000;

  //Print connected stations
  Serial.printf(" [i] Connected stations: %d\n", WiFi.softAPgetStationNum());
  display.fillRect((OLED_WIDTH - 60), 0, 36, 8, BLACK);
  display.setTextSize(1);
  display.setCursor((OLED_WIDTH - 60), 0);
  display.printf("STA:%d", WiFi.softAPgetStationNum());
  display.display();
}

/**
 * Check STA status
 */
void checkStatusSTA() {
  //Timer check
  if(__Timers.staStatus > millis()) return;
  __Timers.staStatus = millis() + 1000;

  //Get signal level based on RSSI
  int8_t signalLevel = getSignalLevelFromRSSI(WiFi.RSSI());

  //Print STA connection status
  if(WiFi.status() == WL_CONNECTED) {
    oledPrintLine(2, "        <===>");

    //If BTN1 is pressed, then show RSSI with led blinks
    if(digitalRead(BTN1_PIN) == LOW) {
      blinkLed(2, 1); //Two fast blinks: identify RSSI-on-led
      delay(500);
      blinkLed(signalLevel, 3); //Slow blinks (0 to 5) based on signal level
      delay(3000);
    }
  } else {
    oledPrintLine(2, "   Reconnecting...");
  }

  //Print STA RSSI
  display.fillRect(0, 0, 47, 8, BLACK);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("WiFi");
  for(int8_t i=1; i!=6; i++) {
    if(signalLevel < i) {
      display.fillRect((22 + (4 * i)), 5, 3, 1, WHITE);
    } else {
      display.fillRect((22 + (4 * i)), 1, 3, 5, WHITE);
    }
  }

  display.display();
}

/**
 * Blink led
 * @param speed: 1:fast; 2:medium; 3:slow
 */
void blinkLed(uint8_t numberOfBlinks, uint8_t speed) {
  if(speed < 1) speed = 1;
  if(speed > 3) speed = 3;
  speed = speed * 2 - 1;
  for(uint8_t i=numberOfBlinks; i!=0; i--) {
    digitalWrite(LED_PIN, LED_ON);
    delay(10 * speed);
    digitalWrite(LED_PIN, LED_OFF);
    delay(100 * speed);
  }
}

/**
 * Print message on OLED
 */
void oledPrintLine(uint8_t line, String message) {
  if(line < 1) return;
  if(line > 3) return;
  display.fillRect(0, (line * 8), OLED_WIDTH, 8, BLACK);
  display.setTextSize(1);
  display.setCursor(0, (line * 8));
  display.print(message);
  display.display();
}

/**
 * Handlers when station connects/disconnects to/from the Access Point
 */
void apOnStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
  Serial.printf(" [i] WiFi Access Point: Station connected: %x:%x:%x:%x:%x:%x (count: %d)\n", evt.mac[0], evt.mac[1], evt.mac[2], evt.mac[3], evt.mac[4], evt.mac[5], WiFi.softAPgetStationNum());
}
void apOnStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt) {
  Serial.printf(" [i] WiFi Access Point: Station disconnected: %x:%x:%x:%x:%x:%x (count: %d)\n", evt.mac[0], evt.mac[1], evt.mac[2], evt.mac[3], evt.mac[4], evt.mac[5], WiFi.softAPgetStationNum());
}

/**
 * Claculate Wi-Fi signal level from RSSI
 * @return signal level from 0 (worst) to 5 (best)
 */
int8_t getSignalLevelFromRSSI(long rssi) {
  if(rssi > 0) return 0; //If RRSI is >0, then it's not connected
  if(rssi > -63) return 5;
  if(rssi > -69) return 4;
  if(rssi > -73) return 3;
  if(rssi > -76) return 2;
  if(rssi > -80) return 1;
  return 0;
}