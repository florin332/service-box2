// ============================================================================
// dut_protocol.cpp - implementare client serial DUT. Vezi dut_protocol.h.
// ============================================================================

#include "dut_protocol.h"
#include "dut_host.h"

#include <Arduino.h>
#include <string.h>

// Arunca resturile din FIFO (ex. linii RAW dintr-un mod analyze ramas activ).
static void flushInput()
{
    dut_cdc_flush_rx();
}

// Urmareste daca Service Mode este activ pe DUT (setat de mb_01, sters de
// mb_00). Folosit pentru a nu re-trimite mb_01 inutil (comanda re-executa
// frame-ul de service pe ecranele DUT).
static bool s_serviceActive = false;

// PanelInfo: completat la fiecare handshake reusit (mb_01), invalidat la
// deconectarea DUT-ului.
static DutPanelInfo s_panelInfo;

static void updatePanelInfo(const char* identity)
{
    memset(&s_panelInfo, 0, sizeof(s_panelInfo));

    strncpy(s_panelInfo.identity, identity, sizeof(s_panelInfo.identity) - 1);

    // "D_<hardware>_<versiune>": Panel ID este prefixul pana la ULTIMUL '_',
    // versiunea este sufixul de dupa. Exemplu: "D_01_10.28" ->
    // panelId = "D_01", firmwareVersion = "10.28".
    const char* lastUnderscore = strrchr(identity, '_');
    if (NULL != lastUnderscore && lastUnderscore != identity &&
        '\0' != lastUnderscore[1]) {
        size_t idLen = (size_t)(lastUnderscore - identity);
        if (idLen >= sizeof(s_panelInfo.panelId))
            idLen = sizeof(s_panelInfo.panelId) - 1;
        memcpy(s_panelInfo.panelId, identity, idLen);
        s_panelInfo.panelId[idLen] = '\0';

        strncpy(s_panelInfo.firmwareVersion, lastUnderscore + 1,
                sizeof(s_panelInfo.firmwareVersion) - 1);

        s_panelInfo.valid = true;
    } else {
        // Format neasteptat: pastram identitatea bruta, dar PanelInfo nu
        // este valid (nu poate fi folosit pentru filtrarea firmware-urilor).
        Serial.printf("DUT: identitate fara format asteptat: '%s'\n", identity);
    }
}

const DutPanelInfo& dut_panel_info()
{
    return s_panelInfo;
}

void dut_protocol_on_disconnect()
{
    s_serviceActive = false;
    memset(&s_panelInfo, 0, sizeof(s_panelInfo));
}

bool dut_query_identity(char* identity, size_t len, uint32_t timeoutMs)
{
    if (NULL == identity || len < 3)
        return false;
    identity[0] = '\0';

    if (!dut_cdc_connected())
        return false;

    flushInput();

    // Daca START a activat deja Service Mode, mb_01 s-ar re-executa pe DUT
    // (re-trimiterea frame-ului de service pe ecrane). Preferam interogarea
    // pasiva a starii; mb_01 se trimite doar daca nu suntem in service.
    // Nota: versiunea exacta nu este expusa de o comanda de interogare -
    // in service mode afisam placeholder "in service".
    if (s_serviceActive) {
        if (!dut_cdc_write_line("mb_status"))
            return false;
        char resp[64];
        int n = dut_cdc_read_line(resp, sizeof(resp), timeoutMs);
        if (n > 0 && strncmp(resp, "OK", 2) == 0) {
            strncpy(identity, "in service", len - 1);
            identity[len - 1] = '\0';
            return true;
        }
        return false;
    }

    if (!dut_cdc_write_line("mb_01"))
        return false;

    char resp[64];
    int n = dut_cdc_read_line(resp, sizeof(resp), timeoutMs);
    if (n <= 0)
        return false;

    // Asteptam identificatorul "D_<hardware>_<versiune>".
    if (strncmp(resp, "D_", 2) != 0) {
        Serial.printf("DUT: raspuns neasteptat la mb_01: '%s'\n", resp);
        return false;
    }

    s_serviceActive = true;   // mb_01 a activat Service Mode pe DUT
    updatePanelInfo(resp);
    strncpy(identity, resp, len - 1);
    identity[len - 1] = '\0';
    return true;
}

bool dut_request_bootsel(uint32_t timeoutMs)
{
    if (!dut_cdc_connected())
        return false;

    flushInput();

    if (!dut_cdc_write_line("SRV BOOTSEL"))
        return false;

    char resp[64];
    int n = dut_cdc_read_line(resp, sizeof(resp), timeoutMs);
    // n == 0 (timeout) sau n == -1 (DUT a intrat deja in bootloader) sunt
    // ambele acceptabile; ACK/OK primit = confirmare explicita.
    if (n > 0) {
        Serial.printf("DUT: raspuns la SRV BOOTSEL: '%s'\n", resp);
        return (strncmp(resp, "ACK", 3) == 0) || (strncmp(resp, "OK", 2) == 0);
    }
    return false;
}

bool dut_enter_service(char* identity, size_t len, uint32_t timeoutMs)
{
    if (NULL != identity && len > 0)
        identity[0] = '\0';

    if (!dut_cdc_connected())
        return false;

    flushInput();

    if (!dut_cdc_write_line("mb_01"))
        return false;

    char resp[64];
    int n = dut_cdc_read_line(resp, sizeof(resp), timeoutMs);
    if (n <= 0)
        return false;

    // La succes DUT-ul trimite identificatorul "D_<hardware>_<versiune>".
    if (strncmp(resp, "D_", 2) != 0) {
        Serial.printf("DUT: raspuns neasteptat la mb_01: '%s'\n", resp);
        return false;
    }

    if (NULL != identity && len > 0) {
        strncpy(identity, resp, len - 1);
        identity[len - 1] = '\0';
    }
    s_serviceActive = true;
    updatePanelInfo(resp);
    Serial.printf("DUT: service mode activ (%s)\n", resp);
    return true;
}

bool dut_exit_service(uint32_t timeoutMs)
{
    if (!dut_cdc_connected())
        return false;

    flushInput();

    if (!dut_cdc_write_line("mb_00"))
        return false;

    char resp[64];
    int n = dut_cdc_read_line(resp, sizeof(resp), timeoutMs);
    if (n <= 0)
        return false;

    // Raspunsul documentat este "ACK SRV EXIT".
    bool ok = (strncmp(resp, "ACK", 3) == 0);
    if (!ok) {
        Serial.printf("DUT: raspuns neasteptat la mb_00: '%s'\n", resp);
    } else {
        s_serviceActive = false;
    }
    return ok;
}
