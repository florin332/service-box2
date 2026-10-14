// ============================================================================
// firmware_repository.cpp - implementare. Vezi header-ul pentru contract.
// ============================================================================

#include "firmware_repository.h"

#include <Arduino.h>
#include <string.h>

#include "bsp/bsp_sdcard.h"

// Capacitatea lotului brut citit de pe SD inainte de filtrare.
static constexpr uint16_t SCAN_BATCH = 32;

// True daca numele incepe cu "<panelId>_" si se termina cu ".uf2"
// (extensia este comparata case-insensitive; prefixul e case-sensitive,
// Panel ID fiind o valoare de protocol).
static bool isCompatible(const char* name, const char* panelId)
{
    size_t idLen   = strlen(panelId);
    size_t nameLen = strlen(name);

    // prefix + '_' + cel putin un caracter + ".uf2"
    if (nameLen < idLen + 1 + 1 + 4)
        return false;

    if (strncmp(name, panelId, idLen) != 0 || name[idLen] != '_')
        return false;

    const char* ext = name + nameLen - 4;
    const char* uf2 = ".uf2";
    for (uint8_t i = 0; i < 4; i++) {
        char a = ext[i], b = uf2[i];
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return false;
    }
    return true;
}

uint16_t fw_repo_list_compatible(const char* panelId,
                                 char names[][24], uint16_t maxNames)
{
    if (NULL == panelId || '\0' == panelId[0] || NULL == names ||
        0 == maxNames)
        return 0;

    if (!bsp_sdcard_is_mounted())
        return 0;

    // Lot brut de .uf2-uri, filtrat apoi pe prefixul Panel ID.
    static char all[SCAN_BATCH][24];
    uint16_t found = bsp_sdcard_list_by_ext(".uf2", all, SCAN_BATCH);

    uint16_t count = 0;
    for (uint16_t i = 0; i < found && count < maxNames; i++) {
        if (isCompatible(all[i], panelId)) {
            strncpy(names[count], all[i], 23);
            names[count][23] = '\0';
            count++;
        }
    }
    return count;
}
