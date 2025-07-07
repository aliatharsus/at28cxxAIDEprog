const int dataPins[8] = {23, 22, 16, 32, 33, 25, 26, 27};

#define STCP 21
#define SHCP 19
#define SRDT 17

#define WE 14
#define OE 13

#define READ 1
#define WRITE 0

#define AT28C64 8192
#define AT28C256 32768

enum Mode { IDLE, ORG_WAIT, WRITING, READING, FILLING, ADWRITING, ADREADING };
Mode mode = IDLE;
uint16_t romSize = AT28C64;
uint16_t addr = 0;
String input = "";

bool fillFlag=0;
byte filler=0xEA;

bool noOrg=0;
bool writeFlagOrg=0;

bool adwriteFlag=0;

unsigned long lastWriteTime = 0;
const unsigned long writeTimeout = 6000;

void setup() {
  Serial.begin(9600);
  
  pinMode(STCP, OUTPUT);
  pinMode(SHCP, OUTPUT);
  pinMode(SRDT, OUTPUT);
  pinMode(WE, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(2, OUTPUT); digitalWrite(2, LOW);

  digitalWrite(STCP, LOW);
  digitalWrite(SHCP, LOW);
  digitalWrite(SRDT, LOW);
  digitalWrite(OE, HIGH);
  digitalWrite(WE, HIGH);
  dataConfig(READ);
}

void loop() {
  //For auto debug message print
  /*
  static unsigned long lastDebug = 0;   //debug every 2 sec
  if (millis() - lastDebug > 2000 && mode!=READING) {
    Serial.print("DEBUG: Mode=");
    Serial.print(mode);
    Serial.print(" Addr=");
    Serial.print(addr);
    Serial.print(" Input='");
    Serial.print(input);
    Serial.println("'");
    lastDebug = millis();
  }*/


  if (mode == IDLE) {
    while (Serial.available()) {
      char c = Serial.read();
      //for per symbol debugging
      /*
      Serial.print("DEBUG: Got char: '");
      Serial.print(c);
      Serial.print("' (");
      Serial.print((int)c);
      Serial.println(")");
      */
      
      if (c == '\n' || c == '\r') {
        Serial.print("Processing command: '");
        Serial.print(input);
        Serial.println("'");
        
        if (input.startsWith("WRITE:")) {
          romSize = input.substring(6).toInt();
          Serial.print("Extracted romSize: ");
          Serial.println(romSize);
          if (romSize == 8192 || romSize == 32768 || romSize == 16384) {
            dataConfig(WRITE);
            addr=0;
            mode = ORG_WAIT;
            Serial.println("WAITING_FOR_ORG");
            lastWriteTime = millis();
          } else {
            Serial.print("ERROR: Invalid romSize: ");
            Serial.println(romSize);
          }
        } 
        else if (input.startsWith("READ:")) {
          romSize = input.substring(5).toInt();
          addr = 0;
          dataConfig(READ);
          mode = READING;
          Serial.println("READING_STARTED");
        }
        else if (input.startsWith("FILL:")) {
          romSize = input.substring(5).toInt();
          addr = 0;
          dataConfig(WRITE);
          mode = FILLING;
          Serial.println("Filling...");
        }
        else if (input.startsWith("ADWR:")) {
          romSize = input.substring(5).toInt();
          addr = 0;
          dataConfig(WRITE);
          mode = ADWRITING;
        }
        else if (input.startsWith("ADRD:")) {
          romSize = input.substring(5).toInt();
          addr = 0;
          dataConfig(READ);
          mode = ADREADING;
        }
        else if (input.startsWith("DEBUG")) {
          Serial.print("DEBUG: Mode=");
          Serial.print(mode);
          Serial.print(" Addr=");
          Serial.print(addr);
          Serial.print(" Input='");
          Serial.print(input);
          Serial.println("'");
        }
        else if (input.length() > 0) {
          Serial.print("ERROR: Unknown command: '");
          Serial.print(input);
          Serial.println("'");
        }
        
        input = "";
        break;
      } else if (c >= 32 && c <= 126) {
        input += c;
      }
    }
  }
    if (mode == ORG_WAIT) {
    if (Serial.available() >= 2) {
      uint8_t low = Serial.read();
      uint8_t high = Serial.read();
      addr = low | (high << 8);
      lastWriteTime = millis();
      mode = WRITING;
      Serial.print("ORG_SET: ");
      Serial.println(addr, HEX);
    } else {
      if (millis() - lastWriteTime > 3000) {
        Serial.println("ORG_TIMEOUT, writing at 0x0000");
        addr = 0;
        mode = WRITING;
      }
    }
  }

  if (mode == WRITING) {
  if (Serial.available()) {
    uint8_t byte = Serial.read();
    lastWriteTime = millis();

    Serial.print(addr, HEX);
    Serial.print(" : ");
    Serial.println(byte, HEX);

    if (addr < romSize) {
      writeByte(addr, byte);
      addr++;
    } else {
      Serial.println("ERROR: Addr exceeds ROM size");
      mode = IDLE;
      dataConfig(READ);
    }
  } else if (millis() - lastWriteTime > writeTimeout) {
    Serial.println("WRITE_TIMEOUT");
    mode = IDLE;
    dataConfig(READ);
  }
}

  if (mode == READING) {
    const uint8_t bytesPerLine = 8;

    if (addr < romSize) {
      if (addr % bytesPerLine == 0) {
        Serial.print("0x");
        if (addr < 0x1000) Serial.print("0");
        if (addr < 0x0100) Serial.print("0");
        if (addr < 0x0010) Serial.print("0");
        Serial.print(addr, HEX);
        Serial.print(": ");
      }

      uint8_t b = readByte(addr++);
      if (b < 0x10) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");

      if (addr % bytesPerLine == 0) Serial.println();
      //if (addr % 256 == 0) digitalWrite(2, !digitalRead(2));
    } else {
      Serial.println();
      Serial.println("READ_COMPLETE");
      mode = IDLE;
    }
  }
  if (mode == FILLING) {
    if(addr<romSize){
      if(Serial.available() && !fillFlag){filler = Serial.read(); fillFlag=1;Serial.print("FILL_BYTE: 0x");Serial.println(filler, HEX);}
      if(fillFlag){
        writeByte(addr,filler);
        addr++;
      }
    }
    else{
      addr = 0;
      mode = IDLE;
      filler = 0xEA;
      fillFlag = 0;
      Serial.println("Fill Complete");
    }
  }
if (mode == ADWRITING) {
  static byte adwriteByte = 0;
  if (!adwriteFlag && Serial.available() >= 3) {
    uint8_t low = Serial.read();
    uint8_t high = Serial.read();
    addr = low | (high << 8);
    adwriteByte = Serial.read();
    adwriteFlag = 1;
  }

  if (adwriteFlag) {
    adwriteFlag = 0;
    writeByte(addr, adwriteByte);
    addr = 0;
    mode = IDLE;
    Serial.println("Written");
  }
}
if (mode == ADREADING) {
  if (Serial.available() >= 2) {
    uint8_t low = Serial.read();
    uint8_t high = Serial.read();
    addr = low | (high << 8);
    byte adreadByte=readByte(addr);
    mode = IDLE;
    Serial.print("Data at ");
    Serial.print(addr,HEX);
    Serial.print(" : ");
    Serial.println(adreadByte,HEX);
    addr = 0;
  }
}

if(mode==IDLE){
  digitalWrite(2,HIGH);
}
else{
  if(addr%256==0){
    digitalWrite(2,!digitalRead(2));
  }
}

}

void writeByte(uint16_t addr, byte dattt) {
  shiftOut16(addr);
  dataOut(dattt);
  digitalWrite(WE, LOW);
  asm volatile("nop\n\t"
               "nop\n\t");
  digitalWrite(WE, HIGH);
  delay(10);
}

uint8_t readByte(uint16_t addr) {
  digitalWrite(OE, HIGH);
  delayMicroseconds(15);
  shiftOut16(addr);
  delayMicroseconds(15);
  digitalWrite(OE, LOW);
  delayMicroseconds(50);
  return dataRead();
}

void shiftOut16(uint16_t data) {
  shiftOut(SRDT, SHCP, MSBFIRST, (data >> 8) & 0xFF);
  shiftOut(SRDT, SHCP, MSBFIRST, data & 0xFF);

  delayMicroseconds(1);
  digitalWrite(STCP, HIGH);
  delayMicroseconds(10);
  digitalWrite(STCP, LOW);
  delayMicroseconds(5);
}

void dataOut(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(dataPins[i], (data >> i) & 0x01);
  }
}

uint8_t dataRead() {
  delayMicroseconds(1);
  uint8_t value = 0;
  for (int i = 0; i < 8; i++) {
    value |= (digitalRead(dataPins[i]) << i);
  }
  return value;
}

void dataConfig(bool state) {
  if (state) {
    for (int i = 0; i < 8; i++) {
      pinMode(dataPins[i], INPUT);
      digitalWrite(dataPins[i], LOW);
    }
    digitalWrite(OE, LOW);
  }
  else {
    for (int i = 0; i < 8; i++) {
      pinMode(dataPins[i], OUTPUT);
      digitalWrite(dataPins[i], LOW);
    }
    digitalWrite(OE, HIGH);
  }
}
