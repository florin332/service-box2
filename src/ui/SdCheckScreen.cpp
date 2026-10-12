#include "SdCheckScreen.h"

#include "bsp/bsp_sdcard.h"

// ============================================================================
// SdCheckScreen - implementare.
//
// Datele SD sunt un snapshot la init(): daca utilizatorul scoate cardul cat
// pagina este afisata, informatiile raman cele de la intrare (nu se mai
// fac accese la card din aceasta pagina, deci nu exista risc de blocaj).
// ============================================================================

SdCheckScreen::SdCheckScreen(Adafruit_GFX* tftInstance)
    : _view(tftInstance)
    , _backHeld(false)
{
}

void SdCheckScreen::fillModel(SdCheckViewModel& m)
{
    memset(&m, 0, sizeof(m));

    m.sdAvailable = bsp_sdcard_is_mounted();
    if (!m.sdAvailable)
        return;

    m.sdTotalMB = bsp_sdcard_size_mb();
    m.sdFreeMB  = bsp_sdcard_free_mb();

    // Lista generoasa: primele SD_MAX_FILES se afiseaza, restul se numara.
    static char names[64][24];
    uint16_t total = bsp_sdcard_list_root(names, 64);

    m.fileCount = (total > SD_MAX_FILES) ? SD_MAX_FILES : (uint8_t)total;
    m.extraCount = (total > SD_MAX_FILES)
                 ? (uint8_t)((total - SD_MAX_FILES) > 255 ? 255 : (total - SD_MAX_FILES))
                 : 0;

    for (uint8_t i = 0; i < m.fileCount; i++) {
        strncpy(m.fileNames[i], names[i], 23);
        m.fileNames[i][23] = '\0';
    }
}

void SdCheckScreen::init()
{
    _backHeld = false;

    SdCheckViewModel m;
    fillModel(m);
    _view.renderFull(m);
}

bool SdCheckScreen::update(int touchX, int touchY, bool isTouched)
{
    const UiRect& r = _view.backRect();
    bool inBtn = (touchX >= r.x && touchX <= (r.x + r.w) &&
                  touchY >= r.y && touchY <= (r.y + r.h));

    if (isTouched) {
        _backHeld = inBtn;
        return false;
    }

    // Eliberare: confirmare doar daca apasarea a inceput in buton.
    if (_backHeld) {
        _backHeld = false;
        return true;
    }
    return false;
}
