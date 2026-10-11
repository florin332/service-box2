#include "TouchCalibration.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>

#include "bsp/bsp_display.h"
#include "bsp/bsp_touch.h"

#if defined(SERVICEBOX_WAVESHARE)
// Constante de culoare pentru ecranele de calibrare (nu fac parte din UiTheme -
// calibrarea e un modul separat, cu stil propriu minimal)
#include <Adafruit_ST7789.h>
#define CALIB_COLOR_BG      ST77XX_BLACK
#define CALIB_COLOR_TITLE   ST77XX_CYAN
#define CALIB_COLOR_TEXT    ST77XX_WHITE
#define CALIB_COLOR_ARROW   ST77XX_YELLOW
#define CALIB_COLOR_OK      ST77XX_GREEN
#define CALIB_COLOR_ERROR   ST77XX_RED
#define CALIB_COLOR_HOLD    0x7800  // MAROON
#else
#define CALIB_COLOR_BG      0x0000
#define CALIB_COLOR_TITLE   0x07FF
#define CALIB_COLOR_TEXT    0xFFFF
#define CALIB_COLOR_ARROW   0xFFE0
#define CALIB_COLOR_OK      0x07E0
#define CALIB_COLOR_ERROR   0xF800
#define CALIB_COLOR_HOLD    0x7800
#endif

// ============================================================================
// Implementare per target. Structura de date in EEPROM difera intre
// Waveshare (flag-uri de orientare) si Marble (flag-uri + limite de mapare).
// ============================================================================

#if defined(SERVICEBOX_WAVESHARE)

// ------------------------------------------------------------
// WAVESHARE: calibrare axe CST328 (2 etape: orizontala, verticala)
// [PORTABLE - tester_port]
// ------------------------------------------------------------

#define CALIB_MAGIC  0x43535433  // "CST3"
#define CALIB_VER    2

// Buton hardware recalibrare (GP29, activ LOW).
// NOTA: GP29 nu figureaza in hardware_map.md - vezi todo.md / Agent Proposals
#define RECALIB_BUTTON 29

struct CalibFlags {
    uint32_t magic;
    uint16_t version;
    uint8_t  swapXY;
    uint8_t  invX;
    uint8_t  invY;
    uint8_t  stage;          // 1 = ORIENT_X, 2 = ORIENT_Y, 3 = gata
    uint8_t  recalibrating;  // 1 = flux recalibrare in desfasurare
};

static CalibFlags calib;

void TouchCalibration::begin()
{
    EEPROM.begin(32);
    EEPROM.get(0, calib);

    if (calib.magic != CALIB_MAGIC || calib.version != CALIB_VER)
    {
        // Prima pornire: default validat pe hardware (tester_port)
        calib.swapXY = 0;
        calib.invX   = 1;
        calib.invY   = 1;
        calib.stage  = 3;
        calib.recalibrating = 0;
        calib.magic   = CALIB_MAGIC;
        calib.version = CALIB_VER;
        EEPROM.put(0, calib);
        EEPROM.commit();
    }

    bsp_touch_set_correction(calib.swapXY, calib.invX, calib.invY);

    pinMode(RECALIB_BUTTON, INPUT_PULLUP);
}

bool TouchCalibration::isActive() const
{
    return calib.stage != 3;
}

static void saveCalib()
{
    calib.magic   = CALIB_MAGIC;
    calib.version = CALIB_VER;
    EEPROM.put(0, calib);
    EEPROM.commit();
}

static void forceReboot()
{
    delay(1500);
    rp2040.reboot();
}

// ------------------------------------------------------------
// Ecrane calibrare (instructiuni minimale pentru tehnician)
// ------------------------------------------------------------

static void renderCalibOrientX()
{
    Adafruit_GFX* d = bsp_display_gfx();
    d->fillScreen(CALIB_COLOR_BG);

    d->setCursor(10, 12);
    d->setTextColor(CALIB_COLOR_TITLE);
    d->setTextSize(2);
    d->println("LINIA 1/2: ORIZONTALA");

    d->setTextSize(1);
    d->setTextColor(CALIB_COLOR_TEXT);
    d->setCursor(10, 45);
    d->println("Trageti o linie dreapta continuu");
    d->setCursor(10, 58);
    d->println("de la STANGA la DREAPTA pe ecran.");

    // Sageata orizontala
    d->drawLine(40, 150, 180, 150, CALIB_COLOR_ARROW);
    d->drawLine(40, 151, 180, 151, CALIB_COLOR_ARROW);
    d->drawLine(160, 130, 180, 151, CALIB_COLOR_ARROW);
    d->drawLine(160, 131, 180, 152, CALIB_COLOR_ARROW);
    d->drawLine(160, 171, 180, 151, CALIB_COLOR_ARROW);
    d->drawLine(160, 170, 180, 150, CALIB_COLOR_ARROW);
}

static void renderCalibOrientY()
{
    Adafruit_GFX* d = bsp_display_gfx();
    d->fillScreen(CALIB_COLOR_BG);

    d->setCursor(10, 12);
    d->setTextColor(CALIB_COLOR_TITLE);
    d->setTextSize(2);
    d->println("LINIA 2/2: VERTICALA");

    d->setTextSize(1);
    d->setTextColor(CALIB_COLOR_TEXT);
    d->setCursor(10, 45);
    d->println("Trageti o linie dreapta continuu");
    d->setCursor(10, 58);
    d->println("de SUS in JOS pe ecran.");

    // Sageata verticala
    d->drawLine(120, 100, 120, 220, CALIB_COLOR_ARROW);
    d->drawLine(121, 100, 121, 220, CALIB_COLOR_ARROW);
    d->drawLine(100, 200, 120, 220, CALIB_COLOR_ARROW);
    d->drawLine(101, 200, 121, 220, CALIB_COLOR_ARROW);
    d->drawLine(140, 200, 120, 220, CALIB_COLOR_ARROW);
    d->drawLine(139, 200, 119, 220, CALIB_COLOR_ARROW);
}

static void renderCalibStepSaved(const char *msg)
{
    Adafruit_GFX* d = bsp_display_gfx();
    d->fillScreen(CALIB_COLOR_BG);

    d->setCursor(10, 120);
    d->setTextColor(CALIB_COLOR_TITLE);
    d->setTextSize(2);
    d->println(msg);

    d->setCursor(10, 150);
    d->setTextSize(1);
    d->setTextColor(CALIB_COLOR_OK);
    d->println("Salvat. Repornire...");
}

static void renderCalibResult()
{
    Adafruit_GFX* d = bsp_display_gfx();
    d->fillScreen(CALIB_COLOR_BG);

    d->setCursor(10, 12);
    d->setTextColor(CALIB_COLOR_TITLE);
    d->setTextSize(2);
    d->println("AXE DETECTATE");

    d->setTextSize(1);
    d->setTextColor(CALIB_COLOR_TEXT);

    d->setCursor(10, 50);
    d->print("SwapXY = ");
    d->println(calib.swapXY);
    d->setCursor(10, 65);
    d->print("InvX   = ");
    d->println(calib.invX);
    d->setCursor(10, 80);
    d->print("InvY   = ");
    d->println(calib.invY);

    d->setCursor(10, 110);
    d->setTextColor(CALIB_COLOR_OK);
    d->println("Salvat in flash.");
    d->setCursor(10, 125);
    d->println("Repornire...");
}

static void renderSwipeTooShort()
{
    Adafruit_GFX* d = bsp_display_gfx();
    d->fillRect(0, 200, bsp_display_width(), 30, CALIB_COLOR_BG);
    d->setCursor(10, 208);
    d->setTextSize(1);
    d->setTextColor(CALIB_COLOR_ERROR);
    d->println("Miscare prea scurta. Reincercati.");
}

void TouchCalibration::renderCurrent()
{
    if (calib.stage == 1)      renderCalibOrientX();
    else if (calib.stage == 2) renderCalibOrientY();
}

// ------------------------------------------------------------
// Masurare swipe (polling direct, fara ISR)
// ------------------------------------------------------------

static bool measureSwipe(int32_t &diffX, int32_t &diffY)
{
    int32_t start_rx = 0, start_ry = 0;
    unsigned long timeout = millis() + 30000;

    while (!bsp_touch_poll_raw(start_rx, start_ry))
    {
        if (millis() > timeout)
            return false;
        delay(10);
    }

    int32_t end_rx = start_rx;
    int32_t end_ry = start_ry;
    int32_t rx, ry;

    while (bsp_touch_poll_raw(rx, ry))
    {
        end_rx = rx;
        end_ry = ry;
        delay(10);
    }

    diffX = end_rx - start_rx;
    diffY = end_ry - start_ry;

    if (abs(diffX) < 60 && abs(diffY) < 60)
        return false;

    return true;
}

bool TouchCalibration::update()
{
    if (calib.stage == 1)
    {
        int32_t diffX, diffY;
        if (measureSwipe(diffX, diffY))
        {
            calib.swapXY = (abs(diffX) >= abs(diffY)) ? 0 : 1;
            int32_t dominantX = calib.swapXY ? diffY : diffX;
            calib.invX = (dominantX > 0) ? 0 : 1;

            calib.stage = 2;
            saveCalib();
            renderCalibStepSaved("PAS 1/2 OK");
            forceReboot(); // nu revine
        }
        else
        {
            renderSwipeTooShort();
        }
        return true;
    }

    if (calib.stage == 2)
    {
        int32_t diffX, diffY;
        if (measureSwipe(diffX, diffY))
        {
            int32_t dominantY = calib.swapXY ? diffX : diffY;
            calib.invY = (dominantY > 0) ? 0 : 1;

            calib.stage = 3;
            calib.recalibrating = 0;
            saveCalib();
            renderCalibResult();
            forceReboot(); // nu revine
        }
        else
        {
            renderSwipeTooShort();
        }
        return true;
    }

    return false; // stage == 3: inactiv
}

void TouchCalibration::checkRecalibButton()
{
    if (calib.stage != 3) return;
    if (digitalRead(RECALIB_BUTTON) == HIGH) return;

    unsigned long pressStart = millis();
    bool validHold = true;

    while (millis() - pressStart < 2000)
    {
        if (digitalRead(RECALIB_BUTTON) == HIGH)
        {
            validHold = false;
            break;
        }
        delay(20);
    }

    if (validHold)
    {
        Adafruit_GFX* d = bsp_display_gfx();
        d->fillScreen(CALIB_COLOR_HOLD);
        d->setCursor(15, 120);
        d->setTextSize(2);
        d->setTextColor(CALIB_COLOR_TEXT);
        d->print("ELIBERATI BUTONUL...");

        while (digitalRead(RECALIB_BUTTON) == LOW) { delay(10); }
        delay(100);

        // Armare recalibrare + reboot in stage 1
        calib.swapXY = 0;
        calib.invX   = 0;
        calib.invY   = 0;
        calib.stage  = 1;
        calib.recalibrating = 1;
        saveCalib();

        d->fillScreen(CALIB_COLOR_BG);
        d->setCursor(10, 140);
        d->setTextSize(2);
        d->setTextColor(CALIB_COLOR_TITLE);
        d->println("RECALIBRARE...");

        forceReboot(); // nu revine
    }
}

#elif defined(SERVICEBOX_MARBLE)

// ------------------------------------------------------------
// MARBLE: calibrare XPT2046 (3 etape: orientare X, orientare Y, 4 puncte)
// [PRELUARE - tester-touch-lcd / corectii_cod, Model 1 BlueTab]
// ------------------------------------------------------------

#define Z_SAMPLE_MIN  300
#define CALIB_MAGIC   0x544C4344
#define CALIB_VER     6

// Buton recalibrare (GP3, INPUT_PULLUP, activ LOW).
// NOTA: nu figureaza in hardware_map.md - vezi todo.md / Agent Proposals
#define RECALIB_BUTTON 3

#define DEF_XMIN 3700
#define DEF_XMAX 370
#define DEF_YMIN 300
#define DEF_YMAX 3700

struct __attribute__((packed)) CalibData {
    uint32_t magic;
    uint16_t version;
    int32_t xmin, xmax, ymin, ymax;
    uint8_t swapXY, invX, invY, stage;
};

static CalibData calib;

// Stare etape
static int calibPointIndex = 0;
static int32_t calibX[4], calibY[4];
static bool awaitingRelease = false;
static unsigned long releaseSince = 0;
static enum { COLLECT_IDLE, COLLECT_SAMPLING } collectState = COLLECT_IDLE;
static int32_t calibSamplesX[12], calibSamplesY[12];
static int sampleCount = 0;
static unsigned long calibIdleSince = 0;
static const unsigned long CALIB_POINT_TIMEOUT_MS = 10000;
static int32_t swipeStartX[2], swipeStartY[2];
static int32_t swipeEndX[2], swipeEndY[2];
static int swipeCount = 0;

void TouchCalibration::begin()
{
    EEPROM.begin(64);
    EEPROM.get(0, calib);

    if (calib.magic != CALIB_MAGIC || calib.version != CALIB_VER) {
        calib.magic   = 0;
        calib.xmin    = DEF_XMIN;
        calib.xmax    = DEF_XMAX;
        calib.ymin    = DEF_YMIN;
        calib.ymax    = DEF_YMAX;
        calib.swapXY  = 0;
        calib.invX    = 0;
        calib.invY    = 0;
        calib.stage   = 3;
    }

    bsp_touch_set_correction(calib.swapXY, calib.invX, calib.invY);
    bsp_touch_set_mapping(calib.xmin, calib.xmax, calib.ymin, calib.ymax);

    pinMode(RECALIB_BUTTON, INPUT_PULLUP);
}

bool TouchCalibration::isActive() const
{
    return calib.stage != 3;
}

static bool saveCalib()
{
    calib.magic   = CALIB_MAGIC;
    calib.version = CALIB_VER;
    EEPROM.put(0, calib);
    return EEPROM.commit();
}

static void forceReboot()
{
    delay(500);
    rp2040.reboot();
}

static void processRawTouch(int32_t rx, int32_t ry, int32_t &cx, int32_t &cy, bool useConfig)
{
    cx = rx;
    cy = ry;
    if (useConfig && calib.swapXY) { int32_t t = cx; cx = cy; cy = t; }
    if (useConfig && calib.invX)   { cx = 4095 - cx; }
    if (useConfig && calib.invY)   { cy = 4095 - cy; }
}

// ------------------------------------------------------------
// Desenare ecrane (cu izolare display pe magistrala SPI comuna)
// ------------------------------------------------------------

static void drawCrosshair(int cx, int cy, uint16_t clr)
{
    Adafruit_GFX* d = bsp_display_gfx();
    bsp_display_select();
    d->drawCircle(cx, cy, 10, clr);
    d->drawLine(cx - 16, cy, cx - 6, cy, clr);
    d->drawLine(cx + 6, cy, cx + 16, cy, clr);
    d->drawLine(cx, cy - 16, cx, cy - 6, clr);
    d->drawLine(cx, cy + 6, cx, cy + 16, clr);
    d->fillCircle(cx, cy, 2, clr);
    bsp_display_deselect();
}

static void renderPageOrientX()
{
    Adafruit_GFX* d = bsp_display_gfx();
    bsp_display_select();
    d->fillScreen(CALIB_COLOR_BG);
    d->setCursor(10, 12);
    d->setTextColor(CALIB_COLOR_TITLE);
    d->setTextSize(2);
    d->println("LINIA 1/2: ORIZONTALA");
    d->setTextSize(1);
    d->setTextColor(CALIB_COLOR_TEXT);
    d->setCursor(10, 45);
    d->println("Trageti o linie dreapta continuu");
    d->setCursor(10, 58);
    d->println("de la STANGA la DREAPTA pe ecran.");
    d->setTextSize(2);
    d->setTextColor(CALIB_COLOR_ARROW);
    d->drawLine(40, 150, 180, 150, CALIB_COLOR_ARROW);
    d->drawLine(40, 151, 180, 151, CALIB_COLOR_ARROW);
    d->drawLine(160, 130, 180, 151, CALIB_COLOR_ARROW);
    d->drawLine(160, 131, 180, 152, CALIB_COLOR_ARROW);
    d->drawLine(160, 171, 180, 151, CALIB_COLOR_ARROW);
    d->drawLine(160, 170, 180, 150, CALIB_COLOR_ARROW);
    bsp_display_deselect();
}

static void renderPageOrientY()
{
    Adafruit_GFX* d = bsp_display_gfx();
    bsp_display_select();
    d->fillScreen(CALIB_COLOR_BG);
    d->setCursor(10, 12);
    d->setTextColor(CALIB_COLOR_TITLE);
    d->setTextSize(2);
    d->println("LINIA 2/2: VERTICALA");
    d->setTextSize(1);
    d->setTextColor(CALIB_COLOR_TEXT);
    d->setCursor(10, 45);
    d->println("Trageti o linie dreapta continuu");
    d->setCursor(10, 58);
    d->println("de SUS in JOS pe ecran.");
    d->drawLine(120, 100, 120, 220, CALIB_COLOR_ARROW);
    d->drawLine(121, 100, 121, 220, CALIB_COLOR_ARROW);
    d->drawLine(100, 200, 120, 220, CALIB_COLOR_ARROW);
    d->drawLine(101, 200, 121, 220, CALIB_COLOR_ARROW);
    d->drawLine(140, 200, 120, 220, CALIB_COLOR_ARROW);
    d->drawLine(139, 200, 119, 220, CALIB_COLOR_ARROW);
    bsp_display_deselect();
}

static void renderCalibPointScreen(int idx)
{
    Adafruit_GFX* d = bsp_display_gfx();
    bsp_display_select();
    d->fillScreen(CALIB_COLOR_BG);
    d->setCursor(10, 10);
    d->setTextColor(CALIB_COLOR_TITLE);
    d->setTextSize(2);
    d->print("PAS 2: COORDONATE ");
    d->print(idx + 1);
    d->println("/4");
    bsp_display_deselect();

    const int TX[] = {25, 215, 215, 25};
    const int TY[] = {85, 85, 300, 300};

    for (int i = 0; i < idx; i++) drawCrosshair(TX[i], TY[i], 0x7BEF);
    drawCrosshair(TX[idx], TY[idx], CALIB_COLOR_ARROW);
}

void TouchCalibration::renderCurrent()
{
    if (calib.stage == 1)      renderPageOrientX();
    else if (calib.stage == 4) renderPageOrientY();
    else if (calib.stage == 2) { calibPointIndex = 0; renderCalibPointScreen(0); }
}

// ------------------------------------------------------------
// Loop calibrare
// ------------------------------------------------------------

bool TouchCalibration::update()
{
    // Folosim ultima citire din bsp_touch (facuta la inceput de loop).
    int32_t rx, ry, rz;
    bool pressed;
    bsp_touch_last_raw(rx, ry, rz, pressed);

    // Poarta non-blocanta: ignora touch pana la eliberare completa
    if (awaitingRelease) {
        if (pressed) { releaseSince = 0; }
        else if (releaseSince == 0) { releaseSince = millis(); }
        else if (millis() - releaseSince >= 50) { awaitingRelease = false; releaseSince = 0; }
        if (awaitingRelease) { delay(10); return true; }
    }

    if (calib.stage == 1) {
        // PAS 1: swipe orizontal STANGA -> DREAPTA (swapXY + invX)
        if (pressed) {
            if (swipeCount < 2) { swipeStartX[swipeCount] = rx; swipeStartY[swipeCount] = ry; }
            swipeEndX[0] = swipeEndX[1]; swipeEndX[1] = rx;
            swipeEndY[0] = swipeEndY[1]; swipeEndY[1] = ry;
            swipeCount++;
        }
        else if (swipeCount > 0) {
            if (swipeCount >= 3) {
                long diffX = (long)((swipeEndX[0] + swipeEndX[1]) / 2)
                           - (long)((swipeStartX[0] + swipeStartX[1]) / 2);
                long diffY = (long)((swipeEndY[0] + swipeEndY[1]) / 2)
                           - (long)((swipeStartY[0] + swipeStartY[1]) / 2);

                calib.swapXY = (abs(diffX) > abs(diffY)) ? 0 : 1;
                long dominantX = calib.swapXY ? diffY : diffX;
                calib.invX = (dominantX > 0) ? 0 : 1;

                calib.stage = 4;
                saveCalib();
                forceReboot();
            }
            swipeCount = 0;
        }
        delay(10);
        return true;
    }
    else if (calib.stage == 4) {
        // PAS 4: swipe vertical SUS -> JOS (invY)
        if (pressed) {
            if (swipeCount < 2) { swipeStartX[swipeCount] = rx; swipeStartY[swipeCount] = ry; }
            swipeEndX[0] = swipeEndX[1]; swipeEndX[1] = rx;
            swipeEndY[0] = swipeEndY[1]; swipeEndY[1] = ry;
            swipeCount++;
        }
        else if (swipeCount > 0) {
            if (swipeCount >= 3) {
                long diffX = (long)((swipeEndX[0] + swipeEndX[1]) / 2)
                           - (long)((swipeStartX[0] + swipeStartX[1]) / 2);
                long diffY = (long)((swipeEndY[0] + swipeEndY[1]) / 2)
                           - (long)((swipeStartY[0] + swipeStartY[1]) / 2);

                long dominantY = calib.swapXY ? diffX : diffY;
                calib.invY = (dominantY > 0) ? 0 : 1;

                calib.stage = 2;
                saveCalib();
                forceReboot();
            }
            swipeCount = 0;
        }
        delay(10);
        return true;
    }
    else if (calib.stage == 2) {
        // PAS 2: atingerea a 4 puncte in colturi (xmin/xmax/ymin/ymax)
        if (collectState == COLLECT_IDLE) {
            if (rz >= Z_SAMPLE_MIN) {
                collectState = COLLECT_SAMPLING;
                sampleCount = 0;
                calibIdleSince = 0;
            }
            else {
                if (calibIdleSince == 0) { calibIdleSince = millis(); }
                else if (millis() - calibIdleSince >= CALIB_POINT_TIMEOUT_MS) {
                    renderCalibPointScreen(calibPointIndex);

                    bsp_display_select();
                    Adafruit_GFX* d = bsp_display_gfx();
                    d->setCursor(10, 250);
                    d->setTextSize(1);
                    d->setTextColor(CALIB_COLOR_ERROR, CALIB_COLOR_BG);
                    d->print("Timeout! Atingeti din nou punctul. ");
                    bsp_display_deselect();

                    calibIdleSince = millis();
                }
            }
        }
        else if (rz >= Z_SAMPLE_MIN) {
            if (sampleCount < 12) {
                calibSamplesX[sampleCount] = rx;
                calibSamplesY[sampleCount] = ry;
                sampleCount++;
            }
        }
        else {
            // Deget ridicat: finalizeaza punctul curent
            if (sampleCount >= 5) {
                int32_t sumX = 0, sumY = 0;
                for (int i = 2; i < sampleCount - 2; i++) {
                    sumX += calibSamplesX[i];
                    sumY += calibSamplesY[i];
                }

                int32_t mx, my;
                processRawTouch(sumX / (sampleCount - 4), sumY / (sampleCount - 4), mx, my, true);

                calibX[calibPointIndex] = mx;
                calibY[calibPointIndex] = my;
                calibPointIndex++;

                if (calibPointIndex >= 4) {
                    int32_t loX = calibX[0], hiX = calibX[0];
                    int32_t loY = calibY[0], hiY = calibY[0];
                    for (int i = 1; i < 4; i++) {
                        if (calibX[i] < loX) loX = calibX[i];
                        if (calibX[i] > hiX) hiX = calibX[i];
                        if (calibY[i] < loY) loY = calibY[i];
                        if (calibY[i] > hiY) hiY = calibY[i];
                    }

                    int32_t deltaX = hiX - loX;
                    int32_t deltaY = hiY - loY;

                    // Extrapolare la margini (legata de pozitiile crosshair 25/215, 85/300)
                    calib.xmin = loX - (25 * deltaX) / 190;
                    calib.xmax = hiX + (25 * deltaX) / 190;
                    calib.ymin = loY - (85 * deltaY) / 215;
                    calib.ymax = hiY + (20 * deltaY) / 215;

                    calib.stage = 3;
                    saveCalib();
                    forceReboot();
                }
                else {
                    renderCalibPointScreen(calibPointIndex);
                }
            }
            collectState = COLLECT_IDLE;
        }
        delay(10);
        return true;
    }

    return false; // stage == 3: inactiv
}

void TouchCalibration::checkRecalibButton()
{
    if (calib.stage != 3) return;

    static uint8_t recalibPhase = 0;
    static unsigned long recalibStart = 0;
    bool btnLow = (digitalRead(RECALIB_BUTTON) == LOW);

    if (recalibPhase == 0 && btnLow) {
        recalibPhase = 1;
        recalibStart = millis();
    }
    else if (recalibPhase == 1) {
        if (!btnLow) { recalibPhase = 0; }
        else if (millis() - recalibStart >= 2000) {
            bsp_display_select();
            Adafruit_GFX* d = bsp_display_gfx();
            d->fillScreen(CALIB_COLOR_HOLD);
            d->setCursor(15, 120);
            d->setTextSize(2);
            d->setTextColor(CALIB_COLOR_TEXT);
            d->print("ELIBERATI BUTONUL...");
            bsp_display_deselect();
            recalibPhase = 2;
        }
    }
    else if (recalibPhase == 2 && !btnLow) {
        calib.magic = 0;
        calib.stage = 1;
        saveCalib();
        forceReboot();
    }
}

#endif // target selection
