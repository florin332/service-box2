#ifndef SD_CHECK_VIEW_H
#define SD_CHECK_VIEW_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

#include "UiLayout.h"
#include "UiTheme.h"

// ============================================================================
// SdCheckView - renderer PASIV pentru pagina SD Check.
//
// Deseneaza exclusiv ce primeste in SdCheckViewModel: starea cardului
// (montat/absent, capacitati) si lista de fisiere din radacina. Nu acceseaza
// cardul si nu cunoaste FatFS.
//
// Numele de fisiere vin deja formatate in model (bsp_sdcard_list_root),
// maxim SD_MAX_FILES intrari; daca sunt mai multe pe card, modelul poarta
// si un contor de rest (afisat ca "+N altele").
// ============================================================================

constexpr uint8_t SD_MAX_FILES = 10;   // cati nume incap in lista

struct SdCheckViewModel {
    bool     sdAvailable;              // card montat
    uint32_t sdTotalMB;
    uint32_t sdFreeMB;
    uint8_t  fileCount;                // cate nume valide in fileNames
    uint8_t  extraCount;               // "+N altele" (fisiere neafisate)
    char     fileNames[SD_MAX_FILES][24];
};

class SdCheckView {
public:
    explicit SdCheckView(Adafruit_GFX* gfx);

    // Desenare completa a paginii (fundal + titlu + status + lista + BACK).
    void renderFull(const SdCheckViewModel& m);

    // Zona de hit-test a butonului BACK (calculata din layout).
    const UiRect& backRect() const { return _backRect; }

private:
    Adafruit_GFX* _gfx;

    bool     _layoutReady;
    UiLayout _layout;
    UiRect   _backRect;
    int16_t  _btnRadius;

    void ensureLayout();
    void drawStatus(const SdCheckViewModel& m);
    void drawFileList(const SdCheckViewModel& m);
    void drawBackButton();
};

#endif // SD_CHECK_VIEW_H
