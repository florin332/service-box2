#include "StartupView.h"

// ============================================================================
// StartupView - implementare.
//
// Toate pozitiile sunt in miimi din dimensiunile display-ului, calibrate
// astfel incat pe 240x320 (ambele targeturi curente) sa reproduca aspectul
// validat pe hardware, cu re-layout pentru butonul SD:
//   titlu y=30 (94‰, font size 2 - redus o treapta), subtitlu y=75 (234‰),
//   buton SD (50,150,140,30) = (208‰, 469‰, 583‰, 94‰),
//   buton START (50,200,140,45) = (208‰, 625‰, 583‰, 141‰), raza 6 (25‰),
//   rand baterie y=266..286 (831‰), punct x=20 (83‰) / text x=32 (133‰),
//   rand SD status y=282 (881‰), punct y=290 (906‰) / text y=287 (897‰).
// Eticheta de versiune (v1.0.0) a fost eliminata la cerinta.
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

    // Buton SD: aceeasi latime cu START, inaltime redusa, deasupra START.
    _sdRect    = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                      208, 469, 583, 94);
    // Buton START coborat fata de pozitia initiala (594) pentru a face
    // loc butonului SD.
    _btnRect   = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                      208, 625, 583, 141);
    _btnRadius = _layout.scaleMin(25);   // 6 px pe 240
    _dotRadius = _layout.scaleMin(17);   // 4 px pe 240

    _layoutReady = true;
}

void StartupView::renderFull(const StartupViewModel& m)
{
    ensureLayout();

    _gfx->fillScreen(UiTheme::COLOR_BACKGROUND);

    // Titlu principal (centrat orizontal, font redus o treapta si mutat
    // mai sus: face loc butonului SD deasupra butonului START)
    uint8_t sTitle = UiTheme::textSizeScaled(_layout.height(), 2);
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MAIN);
    _gfx->setTextSize(sTitle);
    _gfx->setCursor(_layout.centerTextX(sTitle, strlen("SERVICE BOX")),
                    _layout.py(94));
    _gfx->print("SERVICE BOX");

    // Subtitlu
    uint8_t sSub = UiTheme::textSizeScaled(_layout.height(), 2);
    _gfx->setTextSize(sSub);
    _gfx->setCursor(_layout.centerTextX(sSub, strlen("PANOURI ASCENSOARE")),
                    _layout.py(234));
    _gfx->print("PANOURI ASCENSOARE");

    drawSdButton(m.sdAvailable);
    drawButton(m.buttonText);
    drawBatteryInfo(m);
    updateStorage(m.sdFreeMB, m.sdTotalMB, m.sdAvailable);
}

void StartupView::drawSdButton(bool enabled)
{
    ensureLayout();

    const UiRect& r = _sdRect;

    uint16_t fill = enabled ? UiTheme::COLOR_BTN_BASE
                            : UiTheme::COLOR_BTN_DISABLED;
    _gfx->fillRoundRect(r.x, r.y, r.w, r.h, _btnRadius, fill);

    // Diferenta card detectat/nedetectat o face DOAR conturul:
    // alb cu card, gri fara card. Textul ramane mereu gri (ca pe START).
    uint16_t border = enabled ? UiTheme::COLOR_TEXT_MAIN
                              : UiTheme::COLOR_TEXT_MUTED;
    _gfx->drawRoundRect(r.x, r.y, r.w, r.h, _btnRadius, border);

    uint8_t s = UiTheme::textSizeScaled(_layout.height(), 2);
    int16_t textWidth  = 2 * 6 * s;   // "SD"
    int16_t textHeight = 8 * s;

    _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
    _gfx->setTextSize(s);
    _gfx->setCursor(r.x + (r.w - textWidth) / 2,
                    r.y + (r.h - textHeight) / 2);
    _gfx->print("SD");
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

    // Zona randului de baterie (fara suprapunere cu randul SD de mai jos)
    UiRect zone = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                       42, 831, 917, 44);
    _gfx->fillRect(zone.x, zone.y, zone.w, zone.h, UiTheme::COLOR_BACKGROUND);

    // Punct indicator colorat (culoarea vine rezolvata din model)
    _gfx->fillCircle(_layout.px(83), _layout.py(853), _dotRadius, m.batteryColor);

    // Text tensiune, aliniat cu statusul SD.
    // NOTA: snprintf("%f") NU este suportat de newlib-nano (config default
    // Pico SDK) - valoarea nu se printeaza. Formatare manuala: parte
    // intreaga + 2 zecimale, cu aritmetica pe intregi.
    float v = m.batteryVoltageV;
    int vi = (int)v;
    int vd = (int)((v - vi) * 100.0f + 0.5f);
    if (vd >= 100) { vi++; vd = 0; }  // rotunjire care depaseste (ex. 3.999)
    char batStr[32];
    snprintf(batStr, sizeof(batStr), "BATTERY: %d.%02d V", vi, vd);
    _gfx->setTextSize(UiTheme::textSizeScaled(_layout.height(), 1));
    _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
    _gfx->setCursor(_layout.px(133), _layout.py(844));
    _gfx->print(batStr);
}

void StartupView::updateStorage(uint32_t freeMB, uint32_t totalMB, bool cardAvailable)
{
    ensureLayout();

    // Rescrie randul de jos (fara suprapunere cu randul baterie de mai sus)
    UiRect zone = _layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                       42, 900, 917, 44);
    _gfx->fillRect(zone.x, zone.y, zone.w, zone.h, UiTheme::COLOR_BACKGROUND);
    _gfx->setTextSize(UiTheme::textSizeScaled(_layout.height(), 1));

    uint16_t dotColor = cardAvailable ? UiTheme::COLOR_GREEN : UiTheme::COLOR_RED;
    _gfx->fillCircle(_layout.px(83), _layout.py(922), _dotRadius, dotColor);

    _gfx->setTextColor(UiTheme::COLOR_TEXT_MUTED);
    _gfx->setCursor(_layout.px(133), _layout.py(913));

    if (cardAvailable) {
        // Pe pagina START doar starea; capacitatile sunt pe pagina SD Check.
        _gfx->print("SD: READY");
    } else {
        _gfx->print("SD: MISSING");
    }
}
