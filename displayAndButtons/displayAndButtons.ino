#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h>
#include "esp_adc_cal.h"

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23

#define TFT_BL          4  // Display backlight control pin
#define ADC_EN          14
#define ADC_PIN         34
#define BUTTON_1        35
#define BUTTON_2        0



TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

char buff[512];
int vref = 1100;
int btn1Cick = false;
int btn2Cick = false;

int counter = 0;

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{   
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void button_init()
{
    /*
    btn1.setLongClickHandler([](Button2 & b) {
        btn1Cick = false;
        btn2Cick = false;
        displayInCenter("btn 1 long press");
    });

    btn2.setLongClickHandler([](Button2 & b) {
        btn1Cick = false;
        btn2Cick = false;
        displayInCenter("btn 2 long press");
    });

    btn1.setPressedHandler([](Button2 & b) {
        btn1Cick = true;
        btn2Cick = false;
        displayInCenter("btn 1 click");
    });

    btn2.setPressedHandler([](Button2 & b) {
        btn1Cick = false;
        btn2Cick = true;
        displayInCenter("btn 2 click");
    });
    */
}

void displayInCenter(String str)
{
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.fillScreen(TFT_BLACK);
    tft.drawString(str,  tft.width() / 2, tft.height() / 2 );
}

void displayAtTheBottom(String str)
{
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.fillScreen(TFT_BLACK);
    tft.drawString(str,  tft.width() / 2, tft.height() -5 );
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}


void setup()
{
    Serial.begin(115200);
    Serial.println("Start");
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    displayInCenter("Started");

    if (TFT_BL > 0) { // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
         pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
         digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    }
/*
    tft.setSwapBytes(true);
    tft.pushImage(0, 0,  240, 135, ttgo);
    espDelay(5000);
*/

    // can draw here already

    button_init();

    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC1_CHANNEL_6, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
    //Check type of calibration value used to characterize ADC
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        vref = adc_chars.vref;
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
    } else {
        Serial.println("Default Vref: 1100mV");
    }
}

unsigned long lastPoll = 0;
unsigned long lastRedraw = 0;
unsigned long lastPressStarted = 0;
bool pressed = false;

int repressPeriod = 300;
int repeatingPeriod = 100;

void loop()
{
    /*
    if (btn1Cick) {
        showVoltage();
    }
    */
    button_loop();
    unsigned long time = millis();
    if(btn1.isPressed())
    {
        if(!pressed)
        {
            counter++;
            pressed = true;
            lastPressStarted = time;
        }
        else
        {
            if(time-lastPressStarted > repressPeriod && time - lastPoll > repeatingPeriod)
            {
                lastPoll = time;
                counter++;
            }
        }
        
    }
    else
    {
        pressed = false;
    }
        
    if(time - lastRedraw > 100)
    {
        lastRedraw = time;
        int newMillis = millis();
        displayAtTheBottom("c " + String(counter)+
            "\nm "+ String(newMillis/1000) + "." + String(newMillis%1000));
    }
}

///////////

void showVoltage()
{
    static uint64_t timeStamp = 0;
    if (millis() - timeStamp > 1000) {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PIN);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        String voltage = "Voltage :" + String(battery_voltage) + "V";
        Serial.println(voltage);
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(voltage,  tft.width() / 2, tft.height() / 2 );
    }
}

void wifi_scan()
{
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    tft.drawString("Scan Networkrrrrr", tft.width() / 2, tft.height() / 2);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int16_t n = WiFi.scanNetworks();
    tft.fillScreen(TFT_BLACK);
    if (n == 0) {
        tft.drawString("no networks found", tft.width() / 2, tft.height() / 2);
    } else {
        tft.setTextDatum(TL_DATUM);
        tft.setCursor(0, 0);
        Serial.printf("Found %d net\n", n);
        for (int i = 0; i < n; ++i) {
            sprintf(buff,
                    "[%d]:%s(%d)",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i));
            tft.println(buff);
        }
    }
    WiFi.mode(WIFI_OFF);
}

// old  drawing
/*
    tft.setRotation(1);
	int width = tft.width();
	int height = tft.height();
    tft.fillScreen(TFT_BLACK);

	for(int i = 0; i<10; i++)
	{
		tft.fillScreen(TFT_BLACK);
		for(int x = 0; x<width; x++)
		{
			for(int y = 0; y<height-1; y++)
				tft.drawPixel(x, y, (int)((float)i*32/10.0) << 11 | (int)((float)i*32/10.0)*2 << 5 | (int)((float)i*32/10.0));	// red green blue
				//tft.drawPixel(x, y, i*3 << 11 | i*3*2 << 5 | i*3);	// red green blue
		}
		//espDelay(100);
	}


    tft.setRotation(0);
    int i = 5;
    while (i--) {
        tft.fillScreen(TFT_RED);
        espDelay(1000);
        tft.fillScreen(TFT_BLUE);
        espDelay(1000);
        tft.fillScreen(TFT_GREEN);
        espDelay(1000);
    }
*/