#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>
#include <LiquidCrystal_I2C.h>
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define R 9
#define E 8


// RTC e LCD

RTC_DS1307 rtc;
LiquidCrystal LCD(R, E, D4, D5, D6, D7);
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte playSymbol[8] = {
  0b10000,
  0b11000,
  0b11100,
  0b11110,
  0b11110,
  0b11100,
  0b11000,
  0b10000
};

byte stopSymbol[8] = {
  0b00000,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b00000,
  0b00000
};

byte upArrow[8] = {
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00000,
  0b00000
};

byte downArrow[8] = {
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b00000
};

byte pauseSymbol[8] = {
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b00000,
  0b00000
};

byte arrowRightSymbol[8] = {
  0b00100,
  0b00110,
  0b11111,
  0b00110,
  0b00100,
  0b00000,
  0b00000,
  0b00000
};

int const CLOCK_MODE = 0;
int const STOPWATCH_MODE = 1;
int const STOPWATCH_MODE_RUNNING = 2;
int const TIMER_MODE = 3;
int const TIMER_MODE_RUNNING = 4;
int const ALARM_MODE = 5;

void exibirDataHora(RTC_DS1307& rtc, LiquidCrystal_I2C& lcd);
void printCronometro(unsigned long centesimos);
void printAlarme(unsigned long alarmeDueTime);
void printTimer(unsigned long centesimos);
void printMenu(int mode);

void setup() {
  // put your setup code here, to run once:
  LCD.begin(16, 2);
  LCD.setCursor(0, 0);
  LCD.print("Oi mundo!");
  Serial.begin(9600);

  Wire.begin();
  rtc.begin();
  lcd.begin(16, 2);
  lcd.backlight();

  // Create custom characters in CGRAM slots 0 to 3
  lcd.createChar(0, playSymbol);
  lcd.createChar(1, stopSymbol);
  lcd.createChar(2, upArrow);
  lcd.createChar(3, downArrow);
  lcd.createChar(4, pauseSymbol);
  lcd.createChar(5, arrowRightSymbol);

  if (!rtc.isrunning()) {
    lcd.print("RTC parado");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // só na 1ª vez
    delay(2000);
    lcd.clear();
  }
}

unsigned long count = 100000;

void loop() {
  unsigned long currentMillis = millis();

  // Alternar o estado de piscar
  
  //exibirDataHora(rtc, lcd);

  //delay(1000);
  // put your main code here, to run repeatedly:
  //printCronometro(10000);
  //printTimer(count--);
  //printAlarme(30000);
  printMenu(CLOCK_MODE);
  delay(3000);
  printMenu(STOPWATCH_MODE);
  delay(3000);
  printMenu(STOPWATCH_MODE_RUNNING);
  delay(3000);
  printMenu(TIMER_MODE);
  delay(3000);
  printMenu(TIMER_MODE_RUNNING);
  delay(3000);
  printMenu(ALARM_MODE);
  delay(1000);
  delay(10);
}

void printCronometro(unsigned long centesimos) {
  int totalSegundos = centesimos / 100;
  int h = totalSegundos / 3600;
  int m = (totalSegundos % 3600) / 60;
  int s = totalSegundos % 60;
  int cs = centesimos % 100; // centésimos restantes

  char buffer[20];
  // Formato: HH:MM:SS.CC
  sprintf(buffer, "%02d:%02d:%02d.%02d", h, m, s, cs);
  
  lcd.setCursor(0, 0);
  //lcd.print("");
  
  lcd.setCursor(0, 1);
  lcd.print("M");
  
  lcd.setCursor(15, 0);
  lcd.write(byte(4)); // ⏸
  //lcd.write(byte(0)); // ▶

  lcd.setCursor(15, 1);
  lcd.print("R");

  lcd.setCursor(2, 0);
  lcd.print(buffer);
}

void printTimer(unsigned long centesimos) {
  int totalSegundos = centesimos / 100;
  int h = totalSegundos / 3600;
  int m = (totalSegundos % 3600) / 60;
  int s = totalSegundos % 60;
  int cs = centesimos % 100; // centésimos restantes

  char buffer[20];
  // Formato: HH:MM:SS.CC
  sprintf(buffer, "%02d:%02d:%02d.%02d", h, m, s, cs);
  
  lcd.setCursor(0, 0);
  lcd.print("E");
  
  lcd.setCursor(0, 1);
  lcd.print("M");
  
  lcd.setCursor(15, 0);
  lcd.write(byte(4)); // ⏸
  //lcd.write(byte(0)); // ▶

  lcd.setCursor(15, 1);
  lcd.print("R");

  lcd.setCursor(2, 0);
  lcd.print(buffer);
}

void exibirDataHora(RTC_DS1307& rtc, LiquidCrystal_I2C& lcd) {
  DateTime now = rtc.now();

  char linha1[17];
  char linha2[17];

  // Exemplo: 23:59:59
  sprintf(linha1, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  // Exemplo: 2025-06-24
  sprintf(linha2, "%04d-%02d-%02d", now.year(), now.month(), now.day());

  lcd.setCursor(0, 0);
  lcd.print("E");
  lcd.setCursor(0, 1);
  lcd.print("M");
  lcd.setCursor(2, 0);
  lcd.print(linha1);
  lcd.setCursor(2, 1);
  lcd.print(linha2);
}

void printAlarme(unsigned long alarmeDueTime) {
  int totalSegundos = alarmeDueTime;
  int h = totalSegundos / 3600;
  int m = (totalSegundos % 3600) / 60;
  int s = totalSegundos % 60;
  
  //char buffer[20];
  // Formato: HH:MM:SS
  //sprintf(buffer, "%02d:%02d:%02d", h, m, s);
  
  lcd.setCursor(0, 0);
  //lcd.print("E");
  lcd.write(byte(2));

  lcd.setCursor(0, 1);
  lcd.print("M");
  
  lcd.setCursor(15, 0);
  lcd.write(byte(3)); // ↓
  //lcd.write(byte(4)); // ⏸
  //lcd.write(byte(0)); // ▶

  lcd.setCursor(15, 1);
  lcd.write(5);
  //lcd.print("R");

  lcd.setCursor(2, 0);
  lcd.print("08:00");
  //lcd.print(buffer);
}



void printMenuButtons(uint8_t btn_tl, uint8_t btn_tr, uint8_t btn_ll, uint8_t btn_lr) {
  lcd.setCursor(0, 0);
  if (btn_tl >= 32 && btn_tl <= 126)
    lcd.write((char) btn_tl);
  else
    lcd.write(btn_tl);

  lcd.setCursor(0, 1);
  if (btn_tr >= 32 && btn_tr <= 126)
    lcd.write((char) btn_tr);
  else
    lcd.write(btn_tr);

  lcd.setCursor(15, 0);
  if (btn_ll >= 32 && btn_ll <= 126)
    lcd.write((char) btn_ll);
  else
    lcd.write(btn_ll);
  
  lcd.setCursor(15, 1);
  if (btn_lr >= 32 && btn_lr <= 126)
    lcd.write((char) btn_lr);
  else
    lcd.write(btn_lr);
}

void printMenu(int mode) {

  switch (mode)
  {
    case (CLOCK_MODE):
      printMenuButtons('E', 'M', ' ', ' ');
      lcd.setCursor(2, 0);
      lcd.print("CLOCK");
      break;
    case (STOPWATCH_MODE):
      printMenuButtons(' ', 'M', byte(0) /*▶*/, 'R');
      lcd.setCursor(2, 0);
      lcd.print("STW");
      break;
      case (STOPWATCH_MODE_RUNNING):
      printMenuButtons(' ', 'M', byte(4) /*⏸*/, 'R');
      lcd.setCursor(2, 0);
      lcd.print("STWR");
      break;
    case (TIMER_MODE):
      printMenuButtons('E', 'M', byte(0) /*▶*/, 'R');
      lcd.setCursor(2, 0);
      lcd.print("TIMER");
      break;
    case (TIMER_MODE_RUNNING):
      printMenuButtons(' ', 'M', byte(4) /*⏸*/, 'R');
      lcd.setCursor(2, 0);
      lcd.print("TIMERUNING");
      break;
    case (ALARM_MODE):
      printMenuButtons(' ', 'M', ' ', ' ');
      lcd.setCursor(2, 0);
      lcd.print("ALARM");
      break;
    default:
      break;
  }
}