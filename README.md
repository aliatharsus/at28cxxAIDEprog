# AT28cXXX EEPROM Programmer - ArduinoIDE + WebApp

No bloated libraries, no dependencies, no nonsense.  
Just plug your EEPROM into any Arduino-compatible board, open a browser, and flash/read/fill with ease.

Supports **AT28C64**, **AT28C128**, **AT28C256**  
Only `.bin` files for now.

ADDED .org (* = $xxxx) support

---

## Features

-  Write `.bin` files to EEPROM (AT28C64 / 128 / 256)
-  Read EEPROM to browser (in nice hex format)
-  Fill EEPROM with any byte (default = EA)
-  Write a byte to specific address
-  Read a byte from specific address
-  Pure browser interface (no Python, no CLI tools)
-  Blinks pin 2 every 256 bytes for visual progress
-  Easy wiring via 2x 74HC595 shift registers for address bus

---

##  Known Issues

- **Read operation freezes browser UI**  
  Not a bug. It's just flooding serial data. The browser *will* show everything after it finishes. Be patient.

- **No verify after write**  
  If you need it, feel free to add a read loop — the code is totally open.

---

## Setup

### Hardware

- EEPROM: AT28C64 / 128 / 256 (28-pin DIP)
- 2x 74HC595 shift registers for 16-bit addressing (actually 15 for c256)
- Data lines go to any 8 GPIOs
- Wire everything as per your board and change the pin numbers at the top of the `.ino` file, and upload (an example with nodemcu32s esp32 board is provided in the png)

```cpp
const int dataPins[8] = {23, 22, 16, 32, 33, 25, 26, 27};
#define STCP 21
#define SHCP 19
#define SRDT 17
#define WE 14
#define OE 13