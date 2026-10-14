#ifndef FLASH_DUT_SCREEN_H
#define FLASH_DUT_SCREEN_H

#include <Arduino.h>
#include "graphic/FlashDutView.h"

// ============================================================================
// Pagina UI "Flash DUT" - flash-uire firmware .uf2 pe panoul conectat (DUT).
//
// Responsabilitati:
//   - la intrare: citeste lista firmware-urilor COMPATIBILE din Firmware
//     Repository (filtrate pe Panel ID din handshake-ul de la apasarea
//     butonului START; fara Panel ID valid nu exista firmware compatibil)
//   - selectia fisierului + butonul FLASH (activ doar cu DUT conectat si
//     fisier selectat); pornirea fluxului prin dut_flash_start()
//   - render() periodic: urmareste faza/progresul flasher-ului
//   - la terminare: DONE/FAILED -> navigare la START (panelul reboot-eaza
//     si iese singur din Service Mode; noul firmware este descoperit de
//     handshake-ul paginii START, nu de o verificare aici - prompt1.md);
//     BACK (fara flash) revine la DASHBOARD, cu sesiunea activa
//
// Comanda flash-ului este directa (dut_flash); navigatorul primeste de aici
// doar cererea de BACK/iesire.
// ============================================================================

class FlashDutScreen {
public:
    explicit FlashDutScreen(Adafruit_GFX* tftInstance);

    // Citeste starea initiala (fisiere + DUT) si deseneaza pagina completa.
    void init();

    // Interactiune tactila. Returneaza true o data, la eliberarea butonului
    // BACK (in afara unui flash in desfasurare). Finalul unui flux de flash
    // este semnalat separat, prin flashFinished(). Destinatiile le decide
    // navigatorul: BACK -> DASHBOARD; terminare flash -> START.
    bool update(int touchX, int touchY, bool isTouched);

    // Refresh dinamic (progres flash). Idempotent.
    void render();

private:
    FlashDutView      _view;
    FlashDutViewModel _model;

    bool    _backHeld;
    bool    _flashHeld;
    int8_t  _rowHeld;               // -1 = niciun rand apasat
    bool    _wasConnected;          // pentru detectia tranzitiilor de link
    uint8_t _lastProgressPct;       // anti-flicker pe zona de stare
    char    _lastStatusLine[48];

    void refreshDutState(bool allowQuery); // citeste link + identitate
    void setStatus(const char* text, uint16_t color);
    static bool hitTest(const UiRect& r, int x, int y);

public:
    // True o data, dupa terminarea unui flux de flash (DONE sau FAILED) -
    // navigatorul il consuma si muta pagina la START.
    bool flashFinished() const;
};

#endif // FLASH_DUT_SCREEN_H
