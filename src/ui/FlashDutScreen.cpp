#include "FlashDutScreen.h"

#include <string.h>

#include "dut/dut_host.h"
#include "dut/dut_protocol.h"
#include "dut/dut_flash.h"
#include "dut/firmware_repository.h"
#include "bsp/bsp_sdcard.h"
#include "navigation/NavigationPolicy.h"

#if defined(SERVICEBOX_WAVESHARE)
#include "bsp/bsp_ina219.h"
#endif

// ============================================================================
// FlashDutScreen - implementare. Vezi header-ul pentru responsabilitati.
// ============================================================================

FlashDutScreen::FlashDutScreen(Adafruit_GFX* tftInstance)
    : _view(tftInstance)
    , _backHeld(false)
    , _flashHeld(false)
    , _rowHeld(-1)
    , _touchDown(false)
    , _releaseStartMs(0)
    , _wasConnected(false)
    , _lastProgressPct(255)   // fortat diferit la primul render
{
    memset(&_model, 0, sizeof(_model));
    _model.selected = -1;
    _lastStatusLine[0] = '\0';
}

void FlashDutScreen::setStatus(const char* text, uint16_t color)
{
    strncpy(_model.statusLine, text, sizeof(_model.statusLine) - 1);
    _model.statusLine[sizeof(_model.statusLine) - 1] = '\0';
    _model.statusColor = color;
}

void FlashDutScreen::refreshDutState(bool allowQuery)
{
    _model.hostAvailable = dut_host_available();

    bool connected = dut_cdc_connected();
    _model.dutConnected = connected;
    // In modul blank (MSC direct) flag-ul ramane setat din init(); nu este
    // reevaluat aici - pagina este statica, revenirea la START reface starea.

    if (connected && (!_wasConnected || allowQuery)) {
        // Identitatea (versiunea curenta) - blocant max ~2.5s, doar la
        // tranzitii/intrarea in pagina, nu in bucla de refresh.
        dut_query_identity(_model.dutIdentity, sizeof(_model.dutIdentity));
    }
    if (!connected) {
        _model.dutIdentity[0] = '\0';
    }
    _wasConnected = connected;
}

void FlashDutScreen::init()
{
    dut_flash_init();

    memset(&_model, 0, sizeof(_model));
    _model.selected = -1;
    _lastProgressPct = 255;
    _lastStatusLine[0] = '\0';
    _backHeld = _flashHeld = false;
    _rowHeld = -1;
    _touchDown = false;
    _releaseStartMs = 0;

    // Lista firmware-urilor vine din Repository, filtrata pe Panel ID-ul
    // din handshake-ul de la apasarea START (regula: "<panelId>_*.uf2").
    const DutPanelInfo& panel = dut_panel_info();
    bool blankMsc = !panel.valid && !dut_cdc_connected() &&
                    (dut_msc_dev_addr() != 0);
    bool legacyDut = !panel.valid && !blankMsc && dut_cdc_connected();

    if (blankMsc) {
        // Flux "blank DUT": panou nou, enumerat direct ca volum MSC
        // (RPI-RP2), fara CDC si fara identitate. Nu exista Panel ID,
        // deci listam TOATE .uf2-urile; utilizatorul alege explicit.
        _model.fileCount = (uint8_t)fw_repo_list_all(_model.fileNames,
                                                     FLASH_MAX_FILES);
        strncpy(_model.dutIdentity, "blank (bootloader)",
                sizeof(_model.dutIdentity) - 1);
    } else if (legacyDut) {
        // Flux "legacy": firmware vechi fara service mode - CDC-ul
        // raspunde, dar mb_01 nu intoarce identitate. Fara Panel ID nu se
        // poate filtra: listam TOATE .uf2-urile. SRV BOOTSEL nu va fi
        // recunoscut, deci intrarea in bootloader se face cu butonul fizic
        // (WAIT_MSC din dut_flash acopera acest caz, timeout 20 s).
        _model.fileCount = (uint8_t)fw_repo_list_all(_model.fileNames,
                                                     FLASH_MAX_FILES);
        strncpy(_model.dutIdentity, "legacy", sizeof(_model.dutIdentity) - 1);
    } else if (panel.valid) {
        _model.fileCount = (uint8_t)fw_repo_list_compatible(
            panel.panelId, _model.fileNames, FLASH_MAX_FILES);
        strncpy(_model.dutIdentity, panel.identity,
                sizeof(_model.dutIdentity) - 1);
    } else {
        _model.fileCount = 0;
    }
    _model.blankMsc  = blankMsc;
    _model.legacyDut = legacyDut;

    _model.hostAvailable = dut_host_available();
    _model.dutConnected  = dut_cdc_connected();
    _wasConnected        = _model.dutConnected;

    // Indicator baterie (snapshot la intrarea in pagina; culoarea rezolvata
    // prin aceleasi reguli ca in pagina START).
#if defined(SERVICEBOX_WAVESHARE)
    ina219_data_t bat;
    bsp_ina219_read(&bat);
    _model.batteryAvailable = true;
    _model.batteryVoltageV  = bat.bus_voltage_V;
    float bv = bat.bus_voltage_V;
    int pct = (int)((bv - 3.40f) / (4.20f - 3.40f) * 100.0f + 0.5f);
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    _model.batteryColor = NavigationPolicy::indicatorColor(
        (uint8_t)pct, bat.current_mA < -50.0f);
#else
    _model.batteryAvailable = false;
    _model.batteryVoltageV  = 0.0f;
    _model.batteryColor     = UiTheme::COLOR_TEXT_MUTED;
#endif

    if (blankMsc) {
        if (0 == _model.fileCount) {
            setStatus("Niciun .uf2 pe SD", UiTheme::COLOR_YELLOW);
        } else {
            setStatus("DUT blank: alege firmware (nefiltrat)",
                      UiTheme::COLOR_YELLOW);
        }
    } else if (legacyDut) {
        if (0 == _model.fileCount) {
            setStatus("Niciun .uf2 pe SD", UiTheme::COLOR_YELLOW);
        } else {
            setStatus("Firmware vechi: alege, apoi apasa BOOTSEL fizic",
                      UiTheme::COLOR_YELLOW);
        }
    } else if (!panel.valid) {
        setStatus("Handshake lipsa - revino la START", UiTheme::COLOR_YELLOW);
    } else if (0 == _model.fileCount) {
        setStatus("Niciun firmware compatibil pe SD", UiTheme::COLOR_YELLOW);
    } else {
        setStatus("Alege firmware-ul", UiTheme::COLOR_TEXT_MUTED);
    }

    _view.renderFull(_model);
}

bool FlashDutScreen::hitTest(const UiRect& r, int x, int y)
{
    return (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h);
}

bool FlashDutScreen::update(int touchX, int touchY, bool isTouched)
{
    bool busy = dut_flash_busy();

    // ---- Filtru touch ------------------------------------------------------
    // Armare doar pe frontul crescator al atingerii; eliberarea este
    // confirmata dupa RELEASE_CONFIRM_MS milisecunde consecutive fara touch.
    // Fara acest filtru, un glitch din citirea CST328 era interpretat ca
    // eliberare si comuta selectia / pornea actiunea in timp ce degetul
    // era inca pe ecran.
    if (isTouched) {
        _releaseStartMs = 0;
        if (_touchDown) {
            return false;   // tinere apasata - nu se re-armeaza nimic
        }
        _touchDown = true;

        // Front crescator: armam cel mult un control.
        _rowHeld   = -1;
        _flashHeld = false;
        _backHeld  = hitTest(_view.backRect(), touchX, touchY);

        if (!_backHeld && !busy) {
            for (uint8_t i = 0; i < _model.fileCount; i++) {
                if (hitTest(_view.fileRowRect(i), touchX, touchY)) {
                    _rowHeld = (int8_t)i;
                    break;
                }
            }
            if (_rowHeld < 0) {
                _flashHeld = hitTest(_view.flashRect(), touchX, touchY);
            }
        }
        return false;
    }

    // Fara atingere: asteptam confirmarea eliberarii (pe timp).
    if (!_touchDown) {
        return false;
    }
    uint32_t nowMs = millis();
    if (_releaseStartMs == 0) {
        _releaseStartMs = nowMs;   // primul cadru fara atingere
        return false;
    }
    if ((uint32_t)(nowMs - _releaseStartMs) < RELEASE_CONFIRM_MS) {
        return false;
    }
    _touchDown = false;
    _releaseStartMs = 0;

    // ---- Eliberare confirmata ----------------------------------------------
    if (_backHeld) {
        _backHeld = false;
        _rowHeld = -1;
        _flashHeld = false;
        if (!busy) {
            return true;   // cerere BACK catre navigator
        }
        return false;      // BACK blocat in timpul flash-ului
    }

    if (busy) {
        // In timpul flash-ului ignoram restul controalelor.
        _rowHeld = -1;
        _flashHeld = false;
        return false;
    }

    if (_rowHeld >= 0) {
        int8_t row = _rowHeld;
        _rowHeld = -1;
        if (hitTest(_view.fileRowRect(row), touchX, touchY)) {
            _model.selected = (_model.selected == row) ? -1 : row;
            _view.drawFileRow(_model, row);
            _view.drawFlashButton(_model);
        }
    }

    if (_flashHeld) {
        _flashHeld = false;
        bool enabled = _model.hostAvailable &&
                       (_model.dutConnected || _model.blankMsc) &&
                       (_model.selected >= 0);
        if (enabled && hitTest(_view.flashRect(), touchX, touchY)) {
            const char* name = _model.fileNames[_model.selected];
            bool started = _model.blankMsc ? dut_flash_start_direct(name)
                                           : dut_flash_start(name);
            if (started) {
                _model.flashActive = true;
                setStatus("Pornire flash...", UiTheme::COLOR_TEXT_MUTED);
                _view.drawStatus(_model);
            } else {
                // Validarea a esuat - motivul e in statusul flasher-ului.
                setStatus(dut_flash_status().error, UiTheme::COLOR_RED);
                _view.drawStatus(_model);
            }
            _view.drawFlashButton(_model);
        }
    }

    return false;
}

void FlashDutScreen::render()
{
    bool busy = dut_flash_busy();

    if (busy || _model.flashActive) {
        // Urmarire flasher: faza + progresul
        const DutFlashStatus& fs = dut_flash_status();

        _model.progressPct = (fs.totalBlocks > 0)
            ? (uint8_t)((fs.writtenBlocks * 100UL) / fs.totalBlocks)
            : 0;

        char line[48];
        switch (fs.phase) {
            case DutFlashPhase::IDENTIFY:
                setStatus("Identificare DUT...", UiTheme::COLOR_TEXT_MUTED);
                break;
            case DutFlashPhase::ENTER_BOOTSEL:
            case DutFlashPhase::WAIT_MSC:
                setStatus("Astept bootloader...", UiTheme::COLOR_YELLOW);
                break;
            case DutFlashPhase::WRITING:
                snprintf(line, sizeof(line), "Scriere %u%%",
                         (unsigned)_model.progressPct);
                setStatus(line, UiTheme::COLOR_TEXT_MAIN);
                break;
            case DutFlashPhase::DONE:
                _model.flashActive = false;
                setStatus("FLASH_SUCCESS", UiTheme::COLOR_GREEN);
                break;
            case DutFlashPhase::FAILED:
                _model.flashActive = false;
                setStatus(fs.error, UiTheme::COLOR_RED);
                break;
            default:
                break;
        }

        bool changed = (_model.progressPct != _lastProgressPct) ||
                       (strcmp(_model.statusLine, _lastStatusLine) != 0);
        if (changed) {
            _lastProgressPct = _model.progressPct;
            strncpy(_lastStatusLine, _model.statusLine,
                    sizeof(_lastStatusLine) - 1);
            _lastStatusLine[sizeof(_lastStatusLine) - 1] = '\0';
            _view.drawStatus(_model);
        }
        return;
    }

    // In afara flash-ului: pagina este statica (lista a fost citita la
    // intrare; starea legaturii se reface prin revenirea la START).
}

bool FlashDutScreen::flashFinished() const
{
    const DutFlashStatus& fs = dut_flash_status();
    return !_model.flashActive &&
           (fs.phase == DutFlashPhase::DONE || fs.phase == DutFlashPhase::FAILED);
}
