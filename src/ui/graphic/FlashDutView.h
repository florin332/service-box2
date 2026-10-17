#ifndef FLASH_DUT_VIEW_H
#define FLASH_DUT_VIEW_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

#include "UiLayout.h"
#include "UiTheme.h"

// ============================================================================
// FlashDutView - renderer PASIV pentru pagina Flash DUT.
//
// Deseneaza exclusiv ce primeste in FlashDutViewModel: starea legaturii cu
// DUT-ul, lista fisierelor .uf2 de pe SD, selectia curenta si progresul
// flash-ului. Nu acceseaza SD, nu cunoaste USB, nu decide culori de stare
// (statusColor vine rezolvata de la screen).
// ============================================================================

constexpr uint8_t FLASH_MAX_FILES = 4;   // cati nume incap in lista

struct FlashDutViewModel {
    bool     hostAvailable;             // false pe build-ul Marble
    bool     dutConnected;              // link CDC activ cu DUT-ul
    bool     blankMsc;                  // DUT blank: enumerat direct ca MSC
    char     dutIdentity[24];           // "D_01_10.28" sau gol
    uint8_t  fileCount;                 // cate nume valide in fileNames
    char     fileNames[FLASH_MAX_FILES][24];
    int8_t   selected;                  // -1 = niciun fisier selectat
    bool     flashActive;               // flux in desfasurare
    uint8_t  progressPct;               // 0..100 (in timpul scrierii)
    char     statusLine[48];            // text de stare/eroare (rezolvat)
    uint16_t statusColor;               // culoarea textului de stare
    bool     batteryAvailable;          // INA219 prezent
    float    batteryVoltageV;
    uint16_t batteryColor;              // culoare indicator, rezolvata upstream
};

class FlashDutView {
public:
    explicit FlashDutView(Adafruit_GFX* gfx);

    // Desenare completa a paginii.
    void renderFull(const FlashDutViewModel& m);

    // Redesenari partiale (anti-flicker).
    void drawFileRow(const FlashDutViewModel& m, uint8_t idx);
    void drawFlashButton(const FlashDutViewModel& m);
    void drawStatus(const FlashDutViewModel& m);   // progres + linia de stare
    void drawBatteryIndicator(const FlashDutViewModel& m);

    // Zone de hit-test (calculate din layout).
    const UiRect& backRect()  const { return _backRect; }
    const UiRect& flashRect() const { return _flashRect; }
    const UiRect& fileRowRect(uint8_t i) const { return _fileRects[i]; }

private:
    Adafruit_GFX* _gfx;

    bool     _layoutReady;
    UiLayout _layout;
    UiRect   _backRect;
    UiRect   _flashRect;
    UiRect   _fileRects[FLASH_MAX_FILES];
    UiRect   _progressRect;
    int16_t  _btnRadius;
    int16_t  _dotRadius;

    void ensureLayout();
    void drawDutStatus(const FlashDutViewModel& m);
    void drawBackButton();
};

#endif // FLASH_DUT_VIEW_H
