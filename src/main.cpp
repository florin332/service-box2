// ============================================================================
// Service Box - main.cpp
//
// Punct de intrare al aplicatiei. Dupa refactorizarea in 3 straturi + BSP:
//   - bsp_display / bsp_touch: factory-uri hardware per target
//   - ui/graphic:              renderer-e pasive (layout relativ, miimi)
//   - ui/navigation:           masina de stari a paginilor + reguli
//   - ui/actions:              executia efectiva (placeholder pentru teste)
//   - ui/calibration:          flux calibrare touch (separat, cu prioritate)
//
// main.cpp nu mai contine NICIO dependenta directa de tipul de display
// sau de controllerul de touch.
// ============================================================================

#include <Arduino.h>

#include "bsp/bsp_display.h"
#include "bsp/bsp_touch.h"
#include "bsp/bsp_battery.h"
#include "bsp/bsp_sdcard.h"
#include "battery/BspBatteryProvider.h"

#if defined(SERVICEBOX_WAVESHARE)
#include "bsp/bsp_ina219.h"
#endif

#include "ui/StartupScreen.h"
#include "ui/BatteryCheckScreen.h"
#include "ui/SdCheckScreen.h"
#include "ui/DashboardScreen.h"
#include "ui/FlashDutScreen.h"
#include "ui/navigation/PageNavigator.h"
#include "ui/navigation/UiEvents.h"
#include "ui/actions/ServiceActions.h"
#include "ui/calibration/TouchCalibration.h"

#include "dut/dut_host.h"
#include "dut/dut_flash.h"

// ============================================================================
// Instante globale
// ============================================================================

// Display abstract (tipul concret e in bsp_display)
static Adafruit_GFX& activeDisplay = *bsp_display_gfx();

// Battery Manager comun (citeste INA219; mod DEV prin jumper GP28)
static BspBatteryProvider batteryProvider;

// Pagini UI
static BatteryCheckScreen batteryCheckScreen(&activeDisplay, batteryProvider);
static StartupScreen      startupScreen(&activeDisplay, batteryProvider);
static SdCheckScreen      sdCheckScreen(&activeDisplay);
static DashboardScreen    dashboardScreen(&activeDisplay);
static FlashDutScreen     flashDutScreen(&activeDisplay);

// Navigator de pagini (masina de stari + reguli)
static PageNavigator navigator(startupScreen, batteryCheckScreen, sdCheckScreen,
                               dashboardScreen, flashDutScreen, batteryProvider);

// Actiuni (executia efectiva svc box)
static ServiceActions actions(batteryProvider);

// Flag: starea SD a fost trimisa ecranului START (se retrimite la fiecare
// revenire pe pagina - acopera si intoarcerea din SD Check).
static bool sdStatePushed = false;

// Calibrare touch (modul separat, prioritar fata de UI)
static TouchCalibration calibration;

// ============================================================================
// Arduino setup
// ============================================================================
void setup()
{
#if defined(SERVICEBOX_WAVESHARE)
    // Pornire pe baterie: mentine alimentarea dupa eliberarea BAT KEY.
    // Trebuie apelat primul, inainte de orice intarziere lunga.
    bsp_battery_init(3000);
#endif

    Serial.begin(115200);

    Serial.println();
    Serial.println("==============================================");
#if defined(SERVICEBOX_WAVESHARE)
    Serial.println("Service Box - graphic_ui / WAVESHARE RP2350");
#else
    Serial.println("Service Box - graphic_ui / MARBLE PICO");
#endif
    Serial.println("==============================================");
    Serial.flush();

    // Init display + touch prin factory-urile BSP
    Serial.println("BOOT: init display + touch...");
    Serial.flush();
    bsp_display_init();
    bsp_touch_init();

    // Calibrare: incarcare flag-uri EEPROM + aplicare pe bsp_touch +
    // configurare buton hardware recalibrare
    calibration.begin();

    Serial.print("Calibrare: ");
    Serial.println(calibration.isActive() ? "activa" : "completa");
    Serial.flush();

#if defined(SERVICEBOX_WAVESHARE)
    // Initializare INA219 (I2C1, adresa 0x40) - monitor curent/tensiune
    Serial.println("BOOT: init INA219 (0x40)...");
    Serial.flush();
    if (bsp_ina219_init())
    {
        ina219_data_t ina219_data;
        bsp_ina219_read(&ina219_data);
        Serial.print("INA219: Vbus=");
        Serial.print(ina219_data.bus_voltage_V, 3);
        Serial.print(" V, I=");
        Serial.print(ina219_data.current_mA, 1);
        Serial.print(" mA, P=");
        Serial.print(ina219_data.power_mW, 1);
        Serial.println(" mW");
    }
    else
    {
        Serial.println("INA219: absent (0x40 fara raspuns)");
    }
    Serial.flush();
#endif

    // Init SD card + self-test (Waveshare: SDIO GP19-24 | Marble: SPI GP16-19)
    // Nu blocheaza boot-ul daca lipseste cardul: esecul este doar raportat.
    Serial.println("BOOT: init SD card...");
    Serial.flush();
    bsp_sdcard_self_test();
    Serial.flush();

    if (calibration.isActive())
    {
        // Afisare pagina conform stadiului de calibrare
        calibration.renderCurrent();
    }
    else
    {
        // Montare card SD la boot (daca exista). Fara blocaje lungi:
        // cardul este pregatit pe PC, UI-ul nu are mount/format.
        Serial.println("BOOT: mount SD card (daca este prezent)...");
        Serial.flush();
        if (bsp_sdcard_mount())
        {
            Serial.printf("SD: montat - %lu MB total / %lu MB liber\n",
                          (unsigned long)bsp_sdcard_size_mb(),
                          (unsigned long)bsp_sdcard_free_mb());
        }
        else
        {
            Serial.println("SD: card absent sau eroare de montare");
        }
        Serial.flush();

        // BOOT -> Battery Check (prima pagina UI afisata).
        // navigator.begin() face intrarea formala in pagina: include
        // batteryCheckScreen.init() si reseteaza timpii de activitate
        // (altfel un setup > 5s ar consuma timeout-ul de 5s al paginii).
        batteryProvider.begin();
        navigator.begin();
    }
}

// ============================================================================
// Loop principal
// ============================================================================
void loop()
{
    // Battery Manager: citire INA219 / valori simulate DEV + comenzi seriale.
    batteryProvider.update();

    // Citire input - o singura data per iteratie de loop.
    // bsp_touch_read() aplica intern corectia de axe + maparea de calibrare.
    uint16_t uiTouchX = 0, uiTouchY = 0;
    bool uiTouched = bsp_touch_read(uiTouchX, uiTouchY);

    // Calibrarea are prioritate fata de UI-ul aplicatiei
    if (calibration.isActive())
    {
        calibration.update();
        return;
    }

    // Buton hardware recalibrare (doar in mod normal, calibrare completa)
    calibration.checkRecalibButton();

    // Stare SD catre ecranul START (o singura data per intrare in pagina):
    // updateStorageInfo() apeleaza si drawSdButton(), dar renderFull() este
    // apelat de navigator DUPA aceasta linie in aceeasi iteratie, deci
    // ordinea finala de desenare ramane corecta (renderFull acopera totul).
    if (navigator.currentPage() == UiPageId::START && !sdStatePushed)
    {
        startupScreen.updateStorageInfo(bsp_sdcard_free_mb(),
                                        bsp_sdcard_size_mb(),
                                        bsp_sdcard_is_mounted());
        sdStatePushed = true;
    }
    else if (navigator.currentPage() != UiPageId::START)
    {
        sdStatePushed = false;  // retrimite la urmatoarea intrare in START
    }

    // UI: navigatorul aplica regulile si deseneaza pagina curenta.
    // Evenimentele emise sunt executate de stratul Actions.
    navigator.update((int16_t)uiTouchX, (int16_t)uiTouchY, uiTouched);

    for (UiEvent ev = navigator.pollEvent(); ev != UiEvent::NONE;
         ev = navigator.pollEvent())
    {
        ActionOutcome outcome = actions.execute(ev);
        if (outcome.result != ActionResult::OK)
        {
            Serial.print("[Actions] rezultat neasteptat: ");
            Serial.println(outcome.detail);
        }
    }

    // Pump periodic pentru actiuni cu durata (placeholder pentru pipeline teste)
    actions.update();

    // Masina de stari a flasher-ului DUT (no-op cand nu e activa; pe
    // build-ul Marble este stub).
    dut_flash_update();
}

// ============================================================================
// Core1: host USB pe PIO (GP0/GP1) - doar Waveshare.
//
// Doar USBHost.task() ruleaza aici (prelucrarea bit-bang PIO ramane pe core1,
// core0 e liber pentru UI). Protocolul serial si scrierea MSC sunt apelate
// din contextul loop() de pe core0 - pattern-ul DualRole din Adafruit
// TinyUSB.
// ============================================================================
#if defined(SERVICEBOX_WAVESHARE)
void setup1()
{
    dut_host_core1_setup();
}

void loop1()
{
    dut_host_core1_loop();
}
#endif
