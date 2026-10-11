#include "bsp_touch.h"
#include "bsp_display.h"

#include <Arduino.h>

#if defined(SERVICEBOX_WAVESHARE)

// ============================================================
// WAVESHARE RP2350 - touch CST328 (I2C1) [PORTABLE - tester_port]
// Coincide cu hardware_map.md §2.2 (I2C GP6/GP7, vezi bsp_i2c.h)
// ============================================================

#include "bsp_i2c.h"
#include "bsp_cst328.h"

static bsp_cst328_info_t cst328_info;
static bsp_cst328_data_t touch_data;

// Stare curenta (normalizata)
static bool     s_pressed = false;
static uint16_t s_x       = 0;
static uint16_t s_y       = 0;
static int32_t  s_raw_x   = 0;
static int32_t  s_raw_y   = 0;

// Corectie axe (incarcata din calibrare prin bsp_touch_set_correction)
static bool s_swapXY = false;
static bool s_invX   = false;
static bool s_invY   = false;

// Hold la eliberare: pastram coordonatele cateva cadre pentru a absorbi
// zerourile spre contrand atingerii
static uint8_t s_release_hold = 0;
#define RELEASE_HOLD_FRAMES 4

void bsp_touch_init()
{
    // I2C1: SDA=GP6, SCL=GP7, 400 kHz (magistrala comuna cu INA219)
    bsp_i2c_init();

    cst328_info.width    = bsp_display_width();
    cst328_info.height   = bsp_display_height();
    cst328_info.rotation = 0;

    bsp_cst328_init(&cst328_info);
}

void bsp_touch_set_correction(bool swapXY, bool invX, bool invY)
{
    s_swapXY = swapXY;
    s_invX   = invX;
    s_invY   = invY;
}

void bsp_touch_set_mapping(int32_t xmin, int32_t xmax, int32_t ymin, int32_t ymax)
{
    // CST328 livreaza direct pixeli de display - maparea nu se aplica
    (void)xmin; (void)xmax; (void)ymin; (void)ymax;
}

// Corectie coordonate [PORTABLE - tester_port]
// Aplica swapXY / invX / invY pe coordonatele brute CST328.
static void applyCorrection(uint16_t rx, uint16_t ry, uint16_t &ox, uint16_t &oy)
{
    int32_t cx = rx;
    int32_t cy = ry;
    int32_t w  = bsp_display_width();
    int32_t h  = bsp_display_height();

    if (s_swapXY)
    {
        int32_t t = cx;
        cx = cy;
        cy = t;
    }

    if (s_invX)
    {
        cx = (w - 1) - cx;
    }

    if (s_invY)
    {
        cy = (h - 1) - cy;
    }

    ox = constrain(cx, 0, w - 1);
    oy = constrain(cy, 0, h - 1);
}

bool bsp_touch_read(uint16_t& x, uint16_t& y)
{
    bsp_cst328_read();

    bool got_touch = bsp_cst328_get_touch_data(&touch_data) && touch_data.points > 0;

    if (got_touch)
    {
        s_raw_x = touch_data.coords[0].x;
        s_raw_y = touch_data.coords[0].y;
        s_pressed = true;
        s_release_hold = RELEASE_HOLD_FRAMES;

        applyCorrection((uint16_t)s_raw_x, (uint16_t)s_raw_y, s_x, s_y);
    }
    else if (s_release_hold > 0)
    {
        // Cadru fara date - poate fi glitch -> mentinem starea
        s_release_hold--;
    }
    else
    {
        s_pressed = false;
    }

    x = s_x;
    y = s_y;
    return s_pressed;
}

void bsp_touch_last_raw(int32_t& rx, int32_t& ry, int32_t& z, bool& pressed)
{
    rx = s_raw_x;
    ry = s_raw_y;
    z  = 0;             // CST328 nu expune presiunea pe acest drum
    pressed = s_pressed;
}

// Citire punct brut [PORTABLE - tester_port]
// Polling direct pe I2C - NU depinde de ISR/read_data_done.
// (ISR-ul BSP consuma flag-ul dupa prima citire si nu rearmeaza
//  pana la o noua apasare - ar bloca measureSwipe() din calibrare.)
bool bsp_touch_poll_raw(int32_t& rx, int32_t& ry)
{
    uint8_t buf[6];

    // Registrul FLAG_AND_NUM (0xd005): bitii 0-3 = numar puncte
    bsp_i2c_read_reg16(CST328_DEVICE_ADDR, CST328_TOUCH_FLAG_AND_NUM, &buf[0], 1);
    uint8_t points = buf[0] & 0x0F;

    if (points == 0)
        return false;

    // Citim primul punct: ID + XH + YH + XLYL + PRESSURE (5 bytes de la 0xd000)
    bsp_i2c_read_reg16(CST328_DEVICE_ADDR, CST328_1ST_TOUCH_ID, &buf[0], 5);

    // ID trebuie sa fie 0x06 in nibble-ul jos
    if ((buf[0] & 0x0F) != 0x06)
        return false;

    rx = ((uint16_t)buf[1] << 4) | ((buf[3] & 0xF0) >> 4);
    ry = ((uint16_t)buf[2] << 4) | (buf[3] & 0x0F);

    return true;
}

void bsp_touch_wait_release()
{
    while (true)
    {
        bsp_cst328_read();
        if (!bsp_cst328_get_touch_data(&touch_data) || touch_data.points == 0)
            break;
        delay(10);
    }
}

#elif defined(SERVICEBOX_MARBLE)

// ============================================================
// MARBLE PICO - touch XPT2046 (SPI1, magistrala comuna cu LCD)
// [PRELUARE - tester-touch-lcd / corectii_cod, Model 1 BlueTab]
// Pinii coincid cu hardware_map.md §1.2
// ============================================================

#include <SPI.h>
#include <XPT2046_Touchscreen.h>

static const int8_t PIN_TCH_CS   = 9;
static const int8_t PIN_TCH_IRQ  = 8;

// Prag detectie atingere (unitati ADC raw; upstream aplica Z_THRESHOLD=300
// in touched(), tester-ul validat folosea 200 pentru "pressed")
#define Z_TOUCH_MIN   200

// Valori brute default de calibrare (pana la prima calibrare)
#define DEF_XMIN 3700
#define DEF_XMAX 370
#define DEF_YMIN 300
#define DEF_YMAX 3700

// Instanta unica (lazy). Biblioteca upstream PaulStoffregen/XPT2046_Touchscreen:
// magistrala SPI se aloca la begin() - begin(SPI1), nu prin constructor.
static XPT2046_Touchscreen& ts_instance()
{
    static XPT2046_Touchscreen ts(PIN_TCH_CS, PIN_TCH_IRQ);
    return ts;
}

// Configuratie curenta (incarcata din calibrare prin settere)
static bool    s_swapXY = false;
static bool    s_invX   = false;
static bool    s_invY   = false;
static int32_t s_xmin = DEF_XMIN;
static int32_t s_xmax = DEF_XMAX;
static int32_t s_ymin = DEF_YMIN;
static int32_t s_ymax = DEF_YMAX;

// Cache ultima citire (refolosit de etapele de calibrare in aceeasi
// iteratie de loop - o singura citire SPI per iteratie)
static int32_t  s_raw_x = 0;
static int32_t  s_raw_y = 0;
static int32_t  s_raw_z = 0;
static bool     s_pressed = false;
static uint16_t s_x = 0;
static uint16_t s_y = 0;

void bsp_touch_init()
{
    pinMode(PIN_TCH_CS, OUTPUT);
    digitalWrite(PIN_TCH_CS, HIGH);

    ts_instance().begin(SPI1);    // upstream: aloca magistrala SPI1 (validat)
    ts_instance().setRotation(0);
}

void bsp_touch_set_correction(bool swapXY, bool invX, bool invY)
{
    s_swapXY = swapXY;
    s_invX   = invX;
    s_invY   = invY;
}

void bsp_touch_set_mapping(int32_t xmin, int32_t xmax, int32_t ymin, int32_t ymax)
{
    s_xmin = xmin;
    s_xmax = xmax;
    s_ymin = ymin;
    s_ymax = ymax;
}

// Mapare cu protectie la diviziune cu zero (calibrare corupta) [PRELUARE]
static int safeMap(int v, int fl, int fh, int tl, int th)
{
    if (fh == fl)
    {
        return tl;
    }
    return tl + (v - fl) * (th - tl) / (fh - fl);
}

// Citire punct touch, cu izolarea display-ului gestionata intern [PRELUARE]
static void readPoint(int32_t& px, int32_t& py, int32_t& pz)
{
    bsp_display_deselect();          // izoleaza display-ul (CS LCD = HIGH)
    digitalWrite(PIN_TCH_CS, LOW);

    TS_Point p = ts_instance().getPoint();

    digitalWrite(PIN_TCH_CS, HIGH);

    px = p.x;
    py = p.y;
    pz = p.z;
}

bool bsp_touch_read(uint16_t& x, uint16_t& y)
{
    readPoint(s_raw_x, s_raw_y, s_raw_z);
    s_pressed = (s_raw_z > Z_TOUCH_MIN);

    if (s_pressed)
    {
        // Corectie axe + mapare liniara raw -> pixeli display
        int32_t cx = s_raw_x;
        int32_t cy = s_raw_y;

        if (s_swapXY)
        {
            int32_t t = cx;
            cx = cy;
            cy = t;
        }

        if (s_invX) cx = 4095 - cx;
        if (s_invY) cy = 4095 - cy;

        int w = bsp_display_width();
        int h = bsp_display_height();

        s_x = constrain(safeMap(cx, s_xmin, s_xmax, 0, w - 1), 0, w - 1);
        s_y = constrain(safeMap(cy, s_ymin, s_ymax, 0, h - 1), 0, h - 1);
    }

    x = s_x;
    y = s_y;
    return s_pressed;
}

void bsp_touch_last_raw(int32_t& rx, int32_t& ry, int32_t& z, bool& pressed)
{
    rx = s_raw_x;
    ry = s_raw_y;
    z  = s_raw_z;
    pressed = s_pressed;
}

bool bsp_touch_poll_raw(int32_t& rx, int32_t& ry)
{
    int32_t pz;
    readPoint(rx, ry, pz);
    return (pz > Z_TOUCH_MIN);
}

void bsp_touch_wait_release()
{
    int32_t rx, ry, rz;
    do
    {
        readPoint(rx, ry, rz);
        delay(10);
    }
    while (rz > Z_TOUCH_MIN);
}

#endif // target selection
