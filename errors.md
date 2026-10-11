 *  Executing task: C:\Users\Florin\.platformio\penv\Scripts\platformio.exe run --environment waveshare_rp2350 

Processing waveshare_rp2350 (platform: https://github.com/maxgerhardt/platform-raspberrypi.git; board: rpipico2; framework: arduino)
-----------------------------------------------------------------------------------------------------------------------------------------------------------Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/raspberrypi/rpipico2.html
PLATFORM: Raspberry Pi RP2040 (1.20.0+sha.9c167c6) > Pico 2
HARDWARE: RP2350 150MHz, 512KB RAM, 4MB Flash
DEBUG: Current (blackmagic) External (blackmagic, cmsis-dap, jlink, picoprobe, raspberrypi-swd)
PACKAGES: 
 - framework-arduinopico @ 1.60000.0+sha.9a0bc356 
 - tool-picotool-rp2040-earlephilhower @ 5.160100.260719 (16.1.0) 
 - tool-pioasm-rp2040-earlephilhower @ 5.160100.260719 (16.1.0) 
 - toolchain-rp2040-earlephilhower @ 5.160100.260719 (16.1.0)
Flash size: 4.00MB
Sketch size: 4.00MB
Filesystem size: 0.00MB
PSRAM size: 0.00MB
Maximium Sketch size: 4190208 EEPROM start: 0x103ff000 Filesystem start: 0x103ff000 Filesystem end: 0x103ff000
LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 90 compatible libraries
Scanning dependencies...
Dependency Graph
|-- Adafruit GFX Library @ 1.12.6
|-- Adafruit ST7735 and ST7789 Library @ 1.11.0
|-- Adafruit ILI9341 @ 1.6.3
|-- XPT2046_Touchscreen @ 1.4.0+sha.f956c5d
|-- CSE_CST328 @ 0.0.5
|-- EEPROM @ 1.0
|-- SPI @ 1.0
Building in release mode
Compiling .pio\build\waveshare_rp2350\FrameworkArduinoBootloader\none.S.o
Compiling .pio\build\waveshare_rp2350\lib205\Wire\Wire.cpp.o
Compiling .pio\build\waveshare_rp2350\lib3cb\SPI\SPI.cpp.o
Compiling .pio\build\waveshare_rp2350\libada\Adafruit BusIO\Adafruit_BusIO_Register.cpp.o
Compiling .pio\build\waveshare_rp2350\libada\Adafruit BusIO\Adafruit_GenericDevice.cpp.o
Compiling .pio\build\waveshare_rp2350\libada\Adafruit BusIO\Adafruit_I2CDevice.cpp.o
Compiling .pio\build\waveshare_rp2350\libada\Adafruit BusIO\Adafruit_SPIDevice.cpp.o
Compiling .pio\build\waveshare_rp2350\libbc2\Adafruit GFX Library\Adafruit_GFX.cpp.o
Compiling .pio\build\waveshare_rp2350\libbc2\Adafruit GFX Library\Adafruit_GrayOLED.cpp.o
Compiling .pio\build\waveshare_rp2350\libbc2\Adafruit GFX Library\Adafruit_SPITFT.cpp.o
Compiling .pio\build\waveshare_rp2350\libbc2\Adafruit GFX Library\glcdfont.c.o
Compiling .pio\build\waveshare_rp2350\lib643\Adafruit ST7735 and ST7789 Library\Adafruit_ST7735.cpp.o
Compiling .pio\build\waveshare_rp2350\lib643\Adafruit ST7735 and ST7789 Library\Adafruit_ST7789.cpp.o
Compiling .pio\build\waveshare_rp2350\lib643\Adafruit ST7735 and ST7789 Library\Adafruit_ST7796S.cpp.o
Compiling .pio\build\waveshare_rp2350\lib643\Adafruit ST7735 and ST7789 Library\Adafruit_ST77xx.cpp.o
Compiling .pio\build\waveshare_rp2350\lib700\Adafruit ILI9341\Adafruit_ILI9341.cpp.o
Compiling .pio\build\waveshare_rp2350\lib28f\XPT2046_Touchscreen\XPT2046_Touchscreen.cpp.o
Compiling .pio\build\waveshare_rp2350\lib4cb\CSE_Touch\CSE_Touch.cpp.o
Compiling .pio\build\waveshare_rp2350\lib816\CSE_CST328\CSE_CST328.cpp.o
Compiling .pio\build\waveshare_rp2350\lib4c4\EEPROM\EEPROM.cpp.o
Compiling .pio\build\waveshare_rp2350\src\battery\BspBatteryProvider.cpp.o
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_battery.c.o
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_cst328.cpp.o
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_display.cpp.o
.pio\libdeps\waveshare_rp2350\CSE_Touch\src\CSE_Touch.cpp: In static member function 'static CSE_Touch* CSE_Touch_Driver::createDriver(CSE_Touch_t, uint16_t, uint16_t, TwoWire*, uint8_t, uint8_t)':
.pio\libdeps\waveshare_rp2350\CSE_Touch\src\CSE_Touch.cpp:35:72: warning: unused parameter 'width' [-Wunused-parameter]
   35 | CSE_Touch* CSE_Touch_Driver:: createDriver (CSE_Touch_t type, uint16_t width, uint16_t height, TwoWire* wire, uint8_t pinRst, uint8_t pinIrq) {
      |                                                               ~~~~~~~~~^~~~~
.pio\libdeps\waveshare_rp2350\CSE_Touch\src\CSE_Touch.cpp:35:88: warning: unused parameter 'height' [-Wunused-parameter]
   35 | CSE_Touch* CSE_Touch_Driver:: createDriver (CSE_Touch_t type, uint16_t width, uint16_t height, TwoWire* wire, uint8_t pinRst, uint8_t pinIrq) {
      |                                                                               ~~~~~~~~~^~~~~~
.pio\libdeps\waveshare_rp2350\CSE_Touch\src\CSE_Touch.cpp:35:105: warning: unused parameter 'wire' [-Wunused-parameter]
   35 | CSE_Touch* CSE_Touch_Driver:: createDriver (CSE_Touch_t type, uint16_t width, uint16_t height, TwoWire* wire, uint8_t pinRst, uint8_t pinIrq) {
      |                                                                                                ~~~~~~~~~^~~~
.pio\libdeps\waveshare_rp2350\CSE_Touch\src\CSE_Touch.cpp:35:119: warning: unused parameter 'pinRst' [-Wunused-parameter]
   35 | CSE_Touch* CSE_Touch_Driver:: createDriver (CSE_Touch_t type, uint16_t width, uint16_t height, TwoWire* wire, uint8_t pinRst, uint8_t pinIrq) {
      |                                                                                                               ~~~~~~~~^~~~~~
.pio\libdeps\waveshare_rp2350\CSE_Touch\src\CSE_Touch.cpp:35:135: warning: unused parameter 'pinIrq' [-Wunused-parameter]
   35 | CSE_Touch* CSE_Touch_Driver:: createDriver (CSE_Touch_t type, uint16_t width, uint16_t height, TwoWire* wire, uint8_t pinRst, uint8_t pinIrq) {
      |                                                                                                                               ~~~~~~~~^~~~~~
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_dma_channel_irq.c.o
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_i2c.cpp.o
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_i2s.c.o
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_ina219.cpp.o
src\bsp\bsp_cst328.cpp: In function 'void gpio_isr_handler(uint, uint32_t)':
src\bsp\bsp_cst328.cpp:131:43: warning: unused parameter 'events' [-Wunused-parameter]
  131 | void gpio_isr_handler(uint gpio, uint32_t events)
      |                                  ~~~~~~~~~^~~~~~
src\battery\BspBatteryProvider.cpp: In member function 'void BspBatteryProvider::loadSimState()':
src\battery\BspBatteryProvider.cpp:195:5: warning: 's.BspBatteryProvider::SimState::magic' may be used uninitialized [-Wmaybe-uninitialized]
  195 |     if (s.magic != SIM_MAGIC || s.version != SIM_VERSION) {
      |     ^~
src\battery\BspBatteryProvider.cpp:192:14: note: 's.BspBatteryProvider::SimState::magic' was declared here
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_lcd_brightness.c.o
  192 |     SimState s;
      |              ^
src\battery\BspBatteryProvider.cpp:195:30: warning: 's.BspBatteryProvider::SimState::version' may be used uninitialized [-Wmaybe-uninitialized]
  195 |     if (s.magic != SIM_MAGIC || s.version != SIM_VERSION) {
      |         ~~~~~~~~~~~~~~~Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_pcf85063.c.o
~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~
src\battery\BspBatteryProvider.cpp:192:14: note: 's.BspBatteryProvider::SimState::version' was declared here
  192 |     SimState s;
      |              ^
src\battery\BspBatteryProvider.cpp:203:20: warning: 's.BspBatteryProvider::SimState::voltage_V' may be used uninitialized [-Wmaybe-uninitialized]
  203 |     _simVoltage_V  = s.voltage_V;
      |     ~~~~~~~~~~~~~~~^~~~~~~~~~~~~
src\battery\BspBatteryProvider.cpp:192:14: note: 's.BspBatteryProvider::SimState::voltage_V' was declared here
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_qmi8658.c.o
  192 |     SimState s;
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_st7789.c.o
      |              ^
Compiling .pio\build\waveshare_rp2350\src\bsp\bsp_touch.cpp.o
src\battery\BspBatteryProvider.cpp:204:20: warning: 's.BspBatteryProvider::SimState::current_mA' may be used uninitialized [-Wmaybe-uninitialized]
  204 |     _simCurrent_mA = s.current_mA;
      |     ~~~~~~~~~~~~~~~^~~~~~~~~~~~~~
src\battery\BspBatteryProvider.cpp:192:14: note: 's.BspBatteryProvider::SimState::current_mA' was declared here
  192 |     SimState s;
      |              ^
src\bsp\bsp_i2s.c: In function 'bsp_i2s_init':
src\bsp\bsp_i2s.c:57:43: warning: format '%d' expects argument of type 'int', but argument 3 has type 'uint32_t' {aka 'long unsigned int'} [-Wformat=]
   57 |     printf("I2S sound freq: %d, divider: %d\n", BSP_I2S_FREQ, freqDiv);
      |                                          ~^                   ~~~~~~~
      |                                           |                   |
      |                                           int                 uint32_t {aka long unsigned int}
      |                                          %ld
Compiling .pio\build\waveshare_rp2350\src\main.cpp.o
Compiling .pio\build\waveshare_rp2350\src\ui\BatteryCheckScreen.cpp.o
Compiling .pio\build\waveshare_rp2350\src\ui\StartupScreen.cpp.o
src\bsp\bsp_pcf85063.c: In function 'bsp_pcf85063_set_time':
src\bsp\bsp_pcf85063.c:53:12: warning: unused variable 'now_time' [-Wunused-variable]
   53 |     time_t now_time = mktime(now_tm);
      |            ^~~~~~~~
Compiling .pio\build\waveshare_rp2350\src\ui\actions\ServiceActions.cpp.o
Compiling .pio\build\waveshare_rp2350\src\ui\calibration\TouchCalibration.cpp.o
Compiling .pio\build\waveshare_rp2350\src\ui\graphic\BatteryCheckView.cpp.o
Compiling .pio\build\waveshare_rp2350\src\ui\graphic\StartupView.cpp.o
Compiling .pio\build\waveshare_rp2350\src\ui\navigation\PageNavigator.cpp.o
Generating linkerscript C:\Users\Florin\Documents\PlatformIO\Projects\service-box2\.pio\build\waveshare_rp2350/memmap_default.ld
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\BluetoothDebug.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\Bootsel.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\CoreMutex.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\FS.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\PIOProgram.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\RP2040Support.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\Semihosting.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\SerialPIO.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\SerialUART.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\SerialUSB.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\StackThunk.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\Tone.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\USB.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\WMath.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\_freertos.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\api\Common.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\api\IPAddress.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\api\PluggableUSB.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\api\Print.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\api\Stream.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\api\String.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\cyw43_wrappers.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\delay.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\croutine.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\event_groups.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\freertos-lwip.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\freertos-main.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\heap_3a.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\list.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\lwip_freertos.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\mpu_wrappers_v2_asm.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\port.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\portASM.S.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\portasm.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\queue.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\stream_buffer.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\tasks.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\timers.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\freertos\variantHooks.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\gprof_gmon.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\gprof_shim.S.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\libb64\cdecode.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\libb64\cencode.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lock.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\api\api_lib.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\api\api_msg.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\api\err.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\api\if_api.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\api\netbuf.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\api\netdb.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\api\netifapi.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\api\sockets.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\api\tcpip.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\altcp_tls\altcp_tls_mbedtls.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\altcp_tls\altcp_tls_mbedtls_mem.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\lwiperf\lwiperf.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\mdns\mdns.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\mdns\mdns_domain.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\mdns\mdns_out.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\mqtt\mqtt.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\netbiosns\netbiosns.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\smtp\smtp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_asn1.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_core.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_mib2.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_mib2_icmp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_mib2_interfaces.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_mib2_ip.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_mib2_snmp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_mib2_system.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_mib2_tcp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_mib2_udp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_msg.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_netconn.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_pbuf_stream.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_raw.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_scalar.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_snmpv2_framework.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_snmpv2_usm.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_table.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_threadsync.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmp_traps.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmpv3.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\snmp\snmpv3_mbedtls.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\sntp\sntp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\apps\tftp\tftp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\altcp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\altcp_alloc.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\altcp_tcp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\def.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\dns.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\inet_chksum.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\init.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ip.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv4\acd.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv4\autoip.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv4\dhcp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv4\etharp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv4\icmp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv4\igmp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv4\ip4.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv4\ip4_addr.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv4\ip4_frag.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv6\dhcp6.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv6\ethip6.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv6\icmp6.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv6\inet6.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv6\ip6.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv6\ip6_addr.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv6\ip6_frag.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv6\mld6.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\ipv6\nd6.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\mem.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\memp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\netif.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\pbuf.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\raw.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\stats.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\sys.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\tcp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\tcp_in.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\tcp_out.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\timeouts.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\core\udp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\bridgeif.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\bridgeif_fdb.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ethernet.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\lowpan6.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\lowpan6_ble.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\lowpan6_common.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\auth.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\ccp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\chap-md5.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\chap-new.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\chap_ms.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\demand.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\eap.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\ecp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\eui64.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\fsm.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\ipcp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\ipv6cp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\lcp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\magic.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\mppe.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\multilink.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\polarssl\arc4.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\polarssl\des.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\polarssl\md4.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\polarssl\md5.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\polarssl\sha1.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\ppp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\pppapi.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\pppcrypt.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\pppoe.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\pppol2tp.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\pppos.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\upap.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\utils.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\ppp\vj.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\slipif.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip\src\netif\zepif.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\lwip_wrap.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\main.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\malloc-lock.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\posix.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\psram.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\rp2350-memcpy.S.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\async_context_freertos.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\async_context_threadsafe_background.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\btstack_flash_bank.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\btstack_run_loop_async_context.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\cyw43_arch_freertos.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\cyw43_arch_threadsafe_background.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\cyw43_driver_freertos.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\hid_device.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\inet_chksum.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\midi_device.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\msc_device.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\ncm_device.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\newlib_interface.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\sdkoverride\pico_bootsel_via_double_reset.c.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\stdlib_noniso.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\wiring_analog.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\wiring_digital.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\wiring_private.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\wiring_pulse.cpp.o
Compiling .pio\build\waveshare_rp2350\FrameworkArduino\wiring_shift.cpp.o
Archiving .pio\build\waveshare_rp2350\libFrameworkArduino.a
Linking .pio\build\waveshare_rp2350\firmware.elf
Generating UF2 image
picotool uf2 convert -t elf ".pio\build\waveshare_rp2350\firmware.elf" ".pio\build\waveshare_rp2350\firmware.uf2"
Retrieving maximum program size .pio\build\waveshare_rp2350\firmware.elf
Checking size .pio\build\waveshare_rp2350\firmware.elf
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [          ]   2.2% (used 11492 bytes from 524288 bytes)
Flash: [          ]   2.0% (used 83128 bytes from 4190208 bytes)
Building .pio\build\waveshare_rp2350\firmware.bin
Building .pio\build\waveshare_rp2350\firmware.bin.signed
============================================================== [SUCCESS] Took 65.98 seconds ==============================================================

Environment       Status    Duration
----------------  --------  ------------
waveshare_rp2350  SUCCESS   00:01:05.982
=============================================================== 1 succeeded in 00:01:05.982 =============================================================== *  Terminal will be reused by tasks, press any key to close it. 
