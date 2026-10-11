#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include <stdint.h>

// ============================================================================
// UiLayout - helper de layout cu ancore, independent de rezolutie.
//
// Nicio coordonata in pixeli in codul view-urilor: pozitiile si dimensiunile
// se exprima in MIIMI (per-mille, 0..1000) din latimea/inaltimea display-ului,
// citite la runtime din Adafruit_GFX. Astfel aceeasi pagina se aseaza
// identic pe orice rezolutie si orientare.
//
// Elementele mici (raze, grosimi) folosesc scaleMin() - miimi din
// min(width, height) - pentru a ramane proportionale si in landscape.
// ============================================================================

struct UiRect {
    int16_t x, y, w, h;
};

enum class UiHAnchor { LEFT, CENTER, RIGHT };
enum class UiVAnchor { TOP, CENTER, BOTTOM };

class UiLayout {
public:
    UiLayout(uint16_t width, uint16_t height)
        : _w((int16_t)width), _h((int16_t)height)
    {
    }

    int16_t width()  const { return _w; }
    int16_t height() const { return _h; }

    // Miimi din latime / inaltime, cu rotunjire la cel mai apropiat pixel.
    int16_t px(uint16_t perMille) const { return (int16_t)((_w * perMille + 500) / 1000); }
    int16_t py(uint16_t perMille) const { return (int16_t)((_h * perMille + 500) / 1000); }

    // Miimi din dimensiunea minima (raze, puncte indicator, grosimi).
    int16_t scaleMin(uint16_t perMille) const
    {
        int16_t m = (_w < _h) ? _w : _h;
        return (int16_t)((m * perMille + 500) / 1000);
    }

    // Rect ancorat: punctul de ancorare (axPm, ayPm) si dimensiunile
    // (wPm, hPm) in miimi din W/H. Ancora decide la ce se raporteaza
    // coltul/centrul/muchia dreapta a rect-ului.
    UiRect anchoredRect(UiHAnchor ah, UiVAnchor av,
                        uint16_t axPm, uint16_t ayPm,
                        uint16_t wPm, uint16_t hPm) const
    {
        UiRect r;
        r.w = px(wPm);
        r.h = py(hPm);

        int16_t ax = px(axPm);
        int16_t ay = py(ayPm);

        switch (ah) {
            case UiHAnchor::LEFT:   r.x = ax;          break;
            case UiHAnchor::CENTER: r.x = ax - r.w/2;  break;
            case UiHAnchor::RIGHT:  r.x = ax - r.w;    break;
        }
        switch (av) {
            case UiVAnchor::TOP:    r.y = ay;          break;
            case UiVAnchor::CENTER: r.y = ay - r.h/2;  break;
            case UiVAnchor::BOTTOM: r.y = ay - r.h;    break;
        }
        return r;
    }

    // Cursor X pentru text centrat pe toata latimea display-ului.
    // Font GFX clasic: 6*textSize px latime/caracter.
    int16_t centerTextX(uint8_t textSize, uint8_t charCount) const
    {
        int16_t tw = charCount * 6 * textSize;
        return (int16_t)((_w - tw) / 2);
    }

private:
    int16_t _w;
    int16_t _h;
};

#endif // UI_LAYOUT_H
