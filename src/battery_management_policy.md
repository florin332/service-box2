# Battery Management Policy

Battery Manager este comun pentru **Marble Pico** și **Waveshare RP2350**.
Sursa unică de măsurare este **INA219** (I2C, adresa `0x40`).
NU se folosesc ADC-ul bateriei (GP27) sau pinii digitali GP23/GP24 pentru
decizii de stare.

Implementare: [src/battery/BspBatteryProvider.cpp](src/battery/BspBatteryProvider.cpp)

---

## 1. Mașina de stări

Stările sunt evaluate pe baza tensiunii bus (V) și curentului (I) de la
INA219, cu următoarea prioritate:

| Stare | Condiție | Comportament |
|---|---|---|
| **CRITICAL** | V ≤ 3,40 V | Blochează testele; `main.cpp` revine forțat la Battery Check |
| **LOW** | 3,40 V < V ≤ 3,60 V | Avertizare; blochează pornirea testelor noi. Ieșire **numai** la V ≥ 3,70 V (histerezis) |
| **CHARGED** | V > 4,00 V și −0,5 mA < I < 5 mA | Semnalează finalizarea încărcării; avansare automată ca NORMAL |
| **CHARGING** | V > 4,00 V și I < −50 mA | Semnalează încărcarea activă; rămâne în Battery Check |
| **NORMAL** | restul | Funcționare normală |

Ordinea de evaluare: `CRITICAL > LOW > CHARGED > CHARGING > NORMAL`.

### Histerezis LOW

- Intrare în LOW: V ≤ 3,60 V
- Ieșire din LOW: numai la V ≥ 3,70 V

Exemplu: V = 3,65 V menține starea LOW dacă aceasta era activă.

### Anti-oscilație (debounce)

O tranziție de stare este acceptată numai după **4 citiri consecutive**
care o cer (~2 s la intervalul de sampling de 500 ms).

---

## 2. Afișare UI

- În funcționare normală sau CHARGING/CHARGED, UI-ul afișează
  **tensiunea bateriei** (`X.XX V`), nu procentul.
- Punctul indicator colorat folosește procentul derivat din tensiune
  (mapare liniară: 3,40 V = 0 % … 4,20 V = 100 %).

---

## 3. Jumper DEVELOPMENT — GP28

- **GP28 = 0** (legat la GND): mod **DEVELOPMENT**
- **GP28 = 1** (pull-up intern): funcționare normală

Configurare: `INPUT_PULLUP`, citit o singură dată în `setup()`.

În mod DEVELOPMENT valorile INA219 reale sunt înlocuite cu valori
simulate (V, I). Mașina de stări rulează identic pe datele simulate.
Valorile simulate sunt **persistate în EEPROM** și supraviețuiesc
reboot-ului.

### Comenzi seriale (active doar în DEVELOPMENT)

| Comandă | Efect |
|---|---|
| `bat setv <volts>` | Tensiune simulată, ex. `bat setv 3.55` |
| `bat seti <mA>` | Curent simulat, ex. `bat seti -120` |
| `bat preset critical` | V = 3,30 V |
| `bat preset low` | V = 3,55 V (intrare LOW) |
| `bat preset recover` | V = 3,75 V (ieșire LOW, test histerezis) |
| `bat preset normal` | V = 3,90 V |
| `bat preset charging` | V = 4,10 V, I = −120 mA |
| `bat preset charged` | V = 4,15 V, I = 0 mA |
| `bat status` | Afișează V, I, stare, mod |
| `bat help` | Lista comenzilor |

Toate comenzile de setare salvează valorile în EEPROM.

---

## 4. Politica de navigație

- **CRITICAL**: din orice pagină, `main.cpp` revine forțat la
  Battery Check.
- **LOW**: butonul START afișează `BATT LOW` și blockează pornirea
  testelor noi; testele în desfășurare continuă.
- **CHARGING**: Battery Check nu avansează automat.
- **CHARGED** / **NORMAL**: Battery Check avansează automat către START
  după timeout de 5 s.
