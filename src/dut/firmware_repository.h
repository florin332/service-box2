#ifndef FIRMWARE_REPOSITORY_H
#define FIRMWARE_REPOSITORY_H

#include <stdint.h>

// ============================================================================
// firmware_repository - catalogul de firmware-uri de pe cardul SD.
//
// SD este doar mediul de stocare; aplicatia lucreaza cu Repository, nu cu
// fisiere. Regula de compatibilitate (conform docs/prompt1.md):
//
//   Panel ID = prefixul numelui fisierului UF2.
//   Exemplu: Panel ID "DV2_123" -> fisier "DV2_123_*.uf2"
//
// Filtrarea se face AICI, nu in UI. UI primeste lista deja filtrata.
// ============================================================================

// Listeaza firmware-urile compatibile cu panelId (prefix "<panelId>_",
// extensie .uf2, case-insensitive pe extensie). Numele sunt copiate in
// `names`; returneaza cate au fost copiate (max `maxNames`).
// Returneaza 0 daca SD-ul lipseste, panelId e gol sau nu exista potriviri.
uint16_t fw_repo_list_compatible(const char* panelId,
                                 char names[][24], uint16_t maxNames);

#endif // FIRMWARE_REPOSITORY_H
