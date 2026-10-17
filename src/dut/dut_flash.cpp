// ============================================================================
// dut_flash.cpp - implementare flasher UF2. Vezi dut_flash.h pentru flux.
// ============================================================================

#include "dut_flash.h"

#include <Arduino.h>
#include <string.h>

#include "dut_host.h"
#include "dut_protocol.h"
#include "bsp/bsp_sdcard.h"

// ---- Constante UF2 ---------------------------------------------------------

static constexpr uint32_t UF2_MAGIC_START0  = 0x0A324655UL;  // "UF2\n"
static constexpr uint32_t UF2_MAGIC_START1  = 0x9E5D5157UL;
static constexpr uint32_t UF2_MAGIC_END     = 0x0AB16F30UL;
static constexpr uint32_t UF2_FLAG_FAMILYID = 0x00002000UL;
static constexpr uint32_t UF2_FAMILY_RP2040 = 0xE48BFF56UL;

// ---- Parametri flux --------------------------------------------------------

static constexpr uint16_t CHUNK_BYTES        = 4096;      // 8 sectoare/pas
static constexpr uint32_t WAIT_MSC_MS        = 20000UL;   // timp de intrare
                                                          // in bootloader
// Daca ACK-ul la SRV BOOTSEL a fost primit dar linkul CDC nu cade in acest
// interval, hostul PIO a ratat detach-ul electric al resetului rapid al
// DUT-ului -> se forteaza o re-enumerare software a portului.
static constexpr uint32_t REENUM_MS          = 2500UL;
// LBA de start pe volumul virtual RPI-RP2 (oricare - bootrom-ul ignora LBA
// pentru blocurile cu magic UF2; incepem dupa zona de metadate a FAT-ului).
static constexpr uint32_t WRITE_START_LBA    = 256;

// ---- Stare interna ---------------------------------------------------------

static DutFlashStatus s_status;
static uint8_t  s_chunk[CHUNK_BYTES];
static uint32_t s_phaseDeadline;
static uint32_t s_nextLba;
static bool     s_bootselAcked;   // DUT a confirmat SRV BOOTSEL
static bool     s_cdcDropped;     // linkul CDC a cazut => DUT s-a resetat
static bool     s_reenumForced;   // s-a fortat o re-enumerare a portului
static DutLink  s_lastLink;       // ultima stare de link logata in WAIT_MSC

static void setPhase(DutFlashPhase p)
{
    s_status.phase = p;
}

static void fail(const char* reason)
{
    bsp_sdcard_file_close();
    strncpy(s_status.error, reason, sizeof(s_status.error) - 1);
    s_status.error[sizeof(s_status.error) - 1] = '\0';
    setPhase(DutFlashPhase::FAILED);
    Serial.printf("DUT FLASH: ESUAT - %s\n", s_status.error);
}

// ---- Validare fisier UF2 ---------------------------------------------------

static bool validateUf2()
{
    uint32_t size = bsp_sdcard_file_size();
    if (0 == size || (size % 512) != 0) {
        fail("fisier invalid (dimensiune)");
        return false;
    }

    uint8_t hdr[512];
    if (bsp_sdcard_file_read(hdr, sizeof(hdr)) != sizeof(hdr)) {
        fail("citire fisier esuata");
        return false;
    }
    bsp_sdcard_file_rewind();

    uint32_t magicStart0, magicStart1, flags, familyId, magicEnd;
    memcpy(&magicStart0, hdr + 0,   4);
    memcpy(&magicStart1, hdr + 4,   4);
    memcpy(&flags,       hdr + 8,   4);
    memcpy(&familyId,    hdr + 28,  4);
    memcpy(&magicEnd,    hdr + 508, 4);

    if (magicStart0 != UF2_MAGIC_START0 || magicStart1 != UF2_MAGIC_START1 ||
        magicEnd != UF2_MAGIC_END) {
        fail("nu este fisier UF2");
        return false;
    }

    if (!(flags & UF2_FLAG_FAMILYID) || familyId != UF2_FAMILY_RP2040) {
        fail("nu este UF2 pentru RP2040");
        return false;
    }

    s_status.totalBlocks = size / 512;
    return true;
}

// ---- API public ------------------------------------------------------------

void dut_flash_init()
{
    bsp_sdcard_file_close();
    memset(&s_status, 0, sizeof(s_status));
    s_status.phase = DutFlashPhase::IDLE;
}

bool dut_flash_start(const char* uf2Name)
{
    if (dut_flash_busy())
        return false;

    dut_flash_init();

    if (NULL == uf2Name || '\0' == uf2Name[0]) {
        fail("niciun fisier selectat");
        return false;
    }

    if (!dut_cdc_connected()) {
        fail("DUT neconectat");
        return false;
    }

    strncpy(s_status.fileName, uf2Name, sizeof(s_status.fileName) - 1);
    s_status.fileName[sizeof(s_status.fileName) - 1] = '\0';

    if (!bsp_sdcard_file_open(uf2Name)) {
        fail("fisierul nu se deschide");
        return false;
    }

    if (!validateUf2()) {
        return false;  // motivul e deja in status.error
    }

    Serial.printf("DUT FLASH: start '%s' (%lu sectoare)\n",
                  s_status.fileName, (unsigned long)s_status.totalBlocks);
    setPhase(DutFlashPhase::IDENTIFY);
    return true;
}

bool dut_flash_busy()
{
    DutFlashPhase p = s_status.phase;
    return (p != DutFlashPhase::IDLE) &&
           (p != DutFlashPhase::DONE) &&
           (p != DutFlashPhase::FAILED);
}

const DutFlashStatus& dut_flash_status()
{
    return s_status;
}

// ---- Masina de stari ---------------------------------------------------------

void dut_flash_update()
{
    switch (s_status.phase) {

    case DutFlashPhase::IDENTIFY: {
        // Identitatea curenta este informativa (afisata la DONE); handshake-ul
        // sesiunii apartine paginii START, nu flasher-ului. Daca nu a fost
        // completata acolo, o citim acum (fara a reactiva Service Mode).
        if (s_status.identityBefore[0] == '\0') {
            dut_query_identity(s_status.identityBefore,
                               sizeof(s_status.identityBefore));
        }
        setPhase(DutFlashPhase::ENTER_BOOTSEL);
        break;
    }

    case DutFlashPhase::ENTER_BOOTSEL: {
        // ACK-ul poate sa nu apuca sa iasa; rezultatul este retinut pentru
        // diagnosticarea din WAIT_MSC (hang dupa ACK vs. fara ACK).
        // Daca DUT-ul are firmware vechi (fara comanda), utilizatorul poate
        // apasa BOOTSEL fizic - WAIT_MSC acopera ambele cazuri.
        s_bootselAcked = dut_request_bootsel();
        s_cdcDropped   = false;
        s_reenumForced = false;
        s_lastLink     = dut_link_state();
        s_phaseDeadline = millis() + WAIT_MSC_MS;
        setPhase(DutFlashPhase::WAIT_MSC);
        break;
    }

    case DutFlashPhase::WAIT_MSC: {
        if (dut_msc_dev_addr() != 0) {
            s_nextLba = WRITE_START_LBA;
            s_status.writtenBlocks = 0;
            setPhase(DutFlashPhase::WRITING);
            Serial.println("DUT FLASH: bootloader detectat, scriere...");
            break;
        }

        // Urmarirea linkului: caderea CDC-ului dovedeste ca DUT-ul s-a
        // resetat; persistenta lui dupa ACK dovedeste blocaj pe DUT.
        // Tranzitiile sunt logate - daca DUT-ul se reseteaza dar hostul
        // PIO-USB nu vede detach-ul, aici se va vedea (lipsa NONE/MSC).
        DutLink link = dut_link_state();
        if (link != s_lastLink) {
            Serial.printf("DUT FLASH: link %d -> %d\n",
                          (int)s_lastLink, (int)link);
            s_lastLink = link;
        }
        if (link == DutLink::NONE) {
            s_cdcDropped = true;
        }

        // Dupa ACK, DUT-ul se reseteaza imediat. Daca detach-ul electric nu
        // se vede (reset prea rapid pentru connection_check), fortam o
        // re-enumerare software a portului host - o singura data.
        if (s_bootselAcked && !s_cdcDropped && !s_reenumForced &&
            (int32_t)(millis() - (s_phaseDeadline - (WAIT_MSC_MS - REENUM_MS))) > 0) {
            s_reenumForced = true;
            Serial.println("DUT FLASH: detach nevazut dupa ACK - "
                           "fortez re-enumerarea hostului");
            dut_host_force_reenumerate();
        }

        // Dupa re-enumerarea fortata portul a revenit ca CDC (nu MSC):
        // DUT-ul este inca in aplicatie - resetul nu s-a produs.
        if (s_reenumForced && s_cdcDropped && link == DutLink::CDC) {
            fail("DUT nu s-a resetat dupa ACK - power-cycle panou");
            break;
        }
        if ((int32_t)(millis() - s_phaseDeadline) > 0) {
            if (s_cdcDropped) {
                fail("DUT resetat, dar volumul MSC nu a aparut");
            } else {
                fail("DUT nu a intrat in bootloader (apasa BOOTSEL fizic)");
            }
        }
        break;
    }

    case DutFlashPhase::WRITING: {
        if (dut_msc_dev_addr() == 0) {
            fail("volum MSC pierdut in timpul scrierii");
            break;
        }

        uint16_t bytes = bsp_sdcard_file_read(s_chunk, CHUNK_BYTES);
        if (bytes == 0) {
            // Sfarsit de fisier: toate blocurile au fost scrise. Bootloader-ul
            // reboot-eaza singur DUT-ul; verificarea noului firmware se face
            // prin handshake-ul paginii START, nu aici (conform prompt1.md).
            bsp_sdcard_file_close();
            setPhase(DutFlashPhase::DONE);
            Serial.println("DUT FLASH: scriere completa (FLASH_SUCCESS)");
            break;
        }

        // Dimensiunea este validata multiplu de 512, deci chunk-urile sunt
        // aliniate la sectoare.
        uint16_t blocks = (uint16_t)(bytes / 512);
        if (!dut_msc_write_blocks(s_nextLba, s_chunk, blocks)) {
            fail("scriere MSC esuata");
            break;
        }
        s_nextLba += blocks;
        s_status.writtenBlocks += blocks;
        break;
    }

    case DutFlashPhase::IDLE:
    case DutFlashPhase::DONE:
    case DutFlashPhase::FAILED:
    default:
        break;
    }
}
