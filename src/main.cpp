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
void printMenu(int mode);
void isrBtnTL();
void isrBtnTR();
void isrBtnLL();
void isrBtnLR();
// Blinker
void changeBlinkingState();
void setupBlinkingTimer();

// Stopwatch
void startStopWatchTimer();
void stopStopWatchTimer();
void resetStopWatchTimer();
void incrementStopWatchTime();
void setupStopWatchTimer();
// Timer
void startTimer();
void stopTimer();
void resetTimer();
void incrementTime();
void setupTimer();


void generateBody();

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
bool DEBUG_MENU = true;
int const MENU_PADDING = 2;

volatile int MODE = 0;
int const CLOCK_MODE = 0;
int const CLOCK_EDIT_MODE = 1;
int const STOPWATCH_MODE = 2;
int const STOPWATCH_MODE_RUNNING = 3;
int const TIMER_MODE = 4;
int const TIMER_MODE_RUNNING = 5;
int const TIMER_MODE_EDITING = 6;
int const ALARM_MODE = 7;
int const ALARM_MODE_EDITING = 8;

// Time variables
volatile unsigned long timeCs = 0;
volatile unsigned long timeS = 300;
volatile unsigned long timeAlarmS = 28800;

// MODO DE EDIÇÂO
volatile int edit_cursor = 0;
volatile int edit_h = 0;
volatile int edit_m = 0;
volatile int edit_s = 0;
volatile bool edit_blink_state = false;

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


void loop() {
  unsigned long currentMillis = millis(); 
  
  //exibirDataHora(rtc, lcd);
  generateBody();
}

void printHhMmSsEdit() {
    char buffer[20];
    // Formato: HH:MM:SS
    sprintf(buffer, "%02d:%02d:%02d   ", edit_h, edit_m, edit_s);
    if (edit_blink_state) 
    {
      // Set the cursor to blink on HH, MM or SS
      buffer[edit_cursor * 3] = ' ';
      buffer[edit_cursor * 3 + 1] = ' ';
    }
    lcd.setCursor(MENU_PADDING, 0);
    lcd.print(buffer);
}

void printHhMmSsFromSeconds(unsigned long totalSegundos) {
    noInterrupts();
    int h = totalSegundos / 3600;
    int m = (totalSegundos % 3600) / 60;
    int s = totalSegundos % 60;
    interrupts();

    char buffer[20];
    // Formato: HH:MM:SS
    sprintf(buffer, "%02d:%02d:%02d   ", h, m, s);
    lcd.setCursor(MENU_PADDING, 0);
    lcd.print(buffer);
    //Serial.println(totalSegundos);
}

void generateBody() {
  printMenu(MODE);

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
    lcd.setCursor(MENU_PADDING, 0);
    lcd.print(buffer);
  }
  else if (MODE == TIMER_MODE || MODE == TIMER_MODE_RUNNING) {
    printHhMmSsFromSeconds(timeS);
  }
  else if (MODE == TIMER_MODE_EDITING) {
    printHhMmSsEdit();
  }
  else if (MODE == ALARM_MODE) {
    printHhMmSsFromSeconds(timeAlarmS);
  }
  else if (MODE == ALARM_MODE_EDITING) {
    printHhMmSsEdit();
  }
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
  lcd.setCursor(MENU_PADDING, 0);
  lcd.print(linha1);
  lcd.setCursor(2, 1);
  lcd.print(linha2);
}


// Menu Functions

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
      lcd.setCursor(MENU_PADDING, 0);
      lcd.print("CLOCK");
      break;
    case (CLOCK_EDIT_MODE):
      printMenuButtons(byte(2) /*↑*/, byte(3) /*↓*/, 'M', byte(5) /*➡*/);
      lcd.setCursor(MENU_PADDING, 0);
      lcd.print("CLOCKE");
      break;
    case (STOPWATCH_MODE):
      printMenuButtons(' ', byte(0) /*▶*/, 'M', 'R');
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("STW");
      break;
      case (STOPWATCH_MODE_RUNNING):
      printMenuButtons(' ', byte(4) /*⏸*/, 'M', 'R');
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("STWR");
      break;
    case (TIMER_MODE):
      printMenuButtons('E', byte(0) /*▶*/, 'M', 'R');
      lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("TIMER");
      break;
    case (TIMER_MODE_RUNNING):
      printMenuButtons(' ', byte(4) /*⏸*/, ' ', 'R');
      lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("TIMERUNING");
      break;
    case (TIMER_MODE_EDITING):
      printMenuButtons(byte(2) /*↑*/, byte(3) /*↓*/, 'M', byte(5) /*➡*/);
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("TIMEREd");
      break;
    case (ALARM_MODE):
      printMenuButtons('E', ' ', 'M', ' ');
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("ALARM");
      break;
    case (ALARM_MODE_EDITING):
      printMenuButtons(byte(2) /*↑*/, byte(3) /*↓*/, 'M', byte(5) /*➡*/);
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("ALARMed");
      break;
    default:
      break;
  }
}

// Rotinas de interrupção
void isrBtnTL() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

  switch (MODE)
  {
    case CLOCK_MODE:
      noInterrupts();
      MODE = CLOCK_EDIT_MODE;
      interrupts();
      break;
    case TIMER_MODE:
      noInterrupts();
      MODE = TIMER_MODE_EDITING;
      edit_cursor = 0;
      interrupts();
      setupBlinkingTimer();
      break;
    case ALARM_MODE:
      noInterrupts();
      MODE = ALARM_MODE_EDITING;
      edit_cursor = 0;
      interrupts();
      setupBlinkingTimer();
      break;
    case TIMER_MODE_EDITING:
      if (edit_cursor == 0)
      {
        noInterrupts();
        if (edit_h == 0)
          edit_h = 100;
        edit_h--;
        interrupts();
      }
      else if (edit_cursor == 1)
      {
        noInterrupts();
        edit_m--;
        if (edit_m < 0)
          edit_m = 59;
        interrupts();
      }
      else if (edit_cursor == 2)
      {
        noInterrupts();
        edit_s--;
        if (edit_s < 0)
          edit_s = 59;
        interrupts();
      }
      break;
    case ALARM_MODE_EDITING:
      if (edit_cursor == 0)
      {
        noInterrupts();
        if (edit_h == 0)
          edit_h = 23;
        edit_h--;
        interrupts();
      }
      else if (edit_cursor == 1)
      {
        noInterrupts();
        edit_m--;
        if (edit_m < 0)
          edit_m = 59;
        interrupts();
      }
      else if (edit_cursor == 2)
      {
        noInterrupts();
        edit_s--;
        if (edit_s < 0)
          edit_s = 59;
        interrupts();
      }
      break;
    default:
      break;
  }
  if (DEBUG_MENU)
  {
    Serial.println("TL" + String(MODE));
  }
}

void isrBtnTR() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

  switch (MODE)
  {
    case STOPWATCH_MODE:
      startStopWatchTimer();
      break;
    case STOPWATCH_MODE_RUNNING:
      stopStopWatchTimer();
      break;
    case TIMER_MODE:
      startTimer();
      break;
    case TIMER_MODE_RUNNING:
      stopTimer();
      break;
    case TIMER_MODE_EDITING:
      if (edit_cursor == 0)
      {
        noInterrupts();
        edit_h++;
        if (edit_h > 99)
          edit_h = 0;
        interrupts();
      }
      else if (edit_cursor == 1)
      {
        noInterrupts();
        edit_m++;
        if (edit_m > 59)
          edit_m = 0;
        interrupts();
      }
      else if (edit_cursor == 2)
      {
        noInterrupts();
        edit_s++;
        if (edit_s > 59)
          edit_s = 0;
        interrupts();
      }
      break;
    case ALARM_MODE_EDITING:
      if (edit_cursor == 0)
      {
        noInterrupts();
        edit_h++;
        if (edit_h > 23)
          edit_h = 0;
        interrupts();
      }
      else if (edit_cursor == 1)
      {
        noInterrupts();
        edit_m++;
        if (edit_m > 59)
          edit_m = 0;
        interrupts();
      }
      else if (edit_cursor == 2)
      {
        noInterrupts();
        edit_s++;
        if (edit_s > 59)
          edit_s = 0;
        interrupts();
      }
      break;
    default:
      break;
  }
  if (DEBUG_MENU)
  {
    Serial.println("TR" + String(MODE));
  }
}

void isrBtnLL() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

  //Serial.println("M");
  //MODE++;
  //if (MODE > 8) {
  //  MODE = 0;
  //}

  if (MODE == CLOCK_MODE) {
    MODE = STOPWATCH_MODE;
  } else if (MODE == STOPWATCH_MODE) {
    MODE = TIMER_MODE;
  } else if (MODE == TIMER_MODE) {
    MODE = ALARM_MODE;
    noInterrupts();
    edit_h = timeAlarmS / 3600;
    edit_m = (timeAlarmS % 3600) / 60;
    edit_s = timeAlarmS % 60;
    interrupts();
  } else if (MODE == ALARM_MODE) {
    MODE = CLOCK_MODE;
  }

  switch (MODE)
  {
    case STOPWATCH_MODE:
      setupStopWatchTimer();
      break;
    case TIMER_MODE:
      setupTimer();
      break;
    default:
      break;
  }
  if (DEBUG_MENU)
  {
    Serial.println("LL" + String(MODE));
  }
}

void isrBtnLR() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  
  if (MODE == STOPWATCH_MODE || MODE == STOPWATCH_MODE_RUNNING) {
    resetStopWatchTimer();
  }

  switch (MODE)
  {
    case STOPWATCH_MODE:
    case STOPWATCH_MODE_RUNNING:
      resetStopWatchTimer();
      break;
    case TIMER_MODE:
    case TIMER_MODE_RUNNING:
      resetTimer();
      break;
    case TIMER_MODE_EDITING:
      edit_cursor++;
      if (edit_cursor > 2) {
        noInterrupts();
        timeS = edit_h * 3600UL + edit_m * 60UL + edit_s;
        MODE = TIMER_MODE;
        setupTimer(); // Always update timeS before calling this
        interrupts();
      }
      break;
    case ALARM_MODE_EDITING:
      edit_cursor++;
      if (edit_cursor > 2) {
        noInterrupts();
        timeAlarmS = edit_h * 3600UL + edit_m * 60UL + edit_s;
        MODE = ALARM_MODE;
        //setupTimer(); // TODO: Adjunst ALARRME AQUI // Always update timeS before calling this
        interrupts();
      }
      break;
    default:
      break;
  }
  if (DEBUG_MENU)
  {
    Serial.println("LR" + String(MODE));
  }
}

// Blinking Functions
void changeBlinkingState() {
  noInterrupts();
  edit_blink_state = !edit_blink_state;
  interrupts();
}

void setupBlinkingTimer() {
  Timer1.initialize(750000); // 1000 microssegundos = 1 milissegundo
  Timer1.attachInterrupt(changeBlinkingState);
}

// Stopwatch Functions

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
  noInterrupts();
  timeCs++;
  interrupts();
}

void setupStopWatchTimer() {
  Timer1.initialize(10000); // 1000 microssegundos = 1 milissegundo
  Timer1.attachInterrupt(incrementStopWatchTime);
  Timer1.stop();
  noInterrupts();
  timeCs = 0;
  interrupts();
}

// Timer functions
void startTimer() {
  Timer1.start();
  noInterrupts();
  MODE = TIMER_MODE_RUNNING;
  interrupts();
}

void stopTimer() {
  Timer1.stop();
  noInterrupts();
  MODE = TIMER_MODE;
  interrupts();
}

void resetTimer() {
  noInterrupts();
  timeS = edit_h * 3600UL + edit_m * 60UL + edit_s;
  interrupts();
}

void decrementTime() {
  timeS--;
}

void setupTimer() {
  Timer1.initialize(1000000); // 1000 microssegundos = 1 milissegundo
  Timer1.attachInterrupt(decrementTime);
  Timer1.stop();
  edit_h = timeS / 3600;
  edit_m = (timeS % 3600) / 60;
  edit_s = timeS % 60;
}