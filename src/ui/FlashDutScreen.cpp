#include "FlashDutScreen.h"

#include <string.h>

#include "dut/dut_host.h"
#include "dut/dut_protocol.h"
#include "dut/dut_flash.h"
#include "dut/firmware_repository.h"
#include "bsp/bsp_sdcard.h"

// ============================================================================
// FlashDutScreen - implementare. Vezi header-ul pentru responsabilitati.
// ============================================================================

FlashDutScreen::FlashDutScreen(Adafruit_GFX* tftInstance)
    : _view(tftInstance)
    , _backHeld(false)
    , _flashHeld(false)
    , _rowHeld(-1)
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

    // Lista firmware-urilor vine din Repository, filtrata pe Panel ID-ul
    // din handshake-ul paginii START (regula: "<panelId>_*.uf2").
    const DutPanelInfo& panel = dut_panel_info();
    if (panel.valid) {
        _model.fileCount = (uint8_t)fw_repo_list_compatible(
            panel.panelId, _model.fileNames, FLASH_MAX_FILES);
        strncpy(_model.dutIdentity, panel.identity,
                sizeof(_model.dutIdentity) - 1);
    } else {
        _model.fileCount = 0;
    }

    _model.hostAvailable = dut_host_available();
    _model.dutConnected  = dut_cdc_connected();
    _wasConnected        = _model.dutConnected;

    if (!panel.valid) {
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

    // BACK: blocat in timpul flash-ului (iesirea ar lasa UI-ul inconsistent).
    if (isTouched && hitTest(_view.backRect(), touchX, touchY)) {
        _backHeld = true;
    } else if (!isTouched && _backHeld) {
        _backHeld = false;
        if (!busy) {
            return true;   // cerere BACK catre navigator
        }
    }

    if (busy) {
        // In timpul flash-ului ignoram restul controalelor.
        _rowHeld = -1;
        _flashHeld = false;
        return false;
    }

    // Selectie fisier (activare la eliberare, ca restul UI-ului)
    if (isTouched) {
        _rowHeld = -1;
        for (uint8_t i = 0; i < _model.fileCount; i++) {
            if (hitTest(_view.fileRowRect(i), touchX, touchY)) {
                _rowHeld = (int8_t)i;
                break;
            }
        }
        if (_rowHeld < 0 && hitTest(_view.flashRect(), touchX, touchY)) {
            _flashHeld = true;
        }
        return false;
    }

    // Eliberare
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
        bool enabled = _model.hostAvailable && _model.dutConnected &&
                       (_model.selected >= 0);
        if (enabled && hitTest(_view.flashRect(), touchX, touchY)) {
            const char* name = _model.fileNames[_model.selected];
            if (dut_flash_start(name)) {
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
