În service-box2, lucrăm pe următoarea arhitectură. Nu modifica nimic în afara cerințelor de mai jos și nu introduce abstracții/logică suplimentară fără să discutăm.

ARHITECTURĂ UI
- Graphics: doar desenare, layout, coordonate și hit-testing. Nu navighează și nu execută acțiuni.
- Navigation: pagini, tranziții, interblocări și politici. Nu desenează.
- Actions: execută operațiile și returnează rezultate. Nu schimbă singure pagina.
- Hardware/services rămân în afara UI; UI folosește interfețe minimale.

DISPLAY
- Nu crea IDisplay. Adafruit_GFX este deja abstracția display-ului.
- Elimină treptat coordonatele hardcodate; Graphics trebuie să folosească dimensiunea reală a display-ului.

TOUCH
Contract minim:
class ITouch {
  virtual void update() = 0;
  virtual bool isTouched() const = 0;
  virtual int getX() const = 0;
  virtual int getY() const = 0;
};
- X/Y sunt deja coordonate calibrate în pixeli.
- UI nu cunoaște CST328/XPT2046, raw values, pressure, calibrare etc.

BATTERY
- IBatteryProvider furnizează level, charging și BatteryState:
  NORMAL, LOW, CRITICAL, CHARGING.
- LOW: avertizare + nu se inițiază teste noi; operațiile deja pornite nu trebuie întrerupte brutal.
- CRITICAL: ieșire controlată din service și Battery Check.
- Battery Check: 5 s; NORMAL → Start; LOW/CRITICAL/CHARGING → standby; touch trezește pagina.
- Battery status este monitorizat permanent și există indicator pe fiecare pagină.
- Flash poate fi inițiat doar la minimum MEDIUM.
- Nu muta politica bateriei în Graphics.

START
- Start este un mini-loop de verificare.
- La intrare: battery → SD status → detectare device/panel → handshake → identificare.
- Înainte de handshake trebuie verificat că există device conectat.
- Panel ID și firmware-ul curent sunt obținute prin handshake.
- START devine disponibil doar când condițiile necesare sunt îndeplinite.

SD
- SD este intern; tehnicianul nu gestionează mount/unmount.
- Card insert → detectare → mount automat.
- Card remove → detectare → unmount/invalidate automat.
- UI vede doar statusul cardului: prezent/absent (și eventual error).
- Pe Start există doar un indicator simplu de prezență.
- Nu expune mount(), unmount(), filesystem sau spațiu liber către UI.

FIRMWARE REPOSITORY
- SD este doar mediul de stocare; aplicația lucrează cu Firmware Repository.
- Repository-ul oferă lista firmware-urilor compatibile cu Panel ID.
- Regula este simplă: Panel ID = prefixul numelui fișierului UF2.
  Exemplu: Panel ID DV2_123 → DV2_123_*.uf2.
- Filtrarea se face în Repository, nu în UI.
- UI poate afișa și selecta dintre mai multe UF2 compatibile de pe același card.

PANEL ID
- Handshake-ul transmite identificarea panelului și versiunea firmware curentă.
- Firmware version face parte din informația de identificare necesară UI-ului.
- Floor-ul NU face parte din Panel ID și nu trebuie transmis către box.

FLASH
Condiții de inițiere:
- panel conectat;
- Panel ID valid;
- UF2 compatibil disponibil;
- battery >= MEDIUM.
- Flash Action returnează doar FLASH_SUCCESS sau FLASH_ERROR.
- Progress-ul poate fi afișat în timpul flash-ului.
- După FLASH_SUCCESS: panel reboot → iese implicit din Service Mode → boxul merge direct la Start.
- Nu crea o pagină/mașină de stare proprie pentru reconnect/detect/handshake după flash.
- Nu introduce timere/recovery logică suplimentară pentru cazul în care panelul nu mai pornește după flash. Start își execută normal logica sa.
- Firmware-ul nou va fi cunoscut prin următorul handshake și poate fi afișat în Start; nu este necesară o verificare separată pe pagina Flash.

FLOOR SET
- Scrie noua setare de etaj în flash-ul panelului.
- Panelul reboot-ează și iese implicit din Service Mode.
- Boxul merge direct la Start.
- Nu este necesară confirmarea vizuală a etajului pe box.
- Panelul afișează singur etajul setat.
- Nu adăuga etajul în Panel ID și nu cere panelului să îl transmită către box.

PanelService deține starea și mecanismele de comunicare ale panelului. Navigation deține deciziile și interblocările bazate pe această stare. Graphics doar o afișează.

Cazurile de eroare, cum ar fi eșecul handshake-ului sau deconectarea panelului în timpul paginii Start, nu trebuie să introducă stări, timere sau logică de recuperare suplimentare decât dacă acestea sunt definite explicit. Service-ul furnizează rezultatul/starea, iar Navigation o tratează doar conform cerințelor deja stabilite.

PanelService deține starea și mecanismele de comunicare ale panelului. Navigation deține deciziile și interblocările bazate pe această stare. Graphics doar o afișează.. PanelService este hardware dependent, nu va fi implementat acum.
PanelService
 ├─ isConnected()
 ├─ getPanelId()
 ├─ getFirmwareVersion()
 └─ ... comunicarea/handshake-ul real

PanelService
     ↓
 Navigation
     ↓
device connected?
Panel ID valid?
firmware disponibil?
battery OK?
     ↓
poate intra în Flash?

Start declanșează/coordonează procesul de detectare + handshake, dar nu ar trebui să implementeze protocolul hardware.
Navigation
    ↓
PanelService.detect()
    ↓
PanelService.handshake()
    ↓
PanelInfo
    ├── connected
    ├── panelId
    └── firmwareVersion


REGULĂ GENERALĂ
Păstrează arhitectura simplă și orientată pe funcționalitatea reală a Service Box-ului. Nu transforma mecanismele interne de hardware în concepte UI și nu adăuga stări, timere, interfețe sau verificări doar „pentru siguranță” fără o cerință concretă.