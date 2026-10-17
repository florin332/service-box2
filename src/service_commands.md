# Comenzi Service

Comenzile se trimit pe serial ca linii terminate cu `CR` sau `LF`. Parserul este case-sensitive și ignoră spațiile de la începutul liniei. Baud rate-ul configurat este `115200`.

Acest document este **specificația țintă și lista TODO** pentru firmware-ul panourilor (`panou_duplex`, `panou_cabina`). Meniul de service se definește pas cu pas în codul Service Box, apoi se aplică în panouri. Panourile din teren rulează firmware vechi, **fără meniu de service**, și sunt tratate de box ca DUT blank (doar flash-uire prin bootloader).

## Schemă de numerotare

Comenzile sunt grupate pe tipuri de acțiuni și numerotate secvențial `mb_NN`:

| Grup | Interval | Conținut |
| --- | --- | --- |
| 0 — Comenzi principale | `mb_00` – `mb_01` | Intrare / ieșire Service Mode |
| 1 — Flash firmware | `mb_02` | Reset în bootloader USB pentru programare |
| 2 — Setări persistente | `mb_03` – `mb_05` | Adresă panou, gain audio, default-uri |

Parserul recunoaște **exclusiv** forma `mb_NN`; nu există aliasuri.

**Convenție de răspuns:** panourile confirmă execuția cu `ACK_NN`, unde `NN` este numărul comenzii (ex. `mb_02` → `ACK_02`). Comenzile de interogare răspund cu `OK ...`.

**Stare implementare:** doar **Grup 1 (Flash)** este implementat în firmware-ul panourilor. Restul grupurilor sunt **planificate** și se stabilesc pe măsură ce se implementează meniul de service.

Comenzile marcate **necesită Service activ** răspund cu `ERR 03 NOT_IN_SERVICE` dacă Service Mode nu este activ. Coada de comenzi acceptă o singură comandă în așteptare; o comandă queued primită cât timp coada este ocupată răspunde cu `ERR 02 BUSY`. O comandă nerecunoscută răspunde cu `ERR 01 UNKNOWN_CMD`.

## Grup 0 — Comenzi principale — *(planificat)*

Controlul ciclului de viață al Service Mode. Toate celelalte grupe (cu excepțiile notate) sunt accesibile doar după `mb_01`.

| Comandă | Stare | Cerință | Răspuns / efect |
| --- | --- | --- | --- |
| `mb_01` | Planificat | Oricând | Activează Service Mode; identifică panoul, de exemplu `C_01_1.01` (tip, variantă hardware, variantă firmware). |
| `mb_00` | Planificat | Oricând | Dezactivează Service Mode; `ACK_00`. |

Explicații:
- **`mb_01`** — punctul de intrare în orice sesiune de service. Răspunsul cu identitatea panoului permite Service Box-ului să verifice compatibilitatea hardware/firmware înainte de a trimite alte comenzi.
- **`mb_00`** — închide sesiunea de service și redă panoul funcționării normale. Trebuie trimis la finalul oricărei intervenții, altfel panoul rămâne blocat în Service Mode.

## Grup 1 — Flash firmware — *(implementat)*

| Comandă | Stare | Cerință | Răspuns / efect |
| --- | --- | --- | --- |
| `mb_02` | **Implementat** | Service activ | `ACK_02`, apoi reset în bootloader USB (dispozitivul se re-enumerază ca volum MSC `RPI-RP2`, pentru flash-uire firmware de pe Service Box). ACK-ul poate lipsi dacă resetul survine înainte de golirea FIFO-ului USB. |

Explicații:
- **`mb_02`** — singura comandă din grupul de flash. După reset, Service Box-ul scrie noul `.uf2` direct pe volumul MSC. Absența ACK-ului nu indică o eroare; succesul se confirmă prin apariția volumului `RPI-RP2`.

## Grup 2 — Setări persistente (flash protejat) — *(planificat)*

Valorile sunt stocate în zona de flash protejată (ultimul sector, în afara imaginii de firmware) și **nu se pierd la update-ul de firmware prin .uf2**. Toate comenzile din această secțiune necesită Service Mode activ și sunt executate direct în parserul serial.

Setul de valori diferă pe tipul de panou:
- **Panou cabină:** număr ascensor (`1`–`65535`) + gain audio (`0.00`–`4.00`).
- **Panou duplex:** etaj (`0`–`FLOORS-1`) + timeout standby ecran (ms).

Argumentul numeric poate fi trimis simplu (`mb_03 3`) sau încadrat de markerele protocolului (`mb_03 <3>`, `mb_03<3>`) — markerele sunt eliminate la parsare.

| Comandă | Stare | Efect / răspuns |
| --- | --- | --- |
| `mb_03` | Planificat | Interoghează adresa panoului: `OK LIFT <n>` (cabină) sau `OK ETJ <n>` (duplex). |
| `mb_03 <n>` | Planificat | Setează adresa panoului (număr ascensor la cabină, etaj la duplex) și o salvează în flash; `ACK_03 <n>`. Valori invalide: `ERR 08 INVALID_PARAM`. |
| `mb_04` | Planificat | Interoghează gain-ul audio (doar cabină): `OK GAIN <x.xx>`. |
| `mb_04 <x.xx>` | Planificat | Setează gain-ul audio (`0.00`–`4.00`), îl aplică imediat pe ieșirea I2S și îl salvează în flash; `ACK_04 <x.xx>`. Valori invalide: `ERR 08 INVALID_PARAM`. |
| `mb_05` | Planificat | Revine la default-urile din `Config.h` ale tipului de panou și le persistă; `ACK_05`. |

Explicații:
- **`mb_03`** — adresa panoului determină ce ascultă pe bus; fără argument interoghează, cu argument setează și persistă. Răspunsul de interogare dezvăluie și tipul panoului (`LIFT` = cabină, `ETJ` = duplex), util ca verificare încrucișată cu identitatea din `mb_01`.
- **`mb_04`** — gain-ul audio se aplică fără reboot, deci reglajul se poate face „live" în timpul testului acustic. Pe duplex comanda răspunde `ERR 01 UNKNOWN_CMD` (fără ieșire audio).
- **`mb_05`** — restaurare rapidă la valorile din fabrică după o configurare greșită; echivalent cu rescrierea valorilor cu default-urile compilate.

## Erori comune

| Răspuns | Semnificație |
| --- | --- |
| `ERR 01 UNKNOWN_CMD` | Comandă necunoscută, neimplementată pe acest tip de panou sau scrisă cu altă capitalizare. |
| `ERR 02 BUSY` | Coada de comenzi este ocupată. |
| `ERR 03 NOT_IN_SERVICE` | Comanda necesită Service Mode activ. |
| `ERR 08 INVALID_PARAM` | Parametru în afara intervalului valid. |

Alte coduri de eroare (`04`–`07`) vor fi reintroduse odată cu grupurile viitoare.

## TODO implementare panouri

Modificări necesare în `src/Core/Services/ServiceProtocol.cpp` și `src/Presentation/ServiceMenu.cpp` din fiecare proiect de panou, pentru alinierea la această specificație.

### TODO — panou_duplex

- [ ] **`mb_02` (critic):** parserul acceptă azi doar `SRV BOOTSEL` / `mb_boot` și răspunde `ACK SRV BOOTSEL`. De adăugat recunoaștere `mb_02` și răspuns `ACK_02`; de eliminat formele vechi. Box-ul trimite deja exclusiv `mb_02` — **flash-ul din box nu funcționează până la această modificare**. Markerii de diagnosticare `DBG P0/S1/S3/S4` rămân neschimbați (box-ul îi ignoră la așteptarea ACK-ului).
- [ ] **`mb_01` / `mb_00`:** păstrate, dar `mb_00` trebuie să răspundă `ACK_00` (azi `ACK SRV EXIT`). De eliminat aliasurile `SRV ENTER` / `SRV EXIT`.
- [ ] **`mb_03`:** redenumire `mb_etj` → `mb_03`; interogare `OK ETJ <n>`, setare `ACK_03 <n>` (azi `ACK ETJ <n>`).
- [ ] **`mb_05`:** redenumire `mb_defaults` → `mb_05`, răspuns `ACK_05` (azi `ACK DEFAULTS`).
- [ ] **Standby timeout:** `mb_standby` existent nu are echivalent în spec — de decis: mapare pe `mb_04` (rezervat gain-ului la cabină) sau comandă separată într-un grup viitor.
- [ ] **De eliminat din parser** până la definirea grupurilor viitoare: `COMM TEST`/`mb_com`, `mb_com_out`, `mb_status`, `mb_diag`, `mb_com_status`, `mb_com_status_out`, `comm_count_*`, `com_analyze_*`, `DISP TEST/REINIT`, `TEST EXIT`, `MCU *`. Vor răspunde temporar `ERR 01 UNKNOWN_CMD`.

### TODO — panou_cabina

- [ ] **`mb_02` (critic):** identic cu duplex — acceptare `mb_02`, răspuns `ACK_02`, eliminare `SRV BOOTSEL` / `mb_boot`.
- [ ] **`mb_01` / `mb_00`:** ca la duplex — `ACK_00`, fără aliasuri.
- [ ] **`mb_03`:** redenumire `mb_lift` → `mb_03`; interogare `OK LIFT <n>`, setare `ACK_03 <n>` (azi `ACK LIFT <n>`).
- [ ] **`mb_04`:** redenumire `mb_gain` → `mb_04`; setare `ACK_04 <x.xx>` (azi `ACK GAIN <x.xx>`). Formatarea manuală `X.XX` (newlib-nano fără `%f`) rămâne.
- [ ] **`mb_05`:** redenumire `mb_defaults` → `mb_05`, răspuns `ACK_05`. Atenție: handlerul actual oprește redarea audio și reaplică gain-ul — comportamentul se păstrează.
- [ ] **De eliminat din parser:** aceleași comenzi legacy ca la duplex (comunicație, contoare, analiză, display, MCU).
