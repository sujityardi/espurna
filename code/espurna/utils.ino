/*

UTILS MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

String getIdentifier() {
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s_%06X"), DEVICE, ESP.getChipId());
    return String(buffer);
}

String buildTime() {

    const char time_now[] = __TIME__;   // hh:mm:ss
    unsigned int hour = atoi(&time_now[0]);
    unsigned int minute = atoi(&time_now[3]);
    unsigned int second = atoi(&time_now[6]);

    const char date_now[] = __DATE__;   // Mmm dd yyyy
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    unsigned int month = 0;
    for ( int i = 0; i < 12; i++ ) {
        if (strncmp(date_now, months[i], 3) == 0 ) {
            month = i + 1;
            break;
        }
    }
    unsigned int day = atoi(&date_now[3]);
    unsigned int year = atoi(&date_now[7]);

    char buffer[20];
    snprintf_P(
        buffer, sizeof(buffer), PSTR("%04d/%02d/%02d %02d:%02d:%02d"),
        year, month, day, hour, minute, second
    );

    return String(buffer);

}


unsigned long getUptime() {

    static unsigned long last_uptime = 0;
    static unsigned char uptime_overflows = 0;

    if (millis() < last_uptime) ++uptime_overflows;
    last_uptime = millis();
    unsigned long uptime_seconds = uptime_overflows * (UPTIME_OVERFLOW / 1000) + (last_uptime / 1000);

    return uptime_seconds;

}

#define DUMP_SETTING(X, Y) DEBUG_MSG_P(PSTR("   %s: %s\n"), X, String(Y).c_str())

void initDump() {


    DEBUG_MSG_P(PSTR("\n[INIT] Current settings:\n\n"));

    DUMP_SETTING("MANUFACTURER", MANUFACTURER);
    DUMP_SETTING("DEVICE", DEVICE);

    DEBUG_MSG_P(PSTR("   ---------------------------\n"));

    DUMP_SETTING("ALEXA_SUPPORT", ALEXA_SUPPORT);
    DUMP_SETTING("ANALOG_SUPPORT", ANALOG_SUPPORT);
    DUMP_SETTING("DEBUG_SERIAL_SUPPORT", DEBUG_SERIAL_SUPPORT);
    DUMP_SETTING("DEBUG_UDP_SUPPORT", DEBUG_UDP_SUPPORT);
    DUMP_SETTING("DHT_SUPPORT", DHT_SUPPORT);
    DUMP_SETTING("DOMOTICZ_SUPPORT", DOMOTICZ_SUPPORT);
    DUMP_SETTING("DS18B20_SUPPORT", DS18B20_SUPPORT);
    DUMP_SETTING("EMON_SUPPORT", EMON_SUPPORT);
    DUMP_SETTING("HOMEASSISTANT_SUPPORT", HOMEASSISTANT_SUPPORT);
    DUMP_SETTING("I2C_SUPPORT", I2C_SUPPORT);
    DUMP_SETTING("INFLUXDB_SUPPORT", INFLUXDB_SUPPORT);
    DUMP_SETTING("MDNS_SUPPORT", MDNS_SUPPORT);
    DUMP_SETTING("NOFUSS_SUPPORT", NOFUSS_SUPPORT);
    DUMP_SETTING("NTP_SUPPORT", NTP_SUPPORT);
    DUMP_SETTING("RF_SUPPORT", RF_SUPPORT);
    DUMP_SETTING("SPIFFS_SUPPORT", SPIFFS_SUPPORT);
    DUMP_SETTING("TERMINAL_SUPPORT", TERMINAL_SUPPORT);
    DUMP_SETTING("WEB_SUPPORT", WEB_SUPPORT);

    DEBUG_MSG_P(PSTR("   ---------------------------\n"));

    DUMP_SETTING("LIGHT_PROVIDER", LIGHT_PROVIDER);
    DUMP_SETTING("MQTT_AUTOCONNECT", MQTT_AUTOCONNECT);
    DUMP_SETTING("MQTT_USE_ASYNC", MQTT_USE_ASYNC);

}

void heartbeat() {

    unsigned long uptime_seconds = getUptime();
    unsigned int free_heap = ESP.getFreeHeap();

    #if NTP_SUPPORT
        DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), (char *) ntpDateTime().c_str());
    #endif

    if (!mqttConnected()) {
        DEBUG_MSG_P(PSTR("[MAIN] Uptime: %ld seconds\n"), uptime_seconds);
        DEBUG_MSG_P(PSTR("[MAIN] Free heap: %d bytes\n"), free_heap);
        #if ADC_VCC_ENABLED
            DEBUG_MSG_P(PSTR("[MAIN] Power: %d mV\n"), ESP.getVcc());
        #endif
    }

    #if (HEARTBEAT_REPORT_INTERVAL)
        mqttSend(MQTT_TOPIC_INTERVAL, HEARTBEAT_INTERVAL / 1000);
    #endif
    #if (HEARTBEAT_REPORT_APP)
        mqttSend(MQTT_TOPIC_APP, APP_NAME);
    #endif
    #if (HEARTBEAT_REPORT_VERSION)
        mqttSend(MQTT_TOPIC_VERSION, APP_VERSION);
    #endif
    #if (HEARTBEAT_REPORT_HOSTNAME)
        mqttSend(MQTT_TOPIC_HOSTNAME, getSetting("hostname").c_str());
    #endif
    #if (HEARTBEAT_REPORT_IP)
        mqttSend(MQTT_TOPIC_IP, getIP().c_str());
    #endif
    #if (HEARTBEAT_REPORT_MAC)
        mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str());
    #endif
    #if (HEARTBEAT_REPORT_RSSI)
        mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());
    #endif
    #if (HEARTBEAT_REPORT_UPTIME)
        mqttSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());
        #if INFLUXDB_SUPPORT
        influxDBSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());
        #endif
    #endif
    #if (HEARTBEAT_REPORT_FREEHEAP)
        mqttSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());
        #if INFLUXDB_SUPPORT
        influxDBSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());
        #endif
    #endif
    #if (HEARTBEAT_REPORT_RELAY)
        relayMQTT();
    #endif
    #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE) & (HEARTBEAT_REPORT_LIGHT)
        lightMQTT();
    #endif
    #if (HEARTBEAT_REPORT_VCC)
    #if ADC_VCC_ENABLED
        mqttSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());
    #endif
    #endif
    #if (HEARTBEAT_REPORT_STATUS)
        mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);
    #endif

}

void customReset(unsigned char status) {
    EEPROM.write(EEPROM_CUSTOM_RESET, status);
    EEPROM.commit();
}

unsigned char customReset() {
    static unsigned char status = 255;
    if (status == 255) {
        status = EEPROM.read(EEPROM_CUSTOM_RESET);
        if (status > 0) customReset(0);
        if (status > CUSTOM_RESET_MAX) status = 0;
    }
    return status;
}

char * ltrim(char * s) {
    char *p = s;
    while ((unsigned char) *p == ' ') ++p;
    return p;
}