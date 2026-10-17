#include "FlashDutView.h"

// ============================================================================
// FlashDutView - implementare.
//
// Layout in miimi (calibrat pentru 240x320, landscape nu e target):
//   titlu "FLASH DUT" y=94, status DUT y=190,
//   randuri fisiere: y=250 + i*90, h=76, x=125, w=750,
//   buton FLASH (208, 620, 583, 100),
//   bara progres (125, 745, 750, 45), text stare y=810,
//   buton BACK (208, 875, 583, 100).
// ============================================================================

FlashDutView::FlashDutView(Adafruit_GFX* gfx)
    : _gfx(gfx)
    , _layoutReady(false)
    , _layout(0, 0)
    , _btnRadius(0)
    , _dotRadius(0)
{
}

void FlashDutView::ensureLayout()
{
    if (_layoutReady) return;

    _layout = UiLayout(_gfx->width(), _gfx->height());

    for (uint8_t i = 0; i < FLASH_MAX_FILES; i++) {
        _fileRects[i] = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                             125, 250 + i * 90, 750, 76);
    }
    _flashRect    = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                         208, 620, 583, 100);
    _progressRect = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                         125, 745, 750, 45);
    _backRect     = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                         208, 875, 583, 100);
    _btnRadius = _layout.scaleMin(25);
    _dotRadius = _layout.scaleMin(17);   // ca indicatorul din StartupView

    _layoutReady = true;
}

void FlashDutView::renderFull(const FlashDutViewModel& m)
{
    ensureLayout();

    _gfx->fillScreen(UiTheme::COLOR_BACKGROUND);

    uint8_t sTitle = UiTheme::textSizeScaled(_layout.height(), 2);
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
    _gfx->setTextSize(sTitle);
    _gfx->setCursor(_layout.centerTextX(sTitle, strlen("FLASH DUT")),
                    _layout.py(94));
    _gfx->print("FLASH DUT");

    drawDutStatus(m);

    for (uint8_t i = 0; i < FLASH_MAX_FILES; i++) {
        drawFileRow(m, i);
    }

    drawFlashButton(m);
    drawStatus(m);
    drawBackButton();
    drawBatteryIndicator(m);
}

void FlashDutView::drawBatteryIndicator(const FlashDutViewModel& m)
{
    ensureLayout();

    // Indicator baterie in coltul stanga-sus, deasupra titlului (ca pe
    // pagina SERVICE MENU)
    UiRect zone = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                       42, 20, 917, 44);
    _gfx->fillRect(zone.x, zone.y, zone.w, zone.h, UiTheme::COLOR_BACKGROUND);

    _gfx->setTextSize(UiTheme::textSizeScaled(_layout.height(), 1));

    if (!m.batteryAvailable) {
        _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
        _gfx->setCursor(_layout.px(83), _layout.py(33));
        _gfx->print("BATTERY: n/a");
        return;
    }

    _gfx->fillCircle(_layout.px(83), _layout.py(42), _dotRadius, m.batteryColor);

    // NOTA: snprintf("%f") nu e suportat de newlib-nano - formatare manuala.
    float v = m.batteryVoltageV;
    int vi = (int)v;
    int vd = (int)((v - vi) * 100.0f + 0.5f);
    if (vd >= 100) { vi++; vd = 0; }
    char batStr[32];
    snprintf(batStr, sizeof(batStr), "BATTERY: %d.%02d V", vi, vd);
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
    _gfx->setCursor(_layout.px(133), _layout.py(33));
    _gfx->print(batStr);
}

void FlashDutView::drawDutStatus(const FlashDutViewModel& m)
{
    ensureLayout();

    // Curata zona liniei de status DUT
    UiRect zone = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                       42, 180, 917, 44);
    _gfx->fillRect(zone.x, zone.y, zone.w, zone.h, UiTheme::COLOR_BACKGROUND);

    uint8_t s = UiTheme::textSizeScaled(_layout.height(), 1);
    _gfx->setTextSize(s);
    _gfx->setCursor(_layout.px(42), _layout.py(190));

    if (!m.hostAvailable) {
        _gfx->setTextColor(UiTheme::COLOR_RED);
        _gfx->print("USB HOST: nedisponibil pe acest target");
    } else if (m.dutConnected) {
        _gfx->setTextColor(UiTheme::COLOR_GREEN);
        _gfx->print("DUT: ");
        _gfx->print(m.dutIdentity[0] ? m.dutIdentity : "conectat");
    } else if (m.blankMsc) {
        _gfx->setTextColor(UiTheme::COLOR_YELLOW);
        _gfx->print("DUT: blank (bootloader, fara identitate)");
    } else {
        _gfx->setTextColor(UiTheme::COLOR_YELLOW);
        _gfx->print("DUT: neconectat");
    }
}

void FlashDutView::drawFileRow(const FlashDutViewModel& m, uint8_t idx)
{
    ensureLayout();

    const UiRect& r = _fileRects[idx];
    bool valid = (idx < m.fileCount);
    bool isSelected = valid && (m.selected == (int8_t)idx);

    uint16_t fill = isSelected ? UiTheme::COLOR_BTN_BASE
                               : UiTheme::COLOR_BACKGROUND;
    uint16_t border = !valid          ? UiTheme::COLOR_BTN_DISABLED
                      : isSelected    ? UiTheme::COLOR_GREEN
                                      : UiTheme::COLOR_TEXT_MUTED;

    _gfx->fillRoundRect(r.x, r.y, r.w, r.h, _btnRadius, fill);
    _gfx->drawRoundRect(r.x, r.y, r.w, r.h, _btnRadius, border);

    uint8_t s = UiTheme::textSizeScaled(_layout.height(), 1);
    _gfx->setTextSize(s);
    _gfx->setCursor(r.x + _layout.scaleMin(42),
                    r.y + (r.h - 8 * s) / 2);

    if (valid) {
        _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
        _gfx->print(m.fileNames[idx]);
    } else if (idx == 0 && m.fileCount == 0) {
        _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
        _gfx->print("(niciun .uf2 pe SD)");
    }
}

void FlashDutView::drawFlashButton(const FlashDutViewModel& m)
{
    ensureLayout();

    const UiRect& r = _flashRect;
    bool enabled = m.hostAvailable && (m.dutConnected || m.blankMsc) &&
                   !m.flashActive && (m.selected >= 0);

    uint16_t fill = enabled ? UiTheme::COLOR_BTN_BASE
                            : UiTheme::COLOR_BTN_DISABLED;
    _gfx->fillRoundRect(r.x, r.y, r.w, r.h, _btnRadius, fill);
    _gfx->drawRoundRect(r.x, r.y, r.w, r.h, _btnRadius, UiTheme::COLOR_TEXT_MAIN);

    uint8_t s = UiTheme::textSizeScaled(_layout.height(), 2);
    int16_t textWidth  = 5 * 6 * s;   // "FLASH"
    int16_t textHeight = 8 * s;

    _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
    _gfx->setTextSize(s);
    _gfx->setCursor(r.x + (r.w - textWidth) / 2,
                    r.y + (r.h - textHeight) / 2);
    _gfx->print("FLASH");
}

void FlashDutView::drawStatus(const FlashDutViewModel& m)
{
    ensureLayout();

    // Zona dinamica: bara de progres + linia de stare sub ea.
    UiRect zone = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                       42, 735, 917, 140);
    _gfx->fillRect(zone.x, zone.y, zone.w, zone.h, UiTheme::COLOR_BACKGROUND);

    // Bara de progres (vizibila doar in timpul flash-ului)
    if (m.flashActive) {
        const UiRect& p = _progressRect;
        _gfx->drawRoundRect(p.x, p.y, p.w, p.h, _btnRadius,
                            UiTheme::COLOR_TEXT_MUTED);
        int16_t fillW = (int16_t)((p.w - 4) * m.progressPct / 100);
        if (fillW > 0) {
            _gfx->fillRoundRect(p.x + 2, p.y + 2, fillW, p.h - 4,
                                _btnRadius / 2 + 1, UiTheme::COLOR_GREEN);
        }
    }

    // Linia de stare (faza curenta / eroare / rezultat)
    if (m.statusLine[0]) {
        uint8_t s = UiTheme::textSizeScaled(_layout.height(), 1);
        _gfx->setTextSize(s);
        _gfx->setTextColor(m.statusColor);
        _gfx->setCursor(_layout.px(42), _layout.py(810));
        _gfx->print(m.statusLine);
    }
}

void FlashDutView::drawBackButton()
{
    ensureLayout();

    const UiRect& r = _backRect;

    _gfx->fillRoundRect(r.x, r.y, r.w, r.h, _btnRadius, UiTheme::COLOR_BTN_BASE);
    _gfx->drawRoundRect(r.x, r.y, r.w, r.h, _btnRadius, UiTheme::COLOR_TEXT_MAIN);

    uint8_t s = UiTheme::textSizeScaled(_layout.height(), 2);
    int16_t textWidth  = 4 * 6 * s;   // "BACK"
    int16_t textHeight = 8 * s;

    _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
    _gfx->setTextSize(s);
    _gfx->setCursor(r.x + (r.w - textWidth) / 2,
                    r.y + (r.h - textHeight) / 2);
    _gfx->print("BACK");
}
