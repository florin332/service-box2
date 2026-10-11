#ifndef UI_PAGE_ID_H
#define UI_PAGE_ID_H

#include <stdint.h>

// ============================================================================
// Identificatorii paginilor UI - unici pentru tot proiectul.
// Graful de tranzitii este definit in NavigationPolicy.
// ============================================================================

enum class UiPageId : uint8_t {
    BATTERY_CHECK,
    START,
    DASHBOARD,
};

#endif // UI_PAGE_ID_H
