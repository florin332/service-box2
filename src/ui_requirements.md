# SVC_BOX – BOOT / BATTERY CHECK

## Scop

Acest document definește exclusiv interfața și comportamentul necesar
pentru secvența:

BOOT → BATTERY CHECK

În această etapă NU se implementează nicio altă pagină sau funcție din
meniul SVC_BOX.

Structura generală a meniului este definită în:

service_box/svcbox_menu.drawio

---

## 1. BOOT

BOOT este o secvență internă de pornire.

Nu este o pagină UI navigabilă.

După finalizarea inițializării necesare, sistemul trece la:

BATTERY CHECK

---

## 2. BATTERY CHECK

Battery Check este prima pagină UI afișată după pornire.

Scopul paginii:

- citirea stării bateriei;
- afișarea nivelului bateriei;
- indicarea stării de încărcare.

### 2.1 Afișare normală

În funcționare normală se afișează:

BATTERY LEVEL

XX %

Valoarea `XX %` reprezintă nivelul bateriei furnizat de interfața
bateriei.

UI-ul nu trebuie să facă măsurarea tensiunii și nu trebuie să conțină
logica ADC pentru baterie.

---

### 2.2 Indicarea nivelului bateriei

Nivelul bateriei trebuie reprezentat vizual prin trei stări:

- LOW → roșu
- MEDIUM → galben
- GOOD → verde

Pragurile numerice exacte nu sunt stabilite în această etapă.

Logica de determinare a nivelului trebuie să fie separată de partea
grafică.

---

### 2.3 Încărcare

Dacă bateria este în curs de încărcare, pagina afișează:

CHARGING

În această stare:

- la expirarea timeout-ului de 5 secunde fără atingere se avansează
  automat la pagina următoare (START), la fel ca în funcționarea
  normală;
- atingerea ecranului resetează temporizarea de 5 secunde;
- atingerea ecranului nu execută nicio comandă și nu există butoane;
- pagina afișează textul CHARGING cât timp încărcarea este activă;
- punctul indicator își schimbă/pulsează culoarea în funcție de nivelul
  bateriei:
  - LOW → roșu;
  - MEDIUM → galben;
  - GOOD → verde;
  - baterie complet încărcată → verde continuu.

---

### 2.4 Stingerea și reaprinderea display-ului

Battery Check este afișată timp de 5 secunde după activarea sau
reactivarea ecranului.

După 5 secunde fără atingerea ecranului:

- dacă avansarea automată este permisă (NORMAL / CHARGING / CHARGED),
  se trece la pagina următoare (START);
- dacă avansarea automată este blocată (LOW / CRITICAL):
  - partea grafică principală a paginii se stinge;
  - rămâne activ doar punctul indicator al bateriei;
  - punctul continuă să indice starea bateriei.

În standby, o atingere a ecranului:

- reaprinde pagina Battery Check;
- nu reprezintă o comandă;
- nu modifică starea bateriei;
- resetează temporizarea de 5 secunde.

---

## 3. Touchscreen

Battery Check este o pagină exclusiv informativă.

Nu există:

- butoane;
- controale touchscreen;
- gesturi;



---

## 4. Interfața bateriei

Pagina trebuie să primească de la un nivel abstract următoarele informații:

- nivel baterie (%);
- starea de încărcare.

Implementarea inițială din `graphic_ui` va utiliza un STUB pentru baterie.



---

## 5. Ce se implementează acum

Se implementează exclusiv:

BOOT
↓
BATTERY CHECK

Pagina trebuie să poată fi văzută și testată în `graphic_ui`.

---

## 6. Ce NU se implementează acum

Nu se implementează:

- START;
- calibrare;
- verificarea pinului de calibrare;
- handshake;
- identificarea panoului;
- selecția firmware-ului;
- flash;
- communication;
- display test;
- MCU INFO;
- meniul principal;
- alte pagini SVC_BOX.

Nu se adaugă funcții sau tranziții care nu sunt definite în această
specificație.