#include "StartupView.h"

// ============================================================================
// StartupView - implementare.
//
// Toate pozitiile sunt in miimi din dimensiunile display-ului, calibrate
// astfel incat pe 240x320 (ambele targeturi curente) sa reproduca exact
// layout-ul validat anterior pe hardware:
//   titlu y=45 (141‰), subtitlu y=95 (297‰), versiune y=135 (422‰),
//   buton (50,190,140,45) = (208‰, 594‰, 583‰, 141‰), raza 6 (25‰),
//   rand baterie y=266..286 (831‰), punct x=20 (83‰) / text x=32 (133‰),
//   rand SD y=282 (881‰), punct y=290 (906‰) / text y=287 (897‰).
// ============================================================================

StartupView::StartupView(Adafruit_GFX* gfx)
    : _gfx(gfx)
    , _layoutReady(false)
    , _layout(0, 0)
    , _btnRadius(0)
    , _dotRadius(0)
{
}

void StartupView::ensureLayout()
{
    if (_layoutReady) return;

    _layout = UiLayout(_gfx->width(), _gfx->height());

    _btnRect   = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                      208, 594, 583, 141);
    _btnRadius = _layout.scaleMin(25);   // 6 px pe 240
    _dotRadius = _layout.scaleMin(17);   // 4 px pe 240

    _layoutReady = true;
}

void StartupView::renderFull(const StartupViewModel& m)
{
    ensureLayout();

    _gfx->fillScreen(UiTheme::COLOR_BACKGROUND);

    // Titlu principal (centrat orizontal)
    uint8_t sTitle = UiTheme::textSizeScaled(_layout.height(), 3);
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
    _gfx->setTextSize(sTitle);
    _gfx->setCursor(_layout.centerTextX(sTitle, strlen("SERVICE BOX")),
                    _layout.py(141));
    _gfx->print("SERVICE BOX");

    // Subtitlu
    uint8_t sSub = UiTheme::textSizeScaled(_layout.height(), 2);
    _gfx->setTextSize(sSub);
    _gfx->setCursor(_layout.centerTextX(sSub, strlen("PANOURI ASCENSOARE")),
                    _layout.py(297));
    _gfx->print("PANOURI ASCENSOARE");

    // Eticheta versiune
    uint8_t sVer = UiTheme::textSizeScaled(_layout.height(), 1);
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
    _gfx->setTextSize(sVer);
    _gfx->setCursor(_layout.centerTextX(sVer, strlen("v1.0.0")),
                    _layout.py(422));
    _gfx->print("v1.0.0");

    drawButton(m.buttonText);
    drawBatteryInfo(m);
    updateStorage(m.sdFreeMB, m.sdTotalMB, m.sdAvailable);
}

void StartupView::drawButton(const char* text)
{
    ensureLayout();

    const UiRect& r = _btnRect;

    _gfx->fillRoundRect(r.x, r.y, r.w, r.h, _btnRadius, UiTheme::COLOR_BTN_BASE);
    _gfx->drawRoundRect(r.x, r.y, r.w, r.h, _btnRadius, UiTheme::COLOR_TEXT_MAIN);

    uint8_t s = UiTheme::textSizeScaled(_layout.height(), 2);
    int16_t textWidth  = strlen(text) * 6 * s;
    int16_t textHeight = 8 * s;

    int16_t cursorX = r.x + (r.w - textWidth) / 2;
    int16_t cursorY = r.y + _layout.py(47); // 15 px sub marginea butonului (320 ref)

    // Curata zona de text cu culoarea butonului (evita artefacte la
    // schimbarea START / BATT LOW).
    _gfx->fillRect(cursorX - 2, cursorY - 2, textWidth + 4, textHeight + 4,
                   UiTheme::COLOR_BTN_BASE);

    _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
    _gfx->setTextSize(s);
    _gfx->setCursor(cursorX, cursorY);
    _gfx->print(text);
}

void StartupView::drawBatteryInfo(const StartupViewModel& m)
{
    ensureLayout();

    // Zona randului de baterie (intre buton si status SD)
    UiRect zone = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                       42, 831, 917, 62);
    _gfx->fillRect(zone.x, zone.y, zone.w, zone.h, UiTheme::COLOR_BACKGROUND);

    // Punct indicator colorat (culoarea vine rezolvata din model)
    _gfx->fillCircle(_layout.px(83), _layout.py(856), _dotRadius, m.batteryColor);

    // Text tensiune, aliniat cu statusul SD
    char batStr[32];
    snprintf(batStr, sizeof(batStr), "BATTERY: %.2f V", m.batteryVoltageV);
    _gfx->setTextSize(UiTheme::textSizeScaled(_layout.height(), 1));
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
    _gfx->setCursor(_layout.px(133), _layout.py(847));
    _gfx->print(batStr);
}

void StartupView::updateStorage(uint32_t freeMB, uint32_t totalMB, bool cardAvailable)
{
    ensureLayout();

    // Rescrie randul de jos (evita artefacte de flicker)
    UiRect zone = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                       42, 881, 917, 62);
    _gfx->fillRect(zone.x, zone.y, zone.w, zone.h, UiTheme::COLOR_BACKGROUND);
    _gfx->setTextSize(UiTheme::textSizeScaled(_layout.height(), 1));

    uint16_t dotColor = cardAvailable ? UiTheme::COLOR_GREEN : UiTheme::COLOR_RED;
    _gfx->fillCircle(_layout.px(83), _layout.py(906), _dotRadius, dotColor);

    _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
    _gfx->setCursor(_layout.px(133), _layout.py(897));

    if (cardAvailable) {
        _gfx->print("SD: " + String(freeMB) + " MB / " +
                    String((float)totalMB / 1000.0, 1) + " GB FREE");
    } else {
        _gfx->print("SD CARD DISCONNECTED / ERROR");
    }
}
