// ============================================================================
// bsp_sdcard.cpp - implementare SD card per target.
//
// Waveshare RP2350: SDIO 4-bit pe PIO (GP19..GP24), biblioteca carlk3.
//   Pini derivați din PIO (offset-uri fixe fata de D0=21):
//     CLK = 19 (D0 - 2), CMD = 20, D1 = 22, D2 = 23, D3 = 24.
//
// Marble Pico:      SPI0 bit-bang (GP16=MISO, GP19=MOSI, GP18=SCK, CS=GP17).
//   Nu exista controler SPI hardware pe acesti pini la RP2040, de aceea
//   hw_inst = NULL (driverul carlk3 suporta SPI bit-bang).
//   Viteza SPI este limitata (bit-bang): folosim clk_sys / 12 (~11 MHz)
//   pentru siguranta; se poate urca dupa teste pe hardware.
//
// API public in bsp_sdcard.h. Tot ce tine de FatFS / hw_config ramane aici.
// ============================================================================

#include "bsp_sdcard.h"

#include <Arduino.h>
#include <stdio.h>

// Biblioteca carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico (FatFS + drivere SD)
extern "C" {
#include "f_util.h"
#include "ff.h"
#include "hw_config.h"
#include "sd_card.h"
#include "hardware/clocks.h"
#include "hardware/spi.h"
}

// ----------------------------------------------------------------------------
// Configuratie hardware per target (hw_config cerut de biblioteca)
// ----------------------------------------------------------------------------

#if defined(SERVICEBOX_WAVESHARE)

// SDIO nativ 4-bit. Pinii CLK/D1/D2/D3 NU se seteaza explicit - sunt
// derivati de PIO din D0_gpio (vezi rp2040_sdio.pio: CLK = D0-2, Dn = D0+n).
static sd_sdio_if_t sdio_if = {
    .CMD_gpio  = 20,
    .D0_gpio   = 21,
    .baud_rate = 125 * 1000 * 1000 / 6  // ~20.8 MHz; recalculat la init
};

static sd_card_t sd_card = {
    .type      = SD_IF_SDIO,
    .sdio_if_p = &sdio_if,
    .use_card_detect = false
};

#elif defined(SERVICEBOX_MARBLE)

// SPI0 bit-bang: GP16/18/19 nu mapeaza pe un controller SPI hardware RP2040,
// deci hw_inst = NULL. CS pe GP17. Card detect pe GP5 (de verificat pe HW).
static spi_t spi = {
    .hw_inst   = NULL,   // bit-bang SPI
    .miso_gpio = 16,
    .mosi_gpio = 19,
    .sck_gpio  = 18,
    .baud_rate = 125 * 1000 * 1000 / 12,  // ~10.4 MHz, sigur pentru bit-bang
    .spi_mode  = 0
};

static sd_spi_if_t spi_if = {
    .spi     = &spi,
    .ss_gpio = 17
};

static sd_card_t sd_card = {
    .type      = SD_IF_SPI,
    .spi_if_p  = &spi_if,
    .use_card_detect = false
};

#endif

// Functii obligatorii ale API-ului bibliotecii (hw_config.h)
size_t sd_get_num() { return 1; }

sd_card_t* sd_get_by_num(size_t num)
{
    return (0 == num) ? &sd_card : NULL;
}

// ----------------------------------------------------------------------------
// Stare interna
// ----------------------------------------------------------------------------

static bool   s_driver_initialized = false;
static bool   s_mounted            = false;
static FATFS  s_fs;

// ----------------------------------------------------------------------------
// API public
// ----------------------------------------------------------------------------

bool bsp_sdcard_init()
{
    if (s_driver_initialized)
        return true;

#if defined(SERVICEBOX_WAVESHARE)
    // Recalculam baud_rate din frecventa reala a sistemului (RP2350: 150 MHz
    // default pe rpipico2), nu hardcodat la 125 MHz.
    sdio_if.baud_rate = clock_get_hz(clk_sys) / 6;
    Serial.printf("SD: SDIO baud_rate = %lu Hz (clk_sys = %lu Hz)\n",
                  (unsigned long)sdio_if.baud_rate,
                  (unsigned long)clock_get_hz(clk_sys));
#endif

    if (!sd_init_driver())
    {
        Serial.println("SD: sd_init_driver FAILED");
        return false;
    }

    s_driver_initialized = true;
    Serial.println("SD: driver initializat");
    return true;
}

bool bsp_sdcard_mount()
{
    if (s_mounted)
        return true;

    if (!bsp_sdcard_init())
        return false;

    FRESULT fr = f_mount(&s_fs, "0:", 1);
    if (FR_OK != fr)
    {
        Serial.printf("SD: f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }

    s_mounted = true;
    Serial.println("SD: volum montat (0:)");
    return true;
}

void bsp_sdcard_unmount()
{
    if (!s_mounted)
        return;

    f_unmount("0:");
    s_mounted = false;
    Serial.println("SD: volum demontat");
}

bool bsp_sdcard_is_mounted()
{
    return s_mounted;
}

uint32_t bsp_sdcard_size_mb()
{
    if (!s_mounted)
        return 0;

    DWORD fre_clust = 0;
    FATFS* fs_p = NULL;
    FRESULT fr = f_getfree("0:", &fre_clust, &fs_p);
    if (FR_OK != fr || NULL == fs_p)
        return 0;

    // Sectoare totale = (intrari FAT - 2) * sectoare/cluster
    uint64_t tot_sect = (uint64_t)(fs_p->n_fatent - 2) * fs_p->csize;
    return (uint32_t)(tot_sect / 2 / 1024);  // 512 B/sector -> KiB -> MiB
}

uint32_t bsp_sdcard_free_mb()
{
    if (!s_mounted)
        return 0;

    DWORD fre_clust = 0;
    FATFS* fs_p = NULL;
    FRESULT fr = f_getfree("0:", &fre_clust, &fs_p);
    if (FR_OK != fr || NULL == fs_p)
        return 0;

    uint64_t fre_sect = (uint64_t)fre_clust * fs_p->csize;
    return (uint32_t)(fre_sect / 2 / 1024);
}

bool bsp_sdcard_self_test()
{
    Serial.println("SD: self-test inceput...");

    if (!bsp_sdcard_mount())
    {
        Serial.println("SD: self-test ESUAT (mount)");
        return false;
    }

    Serial.printf("SD: total %lu MB, liber %lu MB\n",
                  (unsigned long)bsp_sdcard_size_mb(),
                  (unsigned long)bsp_sdcard_free_mb());

    // Scriere
    FIL fil;
    const char* const filename = "svcbox_sd_test.txt";
    FRESULT fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr)
    {
        Serial.printf("SD: f_open(%s) error: %s (%d)\n",
                      filename, FRESULT_str(fr), fr);
        return false;
    }

    if (f_printf(&fil, "ServiceBox SD self-test @ %lu ms\n",
                 (unsigned long)millis()) < 0)
    {
        Serial.println("SD: f_printf failed");
        f_close(&fil);
        return false;
    }

    fr = f_close(&fil);
    if (FR_OK != fr)
    {
        Serial.printf("SD: f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }

    // Citire inapoi (verificare)
    fr = f_open(&fil, filename, FA_READ);
    if (FR_OK != fr)
    {
        Serial.printf("SD: f_open read error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }

    char buf[128];
    UINT br = 0;
    fr = f_read(&fil, buf, sizeof(buf) - 1, &br);
    f_close(&fil);

    if (FR_OK != fr || 0 == br)
    {
        Serial.println("SD: citire inapoi ESUATA");
        return false;
    }
    buf[br] = '\0';

    Serial.printf("SD: citit %u octeti, OK\n", (unsigned)br);
    Serial.println("SD: self-test REUSIT");
    return true;
}

uint16_t bsp_sdcard_list_root(char names[][24], uint16_t maxNames)
{
    if (!s_mounted || NULL == names || 0 == maxNames)
        return 0;

    DIR dir;
    FRESULT fr = f_opendir(&dir, "0:/");
    if (FR_OK != fr)
    {
        Serial.printf("SD: f_opendir error: %s (%d)\n", FRESULT_str(fr), fr);
        return 0;
    }

    uint16_t count = 0;
    FILINFO fno;
    while (count < maxNames)
    {
        fr = f_readdir(&dir, &fno);
        if (FR_OK != fr || '\0' == fno.fname[0])
            break;  // eroare sau sfarsit de director

        if (fno.fattrib & AM_DIR)
            continue;  // doar fisiere

        strncpy(names[count], fno.fname, 23);
        names[count][23] = '\0';
        count++;
    }

    f_closedir(&dir);
    return count;
}
