#include "SHA1.h"

// -------------------- CONFIG --------------------
#define RS485 Serial1
#define DE_PIN 2 
#define D0_PIN 4
#define D1_PIN 3

#define D0_BIT 4
#define D1_BIT 0

unsigned long lastSendTime = 0;
const unsigned long debounceTime = 5000;

// -------------------- WIEGAND FAST --------------------

void pulseD0_fast() {
  PORTD &= ~(1 << D0_BIT);
  delayMicroseconds(60);
  PORTD |=  (1 << D0_BIT);
}

void pulseD1_fast() {
  PORTD &= ~(1 << D1_BIT);
  delayMicroseconds(60);
  PORTD |=  (1 << D1_BIT);
}

void sendWiegand26(uint32_t frame) {
  for (int i = 25; i >= 0; i--) {
    if (frame & (1UL << i)) pulseD1_fast();
    else pulseD0_fast();
    delayMicroseconds(1000);
  }
}

// -------------------- SHA1 → LAST 24 BITS → W26 --------------------

uint32_t plateToWiegand(const char* plate) {
  SHA1_CTX ctx;
  SHA1Init(&ctx);
  SHA1Update(&ctx, (uint8_t*)plate, strlen(plate));
  uint8_t digest[20];
  SHA1Final(digest, &ctx);

  uint32_t payload =
      ((uint32_t)digest[17] << 16) |
      ((uint32_t)digest[18] << 8)  |
      ((uint32_t)digest[19]);

  uint32_t left12  = (payload >> 12) & 0xFFF;
  uint32_t right12 = payload & 0xFFF;

  uint8_t leftParity  = (__builtin_popcount(left12)  % 2 == 1);
  uint8_t rightParity = (__builtin_popcount(right12) % 2 == 0);

  uint32_t frame = 0;
  frame |= ((uint32_t)leftParity << 25);
  frame |= (payload << 1);
  frame |= rightParity;

  return frame;
}

// -------------------- RS485 FRAME PARSER --------------------

bool readPlateFromRS485(char *plate) {
  static uint8_t buf[64];
  static uint8_t idx = 0;

  while (RS485.available()) {
    uint8_t b = RS485.read();


    if (idx == 0) {
      if (b == 0xAA) buf[idx++] = b;
      continue;
    }
    if (idx == 1) {
      if (b == 0xBB) buf[idx++] = b;
      else if (b != 0xAA) idx = 0; 
      continue;
    }

    buf[idx++] = b;


    if (idx >= 10 && buf[idx-2] == 0xAA && buf[idx-1] == 0x55) {
      

      Serial.print("RAW FRAME (HEX): ");
      for (int i = 0; i < idx; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      int pIdx = 0;
      for (int i = 2; i < idx - 2; i++) {
        char c = (char)buf[i];
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
          if (pIdx < 15) { 
            plate[pIdx++] = c;
          }
        }
      }
      plate[pIdx] = '\0'; 

      idx = 0; 

 
      if (pIdx > 0) {
        return true;
      }
    }


    if (idx >= 63) idx = 0;
  }

  return false;
}

// -------------------- SETUP --------------------

void setup() {
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);

  pinMode(D0_PIN, OUTPUT);
  pinMode(D1_PIN, OUTPUT);
  digitalWrite(D0_PIN, HIGH);
  digitalWrite(D1_PIN, HIGH);

  Serial.begin(115200);
  RS485.begin(9600); 

  Serial.println("Start RS485 → SHA1 → Wiegand26");
}

// -------------------- MAIN LOOP --------------------

void loop() {
  char plate[16];

  if (readPlateFromRS485(plate)) {
    unsigned long now = millis();
    static char lastPlate[16] = "";
    static unsigned long lastPlateTime = 0;


    if (strcmp(plate, lastPlate) == 0 && (now - lastPlateTime) < debounceTime) {
      Serial.println("Ignore dupliacte(debounce)");
      return;
    }


    strcpy(lastPlate, plate);
    lastPlateTime = now;

    Serial.print("Plate: ");
    Serial.println(plate);

    uint32_t frame = plateToWiegand(plate);

    Serial.print("Sending W26: ");
    Serial.println(frame);

    sendWiegand26(frame);
  }
}