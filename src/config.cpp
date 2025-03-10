#include <config.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

/* D E C L A R A T I O N S ****************************************************/  
#define JSON_SIZE 2048
char filename[24] = {"/config.json"};
bool setupMode;
s_config config;


/* P R O T O T Y P E S ********************************************************/
void configGPIO();
void configInitValue();


/**
 * *******************************************************************
 * @brief   Setup for intitial configuration
 * @param   none
 * @return  none
 * *******************************************************************/
void configSetup(){
  
  // start Filesystem 
  Serial.print("LittleFS Status: ");
  Serial.println(LittleFS.begin(true));

  // load config from file
  configLoadFromFile();  
  // gpio settings
  configGPIO();

}

/**
 * *******************************************************************
 * @brief   Setup for GPIO
 * @param   none
 * @return  none
 * *******************************************************************/
void configGPIO(){

  if (setupMode){
    pinMode(LED_BUILTIN, OUTPUT);   // onboard LED
    pinMode(21, OUTPUT);            // green LED D1 on the78mole boards
  }
  else {
    if (config.gpio.led_wifi != -1)
      pinMode(config.gpio.led_wifi, OUTPUT);        // LED for Wifi-Status
    if (config.gpio.led_heartbeat != -1)
      pinMode(config.gpio.led_heartbeat, OUTPUT);   // LED for heartbeat
    if (config.gpio.led_logmode != -1)
    pinMode(config.gpio.led_logmode, OUTPUT);       // LED for LogMode-Status
    
    if (config.oilmeter.use_hardware_meter){
      if (config.gpio.trigger_oilcounter != -1)
        pinMode(config.gpio.trigger_oilcounter, INPUT_PULLUP);    // Trigger Input
      if (config.gpio.led_oilcounter != -1)
        pinMode(config.gpio.led_oilcounter, OUTPUT);              // Status LED
    }
  }

}

/**
 * *******************************************************************
 * @brief   intitial configuration values
 * @param   none
 * @return  none
 * *******************************************************************/
void configInitValue(){

    memset(&config, 0, sizeof(config));

    // WiFi
    snprintf(config.wifi.ssid, sizeof(config.wifi.ssid), "enter SSID...");
    snprintf(config.wifi.password, sizeof(config.wifi.password), "enter password...");
    snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), "ESP-Buderus-KM271");

    // MQTT
    snprintf(config.mqtt.server, sizeof(config.mqtt.server), "enter MQTT server...");
    snprintf(config.mqtt.user, sizeof(config.mqtt.user), "enter MQTT user...");
    snprintf(config.mqtt.password, sizeof(config.mqtt.password), "enter MQTT password...");
    snprintf(config.mqtt.topic, sizeof(config.mqtt.topic), "enter MQTT topic...");
    config.mqtt.port = 1883;
    config.mqtt.enable = false;

    // NTP
    snprintf(config.ntp.server, sizeof(config.ntp.server), "de.pool.ntp.org");
    snprintf(config.ntp.tz, sizeof(config.ntp.tz), "CET-1CEST,M3.5.0,M10.5.0/3");
    config.ntp.enable = true;

    // heating circuits
    config.km271.use_hc1 = false;
    config.km271.use_hc2 = false;
    config.km271.use_ww = false;

    // km271
    config.km271.use_alarmMsg = false;

    // language
    config.lang = 0;
    
    // oilmeter
    config.oilmeter.use_hardware_meter = false;
    config.oilmeter.use_virtual_meter = false;

    // webUI
    config.webUI.enable = true;

    // gpio
    memset(&config.gpio, -1, sizeof(config.gpio));

}

/**
 * *******************************************************************
 * @brief   save configuration to file
 * @param   none
 * @return  none
 * *******************************************************************/
void configSaveToFile() {

    DynamicJsonDocument doc(JSON_SIZE); // reserviert 2048 Bytes für das JSON-Objekt

    doc["lang"] = (config.lang);

    doc["oilmeter"]["use_hardware_meter"] = config.oilmeter.use_hardware_meter;
    doc["oilmeter"]["use_virtual_meter"] = config.oilmeter.use_virtual_meter;
    doc["oilmeter"]["consumption_kg_h"] = config.oilmeter.consumption_kg_h;
    doc["oilmeter"]["oil_density_kg_l"] = config.oilmeter.oil_density_kg_l;

    doc["wifi"]["ssid"] = config.wifi.ssid;
    doc["wifi"]["password"] = config.wifi.password;
    doc["wifi"]["hostname"] = config.wifi.hostname;

    doc["mqtt"]["enable"] = config.mqtt.enable;
    doc["mqtt"]["server"] = config.mqtt.server;
    doc["mqtt"]["user"] = config.mqtt.user;
    doc["mqtt"]["password"] = config.mqtt.password;
    doc["mqtt"]["topic"] = config.mqtt.topic;
    doc["mqtt"]["port"] = config.mqtt.port;
    doc["mqtt"]["config_retain"] = config.mqtt.config_retain;

    doc["ntp"]["enable"] = config.ntp.enable;
    doc["ntp"]["server"] = config.ntp.server;
    doc["ntp"]["tz"] = config.ntp.tz;

    doc["gpio"]["led_wifi"] = config.gpio.led_wifi;
    doc["gpio"]["led_heartbeat"] = config.gpio.led_heartbeat;
    doc["gpio"]["led_logmode"] = config.gpio.led_logmode;
    doc["gpio"]["led_oilcounter"] = config.gpio.led_oilcounter;
    doc["gpio"]["trigger_oilcounter"] = config.gpio.trigger_oilcounter;
    doc["gpio"]["km271_RX"] = config.gpio.km271_RX;
    doc["gpio"]["km271_TX"] = config.gpio.km271_TX;

    doc["webUI"]["enable"] = config.webUI.enable;

    doc["km271"]["use_hc1"] = config.km271.use_hc1;
    doc["km271"]["use_hc2"] = config.km271.use_hc2;
    doc["km271"]["use_ww"] = config.km271.use_ww;
    doc["km271"]["use_alarmMsg"] = config.km271.use_alarmMsg;

    // Delete existing file, otherwise the configuration is appended to the file
    LittleFS.remove(filename);

    // Open file for writing
    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file) {
      Serial.println(F("Failed to create file"));
      return;
    }

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) {
      Serial.println(F("Failed to write to file"));
    }

    // Close the file
    file.close();
}

/**
 * *******************************************************************
 * @brief   load configuration from file
 * @param   none
 * @return  none
 * *******************************************************************/
void configLoadFromFile() {
  // Open file for reading
  File file = LittleFS.open(filename);

  // Allocate a temporary JsonDocument
  StaticJsonDocument<JSON_SIZE> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to read file, using default configuration and start wifi-AP"));
    configInitValue();
    setupMode = true;
  }
  else {
    // Copy values from the JsonDocument to the Config structure
    config.oilmeter.use_hardware_meter = doc["oilmeter"]["use_hardware_meter"];
    config.oilmeter.use_virtual_meter = doc["oilmeter"]["use_virtual_meter"];
    config.oilmeter.consumption_kg_h = doc["oilmeter"]["consumption_kg_h"];
    config.oilmeter.oil_density_kg_l = doc["oilmeter"]["oil_density_kg_l"];
    
    config.lang = doc["lang"];

    snprintf(config.wifi.ssid, sizeof(config.wifi.ssid), doc["wifi"]["ssid"]);
    snprintf(config.wifi.password, sizeof(config.wifi.password), doc["wifi"]["password"]);
    snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), doc["wifi"]["hostname"]);
    
    config.mqtt.enable = doc["mqtt"]["enable"];
    snprintf(config.mqtt.server, sizeof(config.mqtt.server), doc["mqtt"]["server"]);
    snprintf(config.mqtt.user, sizeof(config.mqtt.user), doc["mqtt"]["user"]);
    snprintf(config.mqtt.password, sizeof(config.mqtt.password), doc["mqtt"]["password"]);
    snprintf(config.mqtt.topic, sizeof(config.mqtt.topic), doc["mqtt"]["topic"]);
    config.mqtt.port = doc["mqtt"]["port"];
    config.mqtt.config_retain = doc["mqtt"]["config_retain"];

    config.ntp.enable = doc["ntp"]["enable"];
    snprintf(config.ntp.server, sizeof(config.ntp.server), doc["ntp"]["server"]);
    snprintf(config.ntp.tz, sizeof(config.ntp.tz), doc["ntp"]["tz"]);
    
    config.gpio.led_wifi = doc["gpio"]["led_wifi"];
    config.gpio.led_heartbeat = doc["gpio"]["led_heartbeat"];
    config.gpio.led_logmode = doc["gpio"]["led_logmode"];
    config.gpio.led_oilcounter = doc["gpio"]["led_oilcounter"];
    config.gpio.trigger_oilcounter = doc["gpio"]["trigger_oilcounter"];
    config.gpio.km271_RX = doc["gpio"]["km271_RX"];
    config.gpio.km271_TX = doc["gpio"]["km271_TX"];
    
    config.webUI.enable = doc["webUI"]["enable"];

    config.km271.use_hc1 = doc["km271"]["use_hc1"];
    config.km271.use_hc2 = doc["km271"]["use_hc2"];
    config.km271.use_ww = doc["km271"]["use_ww"];
    config.km271.use_alarmMsg = doc["km271"]["use_alarmMsg"];
  }
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}

