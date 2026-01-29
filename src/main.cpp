#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

// OLED display setup
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE, 6, 5);
int width = 72;
int height = 40;
int xOffset = 0; // = (132-w)/2
int yOffset = 0; // = (64-h)/2

// Sensor setup
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

// Display cycle: 0=temp, 1=humidity, 2=temp, 3=pressure, 4=temp, 5=dewpoint
int displayMode = 0;

void setup(void)
{
     delay(1000);
     
     // Initialize OLED display
     u8g2.begin();
     u8g2.setContrast(255); // set contrast to maximum 
     u8g2.setBusClock(400000); //400kHz I2C 
     u8g2.setFont(u8g2_font_ncenB10_tr);
     
     // Initialize sensors
     Serial.begin(115200);
     if (!aht.begin()) {
         Serial.println("Could not find AHT20 sensor!");
     }
     
     if (!bmp.begin()) {
         Serial.println("Could not find BMP280 sensor!");
     }
}

void loop(void)
{
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
    }
    
    // Calculate text width and center it
    int textWidth = u8g2.getStrWidth(displayText);
    int centerX = xOffset + (width - textWidth) / 2;
    
    u8g2.setCursor(centerX, yOffset+25);
    u8g2.print(displayText);
    
    u8g2.sendBuffer(); // transfer internal memory to the display
    
    // Cycle through display modes: temp->humidity->temp->pressure->temp->dewpoint->repeat
    displayMode = (displayMode + 1) % 6;
    
    delay(2000); // Update every 2 seconds
    if (0==displayMode || 2==displayMode | 4==displayMode ) {
        delay(1000); 
    }
}