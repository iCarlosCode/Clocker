#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"
#include <PinChangeInterrupt.h>
#include <LiquidCrystal_I2C.h>
#include <TimerOne.h>
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define R 9
#define E 8

#define BTNTL 4
#define BTNLL 5
#define BTNTR 6
#define BTNLR 7

void exibirDataHora(RTC_DS1307& rtc, LiquidCrystal_I2C& lcd);
void printCronometro(unsigned long centesimos);
void printAlarme(unsigned long alarmeDueTime);
void printTimer(unsigned long centesimos);
void printMenu(int mode);
void isrBtnTL();
void isrBtnTR();
void isrBtnLL();
void isrBtnLR();
void startStopWatchTimer();
void stopStopWatchTimer();
void resetStopWatchTimer();
void incrementStopWatchTime();
void setupStopWatchTimer();


// RTC e LCD

RTC_DS1307 rtc;
//LiquidCrystal LCD(R, E, D4, D5, D6, D7);
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
int const CLOCK_EDIT_MODE = 1;
int const STOPWATCH_MODE = 2;
int const STOPWATCH_MODE_RUNNING = 3;
int const TIMER_MODE = 4;
int const TIMER_MODE_RUNNING = 5;
int const TIMER_MODE_EDITING = 6;
int const ALARM_MODE = 7;
int const ALARM_MODE_EDITING = 8;

volatile int MODE = 0;
volatile unsigned long timeCs = 0;

void setup() {
  // Configura os pinos como entrada com pull-up
  pinMode(BTNTL, INPUT_PULLUP);
  pinMode(BTNTR, INPUT_PULLUP);
  pinMode(BTNLL, INPUT_PULLUP);
  pinMode(BTNLR, INPUT_PULLUP);
 
  // Anexa interrupções para cada botão (borda de descida)
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(BTNTL), isrBtnTL, FALLING);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(BTNTR), isrBtnTR, FALLING);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(BTNLL), isrBtnLL, FALLING);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(BTNLR), isrBtnLR, FALLING);

  //Timer1.stop(); // Começa parado

  // put your setup code here, to run once:
  //LCD.begin(16, 2);
  //LCD.setCursor(0, 0);
  //LCD.print("Oi mundo!");
  Serial.begin(9600);
  Serial.print("Começar");

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
  //Timer1.start();
  unsigned long currentMillis = millis(); 

  // Alternar o estado de piscar
  
  //exibirDataHora(rtc, lcd);

  //delay(1000);
  // put your main code here, to run repeatedly:
  //printCronometro(10000);
  //printTimer(count--);
  //printAlarme(30000);
  
  

  switch (MODE)
  {
    case CLOCK_MODE:
      printMenu(CLOCK_MODE);
      break;
    case CLOCK_EDIT_MODE:
      printMenu(CLOCK_EDIT_MODE);
      break;
    case STOPWATCH_MODE:
      printMenu(STOPWATCH_MODE);
      break;
    case STOPWATCH_MODE_RUNNING:
      printMenu(STOPWATCH_MODE_RUNNING);
      break;
    case TIMER_MODE:
      printMenu(TIMER_MODE);
      break;
    case TIMER_MODE_RUNNING:
      printMenu(TIMER_MODE_RUNNING);
      break;
    case TIMER_MODE_EDITING:
      printMenu(TIMER_MODE_EDITING);
      break;
    case ALARM_MODE:
      printMenu(ALARM_MODE);
      break;
    case ALARM_MODE_EDITING:
      printMenu(ALARM_MODE_EDITING);
      break;
    default:
      break;
  }
  if (MODE == STOPWATCH_MODE || MODE == STOPWATCH_MODE_RUNNING) {
    noInterrupts();
    int totalSegundos = timeCs / 100;
    int h = totalSegundos / 3600;
    int m = (totalSegundos % 3600) / 60;
    int s = totalSegundos % 60;
    int cs = timeCs % 100; // centésimos restantes
    interrupts();

    char buffer[20];
    // Formato: HH:MM:SS.CC
    sprintf(buffer, "%02d:%02d:%02d.%02d", h, m, s, cs);
    lcd.setCursor(2, 0);
    lcd.print(buffer);
  }
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


void writeToLcd(int column, int row, uint8_t value) {
  lcd.setCursor(column, row);
  if (value >= 32 && value <= 126) {
    lcd.write((char) value);
  } else {
    lcd.write(value);
  }
}

void printMenuButtons(uint8_t btn_tl, uint8_t btn_tr, uint8_t btn_ll, uint8_t btn_lr) {
  writeToLcd(0, 0, btn_tl);
  writeToLcd(15, 0, btn_tr);
  writeToLcd(0, 1, btn_ll);
  writeToLcd(15, 1, btn_lr);
}

void printMenu(int mode) {

  switch (mode)
  {
    case (CLOCK_MODE):
      printMenuButtons('E', ' ', 'M', ' ');
      lcd.setCursor(2, 0);
      lcd.print("CLOCK");
      break;
    case (CLOCK_EDIT_MODE):
      printMenuButtons(byte(2) /*↑*/, byte(3) /*↓*/, 'M', byte(5) /*➡*/);
      lcd.setCursor(2, 0);
      lcd.print("CLOCKE");
      break;
    case (STOPWATCH_MODE):
      printMenuButtons(' ', byte(0) /*▶*/, 'M', 'R');
      lcd.setCursor(2, 0);
      lcd.print("STW");
      break;
      case (STOPWATCH_MODE_RUNNING):
      printMenuButtons(' ', byte(4) /*⏸*/, 'M', 'R');
      lcd.setCursor(2, 0);
      lcd.print("STWR");
      break;
    case (TIMER_MODE):
      printMenuButtons('E', byte(0) /*▶*/, 'M', 'R');
      lcd.setCursor(2, 0);
      lcd.print("TIMER");
      break;
    case (TIMER_MODE_RUNNING):
      printMenuButtons(' ', byte(4) /*⏸*/, 'M', 'R');
      lcd.setCursor(2, 0);
      lcd.print("TIMERUNING");
      break;
    case (TIMER_MODE_EDITING):
      printMenuButtons(' ', byte(4) /*⏸*/, 'M', 'R');
      lcd.setCursor(2, 0);
      lcd.print("TIMEREd");
      break;
    case (ALARM_MODE):
      printMenuButtons(' ', ' ', 'M', ' ');
      lcd.setCursor(2, 0);
      lcd.print("ALARM");
      break;
    case (ALARM_MODE_EDITING):
      printMenuButtons(' ', ' ', 'M', ' ');
      lcd.setCursor(2, 0);
      lcd.print("ALARMed");
      break;
    default:
      break;
  }
}

// Rotinas de interrupção
void isrBtnTL() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  Serial.println("A");
}

void isrBtnTR() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  if (MODE == STOPWATCH_MODE) {
    startStopWatchTimer();
  }
  else if (MODE == STOPWATCH_MODE_RUNNING) {
    stopStopWatchTimer();
  }
}

void isrBtnLL() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  //Serial.println("M");
  MODE++;
  if (MODE > 8) {
    MODE = 0;
  }

  if (MODE == STOPWATCH_MODE) {
    setupStopWatchTimer();
  }
}

void isrBtnLR() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  if (MODE == STOPWATCH_MODE || MODE == STOPWATCH_MODE_RUNNING) {
    resetStopWatchTimer();
  }
}

void startStopWatchTimer() {
  Timer1.start();
  noInterrupts();
  MODE = STOPWATCH_MODE_RUNNING;
  interrupts();
}

void stopStopWatchTimer() {
  Timer1.stop();
  noInterrupts();
  MODE = STOPWATCH_MODE;
  interrupts();
}

void resetStopWatchTimer() {
  noInterrupts();
  timeCs = 0;
  interrupts();
}

void incrementStopWatchTime() {
  timeCs++;
}

void setupStopWatchTimer() {
  Timer1.initialize(10000); // 1000 microssegundos = 1 milissegundo
  Timer1.attachInterrupt(incrementStopWatchTime);
  Timer1.stop();
  noInterrupts();
  timeCs = 0;
  interrupts();
}