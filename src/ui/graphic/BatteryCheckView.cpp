#include "BatteryCheckView.h"

// ============================================================================
// BatteryCheckView - implementare.
//
// Pozitii in miimi din dimensiunile display-ului, calibrate pentru a
// reproduce exact layout-ul validat pe 240x320:
//   titlu y=80 (250‰), tensiune y=130 (406‰), stare incarcare y=200 (625‰),
//   indicator (120,260) = (500‰, 812‰), raza 8 (33‰), text standby y=240
//   (750‰), zona curatare text (80,238,80,14) = (333‰, 744‰, 333‰, 44‰).
// ============================================================================

BatteryCheckView::BatteryCheckView(Adafruit_GFX* gfx)
    : _gfx(gfx)
    , _layoutReady(false)
    , _chargingStandbyActive(false)
    , _layout(0, 0)
    , _indicatorRadius(0)
    , _indicatorX(0)
    , _indicatorY(0)
{
}

void BatteryCheckView::ensureLayout()
{
    if (_layoutReady) return;

    _layout = UiLayout(_gfx->width(), _gfx->height());

    _indicatorRadius = _layout.scaleMin(33);  // 8 px pe 240
    _indicatorX = _layout.px(500);            // centrat orizontal
    _indicatorY = _layout.py(812);            // 260 px pe 320

    _layoutReady = true;
}

void BatteryCheckView::renderMain(const BatteryCheckViewModel& m)
{
    ensureLayout();

    _chargingStandbyActive = false;
    _gfx->fillScreen(UiTheme::COLOR_BACKGROUND);

    // Titlu: BATTERY LEVEL (centrat)
    uint8_t sTitle = UiTheme::textSizeScaled(_layout.height(), 2);
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
    _gfx->setTextSize(sTitle);
    _gfx->setCursor(_layout.centerTextX(sTitle, strlen("BATTERY LEVEL")),
                    _layout.py(250));
    _gfx->print("BATTERY LEVEL");

    // Valoare principala: tensiunea (X.XX V)
    char voltStr[12];
    snprintf(voltStr, sizeof(voltStr), "%.2f V", m.voltageV);

    uint8_t sVolt = UiTheme::textSizeScaled(_layout.height(), 4);
    _gfx->setTextSize(sVolt);
    _gfx->setCursor(_layout.centerTextX(sVolt, strlen(voltStr)),
                    _layout.py(406));
    _gfx->print(voltStr);

    // Stare de incarcare (daca e cazul)
    if (m.chargeText != nullptr) {
        uint8_t sCharge = UiTheme::textSizeScaled(_layout.height(), 2);
        _gfx->setTextColor(m.chargeColor);
        _gfx->setTextSize(sCharge);
        _gfx->setCursor(_layout.centerTextX(sCharge, strlen(m.chargeText)),
                        _layout.py(625));
        _gfx->print(m.chargeText);
    }

    renderIndicator(m);
}

void BatteryCheckView::renderUpdate(const BatteryCheckViewModel& m)
{
    ensureLayout();

    const uint16_t w = _gfx->width();

    // Zona tensiunii (y=406 per-mie): sterge banda si rescrie textul.
    uint8_t sVolt = UiTheme::textSizeScaled(_layout.height(), 4);
    int16_t voltY = _layout.py(406);
    _gfx->fillRect(0, voltY, w, 8 * sVolt, UiTheme::COLOR_BACKGROUND);

    char voltStr[12];
    snprintf(voltStr, sizeof(voltStr), "%.2f V", m.voltageV);
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
    _gfx->setTextSize(sVolt);
    _gfx->setCursor(_layout.centerTextX(sVolt, strlen(voltStr)), voltY);
    _gfx->print(voltStr);

    // Zona starii de incarcare (y=625 per-mie): sterge mereu banda,
    // rescrie textul doar daca exista (tranzitie CHARGING <-> ascuns).
    uint8_t sCharge = UiTheme::textSizeScaled(_layout.height(), 2);
    int16_t chargeY = _layout.py(625);
    _gfx->fillRect(0, chargeY, w, 8 * sCharge, UiTheme::COLOR_BACKGROUND);

    if (m.chargeText != nullptr) {
        _gfx->setTextColor(m.chargeColor);
        _gfx->setTextSize(sCharge);
        _gfx->setCursor(_layout.centerTextX(sCharge, strlen(m.chargeText)),
                        chargeY);
        _gfx->print(m.chargeText);
    }

    renderIndicator(m);
}

void BatteryCheckView::renderIndicator(const BatteryCheckViewModel& m)
{
    ensureLayout();

    // Sterge zona indicatorului (evita artefacte la schimbarea culorii)
    _gfx->fillCircle(_indicatorX, _indicatorY, _indicatorRadius + 2,
                     UiTheme::COLOR_BACKGROUND);
    _gfx->fillCircle(_indicatorX, _indicatorY, _indicatorRadius,
                     m.indicatorColor);
}

void BatteryCheckView::renderStandby(const BatteryCheckViewModel& m)
{
    ensureLayout();

    // Sterge doar zona indicatorului
    _gfx->fillCircle(_indicatorX, _indicatorY, _indicatorRadius + 2,
                     UiTheme::COLOR_BACKGROUND);

    if (m.standbyIndicatorOn) {
        _gfx->fillCircle(_indicatorX, _indicatorY, _indicatorRadius,
                         m.indicatorColor);
    }

    // Tensiunea ramane permanent aprinsa deasupra indicatorului
    char voltStr[12];
    snprintf(voltStr, sizeof(voltStr), "%.2f V", m.voltageV);

    // Zona de curatare a textului (14 px inaltime pe 320 ref)
    UiRect textZone = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                           333, 744, 333, 44);
    _gfx->fillRect(textZone.x, textZone.y, textZone.w, textZone.h,
                   UiTheme::COLOR_BACKGROUND);

    uint8_t s = UiTheme::textSizeScaled(_layout.height(), 1);
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
    _gfx->setTextSize(s);
    _gfx->setCursor(_layout.centerTextX(s, strlen(voltStr)), _layout.py(750));
    _gfx->print(voltStr);
}

void BatteryCheckView::renderChargingStandby(bool dotVisible)
{
    ensureLayout();

    // Prima intrare in standby: sterge tot ecranul (ramane negru).
    if (!_chargingStandbyActive) {
        _gfx->fillScreen(UiTheme::COLOR_BACKGROUND);
        _chargingStandbyActive = true;
    }

    // Punct verde centrat, redesenat doar la schimbarea fazei de clipit.
    int16_t cx = _gfx->width() / 2;
    int16_t cy = _gfx->height() / 2;
    _gfx->fillCircle(cx, cy, _indicatorRadius + 2, UiTheme::COLOR_BACKGROUND);
    if (dotVisible) {
        _gfx->fillCircle(cx, cy, _indicatorRadius, UiTheme::COLOR_GREEN);
    }
}

void BatteryCheckView::clearAll()
{
    _chargingStandbyActive = false;
    _gfx->fillScreen(UiTheme::COLOR_BACKGROUND);
}
