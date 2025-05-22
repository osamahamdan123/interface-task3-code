
// --- Pin Definitions ---
const int LCD_RS = 12;
const int LCD_E  = 11;
const int LCD_D4 = 5;
const int LCD_D5 = 4;
const int LCD_D6 = 3;
const int LCD_D7 = 2;

const int TEMP_PIN = A0;
const int LDR_PIN  = A1;

const int GAUGE_LENGTH = 7;  // Number of character cells in each gauge

// Custom characters: each cell is 5 pixels wide, levels 0–5
byte barLevel[6][8] = {
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  // empty
  {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10},  // 1/5
  {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18},  // 2/5
  {0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C},  // 3/5
  {0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E},  // 4/5
  {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}   // full
};


void lcdInit();
void lcdCreateChar(byte idx, byte data[8]);
void lcdSetCursor(int col, int row);
void lcdPrint(const char* s);
void lcdPrint(int v);
void lcdPrintFloat(float val, byte decimals);
void drawGauge(int x, int y, float value, float minVal, float maxVal);
void pulseEnable();
void send4bit(byte v);
void lcdCommand(byte c);
void lcdWrite(byte d);

// Variables for display toggling 
unsigned long lastToggleTime = 0;
bool showTemperature = true;

void setup() {
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_E,  OUTPUT);
  pinMode(LCD_D4, OUTPUT);
  pinMode(LCD_D5, OUTPUT);
  pinMode(LCD_D6, OUTPUT);
  pinMode(LCD_D7, OUTPUT);

  lcdInit();

  for (int i = 0; i < 6; i++) {
    lcdCreateChar(i, barLevel[i]);
  }
}

void loop() {
  // Read sensors
  int rawTemp  = analogRead(TEMP_PIN);
  int rawLight = analogRead(LDR_PIN);

  // Convert TMP36 output to C
  float voltage = rawTemp * (5.0 / 1023.0);
  float tempC   = (voltage - 0.5) * 100.0;

  // Convert LDR reading to 100%
  int lightPct = map(rawLight, 0, 1023, 0, 100);

  // Toggle display every 2 seconds
  if (millis() - lastToggleTime >= 2000) {
    showTemperature = !showTemperature;
    lastToggleTime = millis();
  }

  lcdSetCursor(0, 0);
  if (showTemperature) {
    lcdPrint("Temp: ");
    lcdPrintFloat(tempC, 1);
    lcdPrint(" C     "); // Padding to clear old text
    lcdSetCursor(0, 1);
    drawGauge(0, 1, tempC, 0, 50);
  } else {
    lcdPrint("Light: ");
    lcdPrint(lightPct);
    lcdPrint("%     "); // Padding to clear old text
    lcdSetCursor(0, 1);
    drawGauge(0, 1, lightPct, 0, 100);
  }

  delay(200); // Slight delay 
}

// --- LCD Functions  ---
void pulseEnable() {
  digitalWrite(LCD_E, HIGH);
  delayMicroseconds(1);
  digitalWrite(LCD_E, LOW);
  delayMicroseconds(100);
}

void send4bit(byte v) {
  digitalWrite(LCD_D4, (v >> 0) & 1);
  digitalWrite(LCD_D5, (v >> 1) & 1);
  digitalWrite(LCD_D6, (v >> 2) & 1);
  digitalWrite(LCD_D7, (v >> 3) & 1);
  pulseEnable();
}

void lcdCommand(byte c) {
  digitalWrite(LCD_RS, LOW);
  send4bit(c >> 4);
  send4bit(c & 0x0F);
  delayMicroseconds(40);
}

void lcdWrite(byte d) {
  digitalWrite(LCD_RS, HIGH);
  send4bit(d >> 4);
  send4bit(d & 0x0F);
  delayMicroseconds(40);
}

void lcdInit() {
  delay(50);
  digitalWrite(LCD_RS, LOW);
  digitalWrite(LCD_E,  LOW);
  send4bit(0x03); delay(5);
  send4bit(0x03); delayMicroseconds(150);
  send4bit(0x03); delayMicroseconds(150);
  send4bit(0x02);
  lcdCommand(0x28); // 2 lines, 5x8
  lcdCommand(0x08); // Display off
  lcdCommand(0x01); delay(2); // Clear
  lcdCommand(0x06); // Entry mode
  lcdCommand(0x0C); // Display on
}

void lcdSetCursor(int col, int row) {
  const byte rowOffset[] = { 0x00, 0x40, 0x14, 0x54 };
  lcdCommand(0x80 | (col + rowOffset[row]));
}

void lcdCreateChar(byte idx, byte data[8]) {
  lcdCommand(0x40 | (idx << 3));
  for (int i = 0; i < 8; i++) {
    lcdWrite(data[i]);
  }
}

void lcdPrint(int v) {
  char buf[7];
  itoa(v, buf, 10);
  for (char* p = buf; *p; p++) {
    lcdWrite(*p);
  }
}

void lcdPrint(const char* s) {
  while (*s) {
    lcdWrite(*s++);
  }
}

void lcdPrintFloat(float val, byte decimals) {
  int whole = int(val);
  float frac = abs(val - whole);
  lcdPrint(whole);
  lcdWrite('.');
  while (decimals--) {
    frac *= 10.0;
    int d = int(frac) % 10;
    lcdPrint(d);
  }
}

void drawGauge(int x, int y, float value, float minVal, float maxVal) {
  float pct = constrain((value - minVal) / (maxVal - minVal), 0.0, 1.0);
  int totalPixels  = GAUGE_LENGTH * 5;
  int filledPixels = int(pct * totalPixels + 0.5);

  lcdSetCursor(x, y);
  for (int i = 0; i < GAUGE_LENGTH; i++) {
    int rem = filledPixels - i * 5;
    byte level = (rem >= 5 ? 5 : (rem > 0 ? rem : 0));
    lcdWrite(level);
  }
}
