#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// OLED display setup
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE, 6, 5);
int width = 72;
int height = 40;
int xOffset = 0; // = (132-w)/2
int yOffset = 0; // = (64-h)/2

// Sensor setup
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

// WiFi and Web Server setup
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

// WiFi Configuration
String ssid = "";
String password = "";
bool wifiConfigMode = false;
const char* ap_ssid = "EggESP32";
const char* ap_password = "93z45x62i";

// Global sensor variables
float currentTemperature = NAN;
float currentHumidity = NAN;
float currentPressure = NAN;
float currentDewPoint = NAN;

// Display cycle: 0=temp, 1=humidity, 2=temp, 3=pressure, 4=temp, 5=dewpoint, 6=IP address
int displayMode = 0;
int configDisplayMode = 0; // 0=SSID, 1=password, 2=IP address, 3=temp, 4=humidity, 5=pressure, 6=dewpoint

// BOOT button setup (GPIO 9 on ESP32-C3)
const int BOOT_PIN = 9;
bool lastButtonState = HIGH;
bool buttonPressed = false;
unsigned long buttonPressStartTime = 0;
const unsigned long WIFI_RESET_DURATION = 3000; // Hold button for 3 seconds to reset WiFi

// WiFi Configuration Functions
void loadWiFiConfig() {
    preferences.begin("wifi", false);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    preferences.end();
}

void saveWiFiConfig() {
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
}

void resetWiFiConfig() {
    preferences.begin("wifi", false);
    preferences.remove("ssid");
    preferences.remove("password");
    preferences.end();
    ssid = "";
    password = "";
    Serial.println("WiFi settings reset!");
}

void handleRoot() {
    String html = "<!DOCTYPE html><html><head><title>ESP32 WiFi Config</title></head><body>";
    html += "<h1>WiFi Configuration</h1>";
    html += "<form action='/save' method='POST'>";
    html += "SSID: <input type='text' name='ssid' value='" + ssid + "'><br><br>";
    html += "Password: <input type='text' name='password' value='" + password + "'><br><br>";
    html += "<input type='submit' value='Save'>";
    html += "</form></body></html>";
    
    server.send(200, "text/html", html);
}

void handleSave() {
    ssid = server.arg("ssid");
    password = server.arg("password");
    
    saveWiFiConfig();
    
    String html = "<!DOCTYPE html><html><head><title>WiFi Saved</title></head><body>";
    html += "<h1>WiFi Configuration Saved!</h1>";
    html += "<p>SSID: " + ssid + "</p>";
    html += "<p>Device will restart and try to connect...</p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
    
    delay(2000);
    ESP.restart();
}

void handleNotFound() {
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>ESP32 Setup</title></head><body><h1>ESP32 WiFi Setup</h1><p><a href='/'>Configure WiFi</a></p></body></html>");
}

void startConfigPortal() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);
    
    dnsServer.start(53, "*", WiFi.softAPIP());
    wifiConfigMode = true;
    
    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.onNotFound(handleNotFound);
    server.begin();
    
    Serial.println("Configuration portal started");
    Serial.println("Connect to WiFi: " + String(ap_ssid));
    Serial.println("Password: " + String(ap_password));
    Serial.println("Go to: http://192.168.4.1");
}

void handleSensorData() {
    String json = "{";
    json += "\"temperature\":" + String(currentTemperature) + ",";
    json += "\"humidity\":" + String(currentHumidity) + ",";
    json += "\"pressure\":" + String(currentPressure) + ",";
    json += "\"dewPoint\":" + String(currentDewPoint);
    json += "}";
    
    server.send(200, "application/json", json);
}

void handleWebPage() {
    String html = "<!DOCTYPE html><html><head>\n";
    html += "<title>ESP32 Sensor Data</title>\n";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8'>\n";
    html += "<style>\n";
    html += "body { font-family: Arial, sans-serif; margin: 40px; background: #f4f4f4; }\n";
    html += ".container { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n";
    html += ".sensor-card { background: #e7f3ff; margin: 10px 0; padding: 15px; border-radius: 5px; border-left: 4px solid #007acc; }\n";
    html += ".value { font-size: 24px; font-weight: bold; color: #007acc; }\n";
    html += ".unit { font-size: 14px; color: #666; }\n";
    html += ".refresh-btn { background: #007acc; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }\n";
    html += "</style>\n";
    html += "<script>\n";
    html += "function updateData() {\n";
    html += "  fetch('/data').then(response => response.json()).then(data => {\n";
    html += "    document.getElementById('temp').innerHTML = data.temperature.toFixed(1) + ' °C';\n";
    html += "    document.getElementById('humidity').innerHTML = data.humidity.toFixed(1) + ' %';\n";
    html += "    document.getElementById('pressure').innerHTML = data.pressure.toFixed(0) + ' hPa';\n";
    html += "    document.getElementById('dewpoint').innerHTML = data.dewPoint.toFixed(1) + ' °C';\n";
    html += "    document.getElementById('lastUpdate').innerHTML = 'Last update: ' + new Date().toLocaleTimeString();\n";
    html += "  });\n";
    html += "}\n";
    html += "setInterval(updateData, 10000);\n";
    html += "</script></head><body>\n";
    html += "<div class='container'>\n";
    html += "<h1>ESP32 Environmental Sensor</h1>\n";
    html += "<div class='sensor-card'>\n";
    html += "<h3>Temperature</h3>\n";
    html += "<div class='value' id='temp'>" + String(currentTemperature, 1) + " °C</div>\n";
    html += "</div>\n";
    html += "<div class='sensor-card'>\n";
    html += "<h3>Humidity</h3>\n";
    html += "<div class='value' id='humidity'>" + String(currentHumidity, 1) + " %</div>\n";
    html += "</div>\n";
    html += "<div class='sensor-card'>\n";
    html += "<h3>Pressure</h3>\n";
    html += "<div class='value' id='pressure'>" + String(currentPressure, 0) + " hPa</div>\n";
    html += "</div>\n";
    html += "<div class='sensor-card'>\n";
    html += "<h3>Dew Point</h3>\n";
    html += "<div class='value' id='dewpoint'>" + String(currentDewPoint, 1) + " °C</div>\n";
    html += "</div>\n";
    html += "<button class='refresh-btn' onclick='updateData()'>Refresh Data</button>\n";
    html += "<p id='lastUpdate'>Data updates automatically every 10 seconds</p>\n";
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
}

bool connectToWiFi() {
    if (ssid == "") return false;
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected!");
        Serial.println("IP address: " + WiFi.localIP().toString());
        
        // Setup web server for sensor data
        server.on("/", handleWebPage);
        server.on("/data", handleSensorData);
        server.begin();
        
        return true;
    } else {
        Serial.println("");
        Serial.println("Failed to connect to WiFi");
        return false;
    }
}

void setup(void)
{
     delay(1000);
     Serial.begin(115200);
     Serial.println("ESP32 Sensor with WiFi starting...");
     
     // Initialize OLED display
     u8g2.begin();
     u8g2.setContrast(255); // set contrast to maximum 
     u8g2.setBusClock(400000); //400kHz I2C 
     u8g2.setFont(u8g2_font_ncenB10_tr);
     
     // Initialize BOOT button
     pinMode(BOOT_PIN, INPUT_PULLUP);
     
     // Display startup message
     u8g2.clearBuffer();
     u8g2.setCursor(0, 15);
     u8g2.print("Starting...");
     u8g2.sendBuffer();
     
     // Initialize sensors
     if (!aht.begin()) {
         Serial.println("Could not find AHT20 sensor!");
     }
     
     if (!bmp.begin()) {
         Serial.println("Could not find BMP280 sensor!");
     }
     
     // WiFi Configuration
     loadWiFiConfig();
     
     // Display WiFi status
     u8g2.clearBuffer();
     u8g2.setCursor(0, 15);
     u8g2.print("WiFi...");
     u8g2.sendBuffer();
     
     // Check if no WiFi credentials are stored
     if (ssid == "") {
         // No WiFi credentials stored, enter config mode
         u8g2.clearBuffer();
         u8g2.setCursor(0, 15);
         u8g2.print("Config");
         u8g2.setCursor(0, 30);
         u8g2.print("Mode");
         u8g2.sendBuffer();
         
         startConfigPortal();
     } else if (!connectToWiFi()) {
         // WiFi credentials exist but connection failed
         u8g2.clearBuffer();
         u8g2.setCursor(0, 12);
         u8g2.print("WiFi Failed");
         u8g2.setCursor(0, 30);
         u8g2.print("Hold BOOT 3s");
         u8g2.sendBuffer();
         delay(2000);
         
         // Continue with normal operation without WiFi
     } else {
         u8g2.clearBuffer();
         u8g2.setCursor(0, 15);
         u8g2.print("WiFi OK");
         u8g2.sendBuffer();
         delay(1000);
     }
}

void loop(void)
{
    // Handle web server requests
    if (wifiConfigMode) {
        dnsServer.processNextRequest();
    }
    server.handleClient();
    
    // Check BOOT button for WiFi reset (long press)
    bool currentButtonState = digitalRead(BOOT_PIN);
    
    if (lastButtonState == HIGH && currentButtonState == LOW) {
        // Button was pressed (falling edge)
        buttonPressed = true;
        buttonPressStartTime = millis();
    } else if (lastButtonState == LOW && currentButtonState == HIGH) {
        // Button was released (rising edge)
        buttonPressed = false;
    }
    
    // Check for long press (3 seconds) to reset WiFi
    if (buttonPressed && (millis() - buttonPressStartTime > WIFI_RESET_DURATION)) {
        // Display reset message
        u8g2.clearBuffer();
        u8g2.setCursor(0, 15);
        u8g2.print("WiFi");
        u8g2.setCursor(0, 30);
        u8g2.print("Reset");
        u8g2.sendBuffer();
        delay(300);
        // Reset WiFi settings and enter config mode
        resetWiFiConfig();
        
        
        delay(1000);
        u8g2.clearBuffer();
        u8g2.setCursor(0, 15);
        u8g2.print("Resta-");
        u8g2.setCursor(0, 30);
        u8g2.print("rting...");
        u8g2.sendBuffer();
        delay(2000);
        ESP.restart();
    }
    
    lastButtonState = currentButtonState;
    
    // Read sensors
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    
    float temperature = temp.temperature;
    float humidityValue = humidity.relative_humidity;
    float pressure = bmp.readPressure() / 100.0F; // Convert Pa to hPa
    
    // Calculate dew point
    float dewPoint = NAN;
    if (!isnan(temperature) && !isnan(humidityValue)) {
        float a = 17.27;
        float b = 237.7;
        float alpha = log(humidityValue/100.0) + (a * temperature) / (b + temperature);
        dewPoint = (b * alpha) / (a - alpha);
    }
    
    // If AHT20 fails, try BMP280 as backup for temperature
    if (isnan(temperature)) {
        temperature = bmp.readTemperature();
    }
    
    // Update global variables for web server
    currentTemperature = temperature;
    currentHumidity = humidityValue;
    currentPressure = pressure;
    currentDewPoint = dewPoint;
    
    // Skip OLED display in config mode
    if (!wifiConfigMode) {
        u8g2.clearBuffer(); // clear the internal memory
        
        // Draw frame only for dew point display
        if (displayMode == 5) {
            u8g2.drawFrame(xOffset, yOffset, width, height); //draw a frame around the border
        }
        
        // Prepare text based on current display mode
        char displayText[16];
        switch (displayMode) {
            case 0: // Temperature
            case 2: // Temperature again  
            case 4: // Temperature third time
                if (!isnan(temperature)) {
                    snprintf(displayText, sizeof(displayText), "%.1f'C", temperature);
                } else {
                    strcpy(displayText, "Temp Error");
                }
                break;
            case 1: // Humidity
                if (!isnan(humidityValue)) {
                    snprintf(displayText, sizeof(displayText), "%.1f%%", humidityValue);
                } else {
                    strcpy(displayText, "Hum Error");
                }
                break;
            case 3: // Pressure
                if (!isnan(pressure)) {
                    snprintf(displayText, sizeof(displayText), "%.0fhPa", pressure);
                } else {
                    strcpy(displayText, "Press Error");
                }
                break;
            case 5: // Dew point
                if (!isnan(dewPoint)) {
                    snprintf(displayText, sizeof(displayText), ": %.1f'C", dewPoint);
                } else {
                    strcpy(displayText, "DP Error");
                }
                break;
            case 6: // IP address
                if (WiFi.status() == WL_CONNECTED) {
                    String ip = WiFi.localIP().toString();
                    // Split IP into two parts to fit on small screen
                    int dotPos = ip.lastIndexOf('.');
                    String firstPart = ip.substring(0, dotPos);
                    String lastPart = ip.substring(dotPos);
                    
                    // Display IP in two lines
                    u8g2.setCursor(0, 12);
                    u8g2.print(firstPart);
                    u8g2.setCursor(0, 30);
                    u8g2.print(lastPart);
                    u8g2.sendBuffer();
                    
                    // Cycle through display modes: temp->humidity->temp->pressure->temp->dewpoint->IP->repeat
                    displayMode = (displayMode + 1) % 7;
                    
                    delay(2000); // Update every 2 seconds
                    if (0==displayMode || 2==displayMode || 4==displayMode || 6==displayMode ) {
                        delay(1000); 
                    }
                    return; // Exit loop early for IP display
                } else {
                    strcpy(displayText, "No WiFi");
                }
                break;
        }
        
        // Calculate text width and center it
        int textWidth = u8g2.getStrWidth(displayText);
        int centerX = xOffset + (width - textWidth) / 2;
        
        u8g2.setCursor(centerX, yOffset+25);
        u8g2.print(displayText);
        
        u8g2.sendBuffer(); // transfer internal memory to the display
        
        // Cycle through display modes: temp->humidity->temp->pressure->temp->dewpoint->IP->repeat
        displayMode = (displayMode + 1) % 7;
        
        delay(2000); // Update every 2 seconds
        if (0==displayMode || 2==displayMode || 4==displayMode || 6==displayMode ) {
            delay(1000); 
        }
    } else {
        // In config mode, alternate between IP address and WiFi credentials
        u8g2.clearBuffer();
        
        if (configDisplayMode == 0) {
            // Show WiFi SSID
            u8g2.setCursor(0, 12);
            u8g2.print("SSID:");
            u8g2.setCursor(0, 30);
            u8g2.print(ap_ssid);
        } else if (configDisplayMode == 1) {
            // Show WiFi password
            u8g2.setCursor(0, 12);
            u8g2.print("Password:");
            u8g2.setCursor(0, 30);
            u8g2.print(ap_password);
        } else if (configDisplayMode == 2) {
            // Show IP address
            u8g2.setCursor(0, 15);
            u8g2.print("Config");
            u8g2.setCursor(0, 30);
            u8g2.print("192.168.4.1");
        } else if (configDisplayMode == 3) {
            // Show Temperature
            char displayText[16];
            if (!isnan(temperature)) {
                snprintf(displayText, sizeof(displayText), "%.1f'C", temperature);
            } else {
                strcpy(displayText, "Temp Error");
            }
            int textWidth = u8g2.getStrWidth(displayText);
            int centerX = xOffset + (width - textWidth) / 2;
            u8g2.setCursor(centerX, yOffset+25);
            u8g2.print(displayText);
        } else if (configDisplayMode == 4) {
            // Show Humidity
            char displayText[16];
            if (!isnan(humidityValue)) {
                snprintf(displayText, sizeof(displayText), "%.1f%%", humidityValue);
            } else {
                strcpy(displayText, "Hum Error");
            }
            int textWidth = u8g2.getStrWidth(displayText);
            int centerX = xOffset + (width - textWidth) / 2;
            u8g2.setCursor(centerX, yOffset+25);
            u8g2.print(displayText);
        } else if (configDisplayMode == 5) {
            // Show Pressure
            char displayText[16];
            if (!isnan(pressure)) {
                snprintf(displayText, sizeof(displayText), "%.0fhPa", pressure);
            } else {
                strcpy(displayText, "Press Error");
            }
            int textWidth = u8g2.getStrWidth(displayText);
            int centerX = xOffset + (width - textWidth) / 2;
            u8g2.setCursor(centerX, yOffset+25);
            u8g2.print(displayText);
        } else {
            // Show Dew point (configDisplayMode == 6)
            u8g2.drawFrame(xOffset, yOffset, width, height); //draw a frame around the border
            char displayText[16];
            if (!isnan(dewPoint)) {
                snprintf(displayText, sizeof(displayText), ": %.1f'C", dewPoint);
            } else {
                strcpy(displayText, "DP Error");
            }
            int textWidth = u8g2.getStrWidth(displayText);
            int centerX = xOffset + (width - textWidth) / 2;
            u8g2.setCursor(centerX, yOffset+25);
            u8g2.print(displayText);
        }
        
        u8g2.sendBuffer();
        
        // Switch display mode every 3 seconds
        configDisplayMode = (configDisplayMode + 1) % 7;
        delay(3000);
    }
}