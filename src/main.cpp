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
#include "battery/BspBatteryProvider.h"

#if defined(SERVICEBOX_WAVESHARE)
#include "bsp/bsp_ina219.h"
#endif

#include "ui/StartupScreen.h"
#include "ui/BatteryCheckScreen.h"
#include "ui/navigation/PageNavigator.h"
#include "ui/navigation/UiEvents.h"
#include "ui/actions/ServiceActions.h"
#include "ui/calibration/TouchCalibration.h"

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

// Navigator de pagini (masina de stari + reguli)
static PageNavigator navigator(startupScreen, batteryCheckScreen, batteryProvider);

// Actiuni (executia efectiva svc box)
static ServiceActions actions(batteryProvider);

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

    if (calibration.isActive())
    {
        // Afisare pagina conform stadiului de calibrare
        calibration.renderCurrent();
    }
    else
    {
        // BOOT -> Battery Check (prima pagina UI afisata)
        batteryProvider.begin();
        batteryCheckScreen.init();
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
}
