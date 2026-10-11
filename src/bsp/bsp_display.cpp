#include "bsp_display.h"

#include <Arduino.h>
#include <SPI.h>

#if defined(SERVICEBOX_WAVESHARE)

// ============================================================
// WAVESHARE RP2350 - LCD ST7789T3 (SPI1) [PORTABLE - tester_port]
// Pinii coincid cu hardware_map.md §2.1
// ============================================================

#include <Adafruit_ST7789.h>

#define LCD_SCLK   10
#define LCD_MOSI   11
#define LCD_MISO   12
#define LCD_CS     13
#define LCD_DC     14
#define LCD_RST    15
#define LCD_BL     16

#define LCD_WIDTH  240
#define LCD_HEIGHT 320

// Instanta unica (lazy - evita problemele de ordine la init globalelor)
static Adafruit_ST7789& st7789_instance()
{
    static Adafruit_ST7789 disp(&SPI1, LCD_CS, LCD_DC, LCD_RST);
    return disp;
}

void bsp_display_init()
{
    // Backlight pornit
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);

    // SPI1 pe pinii LCD (SCLK=10, MOSI=11, MISO=12)
    SPI1.setSCK(LCD_SCLK);
    SPI1.setTX(LCD_MOSI);
    SPI1.setRX(LCD_MISO);

    // Initializare ST7789T3
    // SPI_MODE3 = CPOL_1 / CPHA_1, validat pe hardware (tester_port)
    Adafruit_ST7789& disp = st7789_instance();
    disp.init(LCD_WIDTH, LCD_HEIGHT, SPI_MODE3);
    disp.setSPISpeed(80000000);
    disp.invertDisplay(true);   // echivalentul comenzii 0x21 - obligatoriu ST7789T3
    disp.setRotation(0);
}

Adafruit_GFX* bsp_display_gfx()
{
    return &st7789_instance();
}

uint16_t bsp_display_width()  { return LCD_WIDTH; }
uint16_t bsp_display_height() { return LCD_HEIGHT; }

// Waveshare: LCD pe SPI1, touch pe I2C1 - nu exista magistrala comuna
void bsp_display_select()   {}
void bsp_display_deselect() {}

#elif defined(SERVICEBOX_MARBLE)

// ============================================================
// MARBLE PICO - LCD ILI9341 (SPI1, Model 1 Blue Tab) [PRELUARE]
// Pinii coincid cu hardware_map.md §1.1
// ============================================================

#include <Adafruit_ILI9341.h>

static const int8_t PIN_TFT_CS   = 13;
static const int8_t PIN_TFT_RST  = 14;
static const int8_t PIN_TFT_DC   = 6;
static const int8_t PIN_TFT_MOSI = 11;
static const int8_t PIN_TFT_LED  = 4;
static const int8_t PIN_TFT_SCK  = 10;
static const int8_t PIN_TFT_MISO = 12;

#define LCD_WIDTH  240
#define LCD_HEIGHT 320

static Adafruit_ILI9341& ili9341_instance()
{
    static Adafruit_ILI9341 tft(&SPI1, PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_RST);
    return tft;
}

// Init hardware [PRELUARE - din LcdModel1_BlueTab.cpp]
void bsp_display_init()
{
    pinMode(PIN_TFT_CS, OUTPUT);
    digitalWrite(PIN_TFT_CS, HIGH);

    pinMode(PIN_TFT_LED, OUTPUT);
    digitalWrite(PIN_TFT_LED, HIGH);   // backlight ON

    SPI1.setTX(PIN_TFT_MOSI);
    SPI1.setSCK(PIN_TFT_SCK);
    SPI1.setRX(PIN_TFT_MISO);
    SPI1.begin();

    SPI1.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    SPI1.endTransaction();

    Adafruit_ILI9341& tft = ili9341_instance();
    tft.begin();
    tft.invertDisplay(true);   // Model 1 Blue Tab
    tft.setRotation(0);
}

Adafruit_GFX* bsp_display_gfx()
{
    return &ili9341_instance();
}

uint16_t bsp_display_width()  { return LCD_WIDTH; }
uint16_t bsp_display_height() { return LCD_HEIGHT; }

// Selectie/deselectie chip display (izolare pe magistrala comuna cu touch)
void bsp_display_select()   { digitalWrite(PIN_TFT_CS, LOW); }
void bsp_display_deselect() { digitalWrite(PIN_TFT_CS, HIGH); }

#endif // target selection
