#include "DashboardView.h"

#include <math.h>

// ============================================================================
// DashboardView - implementare.
//
// Layout in miimi (calibrat pentru 240x320):
//   titlu "SERVICE MENU" y=94, telemetrie INA219 y=190,
//   grila 2x2: FLASH (83,280), TEST (537,280), INFO (83,530),
//              SVC OUT (537,530); butoane 380x200, raza 25 la scara minima.
// ============================================================================

namespace {
    constexpr const char* BTN_LABELS[DASH_BTN_COUNT] = {
        "FLASH", "TEST", "INFO", "SVC OUT"
    };
    constexpr uint16_t BTN_X[DASH_BTN_COUNT] = { 83, 537, 83, 537 };
    constexpr uint16_t BTN_Y[DASH_BTN_COUNT] = { 280, 280, 530, 530 };
}

DashboardView::DashboardView(Adafruit_GFX* gfx)
    : _gfx(gfx)
    , _layoutReady(false)
    , _layout(0, 0)
    , _btnRadius(0)
{
}

void DashboardView::ensureLayout()
{
    if (_layoutReady) return;

    _layout = UiLayout(_gfx->width(), _gfx->height());

    for (uint8_t i = 0; i < DASH_BTN_COUNT; i++) {
        _btnRects[i] = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                            BTN_X[i], BTN_Y[i], 380, 200);
    }
    _btnRadius = _layout.scaleMin(25);

    _layoutReady = true;
}

void DashboardView::renderFull(const DashboardViewModel& m)
{
    ensureLayout();

    _gfx->fillScreen(UiTheme::COLOR_BACKGROUND);

    uint8_t sTitle = UiTheme::textSizeScaled(_layout.height(), 2);
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
    _gfx->setTextSize(sTitle);
    _gfx->setCursor(_layout.centerTextX(sTitle, strlen("SERVICE MENU")),
                    _layout.py(94));
    _gfx->print("SERVICE MENU");

    drawTelemetry(m);

    for (uint8_t i = 0; i < DASH_BTN_COUNT; i++) {
        drawButton(i, BTN_LABELS[i]);
    }
}

void DashboardView::drawTelemetry(const DashboardViewModel& m)
{
    ensureLayout();

    // Curata zona liniei de telemetrie
    UiRect zone = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                       42, 180, 917, 44);
    _gfx->fillRect(zone.x, zone.y, zone.w, zone.h, UiTheme::COLOR_BACKGROUND);

    uint8_t s = UiTheme::textSizeScaled(_layout.height(), 1);
    _gfx->setTextSize(s);
    _gfx->setCursor(_layout.px(42), _layout.py(190));

    if (!m.ina219Available) {
        _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
        _gfx->print("INA219: n/a");
        return;
    }

    // NOTA: snprintf("%f") nu e suportat de newlib-nano - formatare manuala.
    int vi = (int)m.busVoltageV;
    int vd = (int)((m.busVoltageV - vi) * 100.0f + 0.5f);
    if (vd >= 100) { vi++; vd = 0; }
    int ii = (int)m.currentMa;
    int id = (int)(fabsf(m.currentMa - ii) * 10.0f + 0.5f);

    _gfx->setTextColor(UiTheme::COLOR_YELLOW);
    _gfx->printf("V: %d.%02d V", vi, vd);
    _gfx->setTextColor(UiTheme::COLOR_GREEN);
    _gfx->printf("   I: %d.%d mA", ii, id);
}

void DashboardView::drawButton(uint8_t idx, const char* label)
{
    ensureLayout();

    const UiRect& r = _btnRects[idx];

    _gfx->fillRoundRect(r.x, r.y, r.w, r.h, _btnRadius, UiTheme::COLOR_BTN_BASE);
    _gfx->drawRoundRect(r.x, r.y, r.w, r.h, _btnRadius, UiTheme::COLOR_TEXT_MAIN);

    uint8_t s = UiTheme::textSizeScaled(_layout.height(), 2);
    int16_t textWidth  = strlen(label) * 6 * s;
    int16_t textHeight = 8 * s;

    _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
    _gfx->setTextSize(s);
    _gfx->setCursor(r.x + (r.w - textWidth) / 2,
                    r.y + (r.h - textHeight) / 2);
    _gfx->print(label);
}
