#ifndef BSP_DISPLAY_H
#define BSP_DISPLAY_H

#include <stdint.h>
#include <Adafruit_GFX.h>

// ============================================================================
// bsp_display - factory unic pentru display, per target.
//
// Singurul punct din proiect care cunoaste tipul concret de display:
//   SERVICEBOX_WAVESHARE -> ST7789T3 (SPI1)
//   SERVICEBOX_MARBLE    -> ILI9341 (SPI1, magistrala comuna cu touch)
//
// Restul codului (UI, calibrare) vede exclusiv Adafruit_GFX* si
// dimensiunile la runtime - nu exista nicio dependenta de driver.
// ============================================================================

// Initializare completa: pini, magistrala SPI, secventa de init controller,
// backlight pornit. Apelata o singura data in setup().
void bsp_display_init();

// Instanta display (construita lazy, la primul apel - sigur pentru
// initializarea globalelor din alte unitati de translatare).
Adafruit_GFX* bsp_display_gfx();

// Dimensiuni display (pixeli, orientarea curenta).
uint16_t bsp_display_width();
uint16_t bsp_display_height();

// Izolare chip-select pe magistrala SPI comuna (Marble: LCD + XPT2046).
// Pe Waveshare (magistrale separate) sunt no-op.
void bsp_display_select();
void bsp_display_deselect();

#endif // BSP_DISPLAY_H
