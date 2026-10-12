#include "SdCheckView.h"

// ============================================================================
// SdCheckView - implementare.
//
// Layout in miimi (calibrat pentru 240x320):
//   titlu "SD CARD" y=100 (100‰), status card y=200 (200‰),
//   lista fisiere din y=281 (281‰), pas rand 62‰,
//   buton BACK = aceeasi geometrie ca pe DASHBOARD: (208‰, 844‰, 583‰, 109‰).
// ============================================================================

SdCheckView::SdCheckView(Adafruit_GFX* gfx)
    : _gfx(gfx)
    , _layoutReady(false)
    , _layout(0, 0)
    , _btnRadius(0)
{
}

void SdCheckView::ensureLayout()
{
    if (_layoutReady) return;

    _layout = UiLayout(_gfx->width(), _gfx->height());

    _backRect  = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                      208, 844, 583, 109);
    _btnRadius = _layout.scaleMin(25);

    _layoutReady = true;
}

void SdCheckView::renderFull(const SdCheckViewModel& m)
{
    ensureLayout();

    _gfx->fillScreen(UiTheme::COLOR_BACKGROUND);

    // Titlu (font redus, ca pe START dupa re-layout)
    uint8_t sTitle = UiTheme::textSizeScaled(_layout.height(), 2);
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
    _gfx->setTextSize(sTitle);
    _gfx->setCursor(_layout.centerTextX(sTitle, strlen("SD CARD")),
                    _layout.py(100));
    _gfx->print("SD CARD");

    drawStatus(m);
    drawFileList(m);
    drawBackButton();
}

void SdCheckView::drawStatus(const SdCheckViewModel& m)
{
    ensureLayout();

    uint8_t sStatus = UiTheme::textSizeScaled(_layout.height(), 1);
    _gfx->setTextSize(sStatus);
    _gfx->setCursor(_layout.px(42), _layout.py(200));

    if (m.sdAvailable) {
        char buf[48];
        snprintf(buf, sizeof(buf), "TOTAL %lu MB / FREE %lu MB",
                 (unsigned long)m.sdTotalMB, (unsigned long)m.sdFreeMB);
        _gfx->setTextColor(UiTheme::COLOR_GREEN);
        _gfx->print(buf);
    } else {
        _gfx->setTextColor(UiTheme::COLOR_RED);
        _gfx->print("CARD ABSENT");
    }
}

void SdCheckView::drawFileList(const SdCheckViewModel& m)
{
    ensureLayout();

    if (!m.sdAvailable) return;

    uint8_t sItem = UiTheme::textSizeScaled(_layout.height(), 1);

    if (0 == m.fileCount) {
        _gfx->setTextSize(sItem);
        _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
        _gfx->setCursor(_layout.px(42), _layout.py(281));
        _gfx->print("(niciun fisier)");
        return;
    }

    for (uint8_t i = 0; i < m.fileCount && i < SD_MAX_FILES; i++) {
        int16_t y = _layout.py(281 + i * 62);
        _gfx->setTextSize(sItem);
        _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
        _gfx->setCursor(_layout.px(42), y);
        _gfx->print(m.fileNames[i]);
    }

    // Contor pentru fisierele neafisate
    if (m.extraCount > 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "+%u altele", (unsigned)m.extraCount);
        _gfx->setTextSize(sItem);
        _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
        _gfx->setCursor(_layout.px(42), _layout.py(281 + m.fileCount * 62));
        _gfx->print(buf);
    }
}

void SdCheckView::drawBackButton()
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
