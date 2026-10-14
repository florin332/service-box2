#ifndef DUT_PROTOCOL_H
#define DUT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// dut_protocol - clientul protocolului serial de service al DUT-ului
// (panou Marble Pico, "panou_duplex").
//
// Protocolul este DEFINIT de DUT (src/service_commands.md din repo-ul
// Panou-lift) si nu se schimba aici:
//   - linii terminate cu CR sau LF, case-sensitive
//   - o singura comanda in zbor (DUT raspunde ERR 02 BUSY daca e ocupat)
//   - raspunsuri: "OK ..." / "ACK ..." / "ERR <cod> <motiv>"
//   - SRV ENTER intoarce identificatorul "D_<hardware>_<major>.<minor>"
//     (ex. "D_01_10.28") - folosit ca handshake + versiune firmware
//   - SRV BOOTSEL (adaugata in firmware-ul DUT) comuta in bootloader;
//     ACK-ul poate sa nu apuca sa iasa - detach-ul CDC e confirmarea
//
// Ruleaza pe core0, peste transportul din dut_host.
// ============================================================================

// Trimite SRV ENTER si asteapta identificatorul DUT-ului.
// La succes: identity primeste sirul "D_..." (truncat la len-1).
// Returneaza false la lipsa legaturii, timeout sau raspuns neasteptat.
bool dut_query_identity(char* identity, size_t len, uint32_t timeoutMs = 2500);

// Trimite SRV BOOTSEL. DUT-ul poate disparea de pe USB inainte de ACK,
// deci rezultatul e informativ: true = ACK/OK primit, false = timeout/
// deconectare (frecvent la succes - verificarea reala e aparitia volumului
// MSC, urmarita de flasher).
bool dut_request_bootsel(uint32_t timeoutMs = 800);

// ---- Comenzi de sesiune (legatura dintre UI si starea de service a DUT) ----

// Trimite "mb_01" (alias SRV ENTER): DUT intra in Service Mode si raspunde
// cu identificatorul "D_<hardware>_<versiune>". La succes, identity primeste
// acel sir. false = DUT neconectat / timeout / raspuns neasteptat.
bool dut_enter_service(char* identity, size_t len, uint32_t timeoutMs = 2500);

// Trimite "mb_00" (alias SRV EXIT): DUT iese din Service Mode si revine la
// functionarea normala. Returneaza true daca s-a primit ACK-ul.
bool dut_exit_service(uint32_t timeoutMs = 1500);

// Apelata din callback-ul de unmount CDC (core1): reseteaza urmarirea
// starii de service a DUT-ului - legatura noua porneste din afara Service
// Mode.
void dut_protocol_on_disconnect();

// ---- PanelInfo: rezultatul ultimului handshake reusit ----------------------
// Stare de sesiune (core0): completata la apasarea butonului START prin
// handshake (mb_01) si invalidata la deconectare. UI o citeste; nu o scrie.
struct DutPanelInfo {
    bool valid;                 // true doar dupa un handshake cu raspuns D_
    char identity[24];          // sirul brut "D_<hardware>_<versiune>"
    char panelId[24];           // prefixul din identitate (pana la ultimul '_')
    char firmwareVersion[16];   // sufixul dupa ultimul '_' (versiunea)
};

const DutPanelInfo& dut_panel_info();

#endif // DUT_PROTOCOL_H
