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

// Editing and Blinker
int getMaxDay(int year, int month);
void changeEditVariableValue(volatile int * edit_variable, int max, int min, int delta);
void defaultChangeEditVariableValue(int delta);
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
void setupTimer();
// Alarm
void resetAlarm();


void generateBody();

// RTC e LCD

RTC_DS1307 rtc;
//LiquidCrystal LCD(R, E, D4, D5, D6, D7);
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte alarmOn[8] = {
  0b00100, //   #
  0b01110, //  ###
  0b01110, //  ###
  0b11111, // #####
  0b11111, // #####
  0b00100, //   #
  0b00100, //   #
  0b00000  //
};

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
int const CLOCK_MODE_EDITING = 1;
int const DATE_MODE_EDITING = 2;
int const STOPWATCH_MODE = 3;
int const STOPWATCH_MODE_RUNNING = 4;
int const TIMER_MODE = 5;
int const TIMER_MODE_RUNNING = 6;
int const TIMER_MODE_EDITING = 7;
int const ALARM_MODE = 8;
int const ALARM_MODE_EDITING = 9;
int const ALARM_MODE_RINGING = 10;

// Time variables
volatile unsigned long timeCs = 0;
volatile unsigned long timeS = 300;
volatile unsigned long timeAlarmS = 28800;
volatile bool ALARM_ON = true;

// MODO DE EDIÇÂO
#define MIN_YEAR 2000
#define MAX_YEAR 2099
volatile int edit_cursor = 0;
volatile int edit_h = 0;
volatile int edit_m = 0;
volatile int edit_s = 0;
volatile int edit_y = 2000;
volatile int edit_mo = 2;
volatile int edit_d = 1;
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
  lcd.createChar(6, alarmOn);

  if (!rtc.isrunning()) {
    lcd.print("RTC parado");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // só na 1ª vez
    delay(2000);
    lcd.clear();
  }
  DateTime now = rtc.now();
  unsigned long ccd =
    now.hour() * 3600UL +
    now.minute() * 60UL +
    now.second();
  timeAlarmS = ccd+10;
}


void loop() {
  DateTime now = rtc.now();
  unsigned long currentSeconds =
    now.hour() * 3600UL +
    now.minute() * 60UL +
    now.second();

   if (currentSeconds >= timeAlarmS && (currentSeconds - timeAlarmS) < 2 && ALARM_ON && MODE != ALARM_MODE_RINGING) {
      noInterrupts();
      MODE = ALARM_MODE_RINGING;
      setupBlinkingTimer();
      interrupts();
   }
   else if (MODE == ALARM_MODE_RINGING && (currentSeconds - timeAlarmS) > 10) {
      resetAlarm();
   }

  generateBody();
}

void printYyMmDdEdit() {
    char buffer[20];
    // Formato: YYYY:MO:DD
    sprintf(buffer, "%04d/%02d/%02d   ", edit_y, edit_mo, edit_d);
    if (edit_blink_state && edit_cursor == 3) 
    {
      // Set the cursor to blink on Year
      buffer[0] = ' ';
      buffer[1] = ' ';
      buffer[2] = ' ';
      buffer[3] = ' ';
    }
    else if (edit_blink_state && edit_cursor == 4) 
    {
      // Set the cursor to blink on Month
      buffer[5] = ' ';
      buffer[6] = ' ';
    }
    else if (edit_blink_state && edit_cursor == 5) 
    {
      // Set the cursor to blink on Date
      buffer[8] = ' ';
      buffer[9] = ' ';
    }
    lcd.setCursor(MENU_PADDING, 0);
    lcd.print(buffer);
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

void printHhMmSsDsFromCSeconds() {
    noInterrupts();
    int totalCSegundos = timeCs / 100;
    int h = totalCSegundos / 3600;
    int m = (totalCSegundos % 3600) / 60;
    int s = totalCSegundos % 60;
    int cs = timeCs % 100; // centésimos restantes
    cs = (cs == 1) ? 0: cs; // Gambiarra pra resolver o .01
    interrupts();

    char buffer[20];
    // Formato: HH:MM:SS.CC
    sprintf(buffer, "%02d:%02d:%02d.%02d", h, m, s, cs);
    lcd.setCursor(MENU_PADDING, 0);
    lcd.print(buffer);
}

void printAlarmRinging() {
    DateTime now = rtc.now();

    char linha1[17];
    char linha2[17];

    sprintf(linha1, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    if (edit_blink_state)
    {
      sprintf(linha1, "        ");
      tone(13, 262, 250); // Toca um tom de 262Hz por 0,250 segundos
    }
    
    lcd.setCursor(MENU_PADDING, 0);
    lcd.print(linha1);
}

void printHhMmSsYyMoDD() {
    DateTime now = rtc.now();

    char linha1[17];
    char linha2[17];

    sprintf(linha1, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    sprintf(linha2, "%04d/%02d/%02d", now.year(), now.month(), now.day());
          
    lcd.setCursor(MENU_PADDING, 0);
    lcd.print(linha1);
    lcd.setCursor(2, 1);
    lcd.print(linha2);
}

void generateBody() {
  printMenu(MODE);

  switch (MODE)
  {
    case STOPWATCH_MODE:
    case STOPWATCH_MODE_RUNNING:
      printHhMmSsDsFromCSeconds();
      break;
    case TIMER_MODE:
    case TIMER_MODE_RUNNING:
      printHhMmSsFromSeconds(timeS);
      break;
    case ALARM_MODE:
      printHhMmSsFromSeconds(timeAlarmS);
      break;
    case CLOCK_MODE:
      printHhMmSsYyMoDD();
      break;
    case CLOCK_MODE_EDITING:
      if (edit_cursor == -1)
      {
        DateTime now = rtc.now();
        noInterrupts();
        edit_cursor = 0;
        edit_y = now.year();
        edit_mo = now.month();
        edit_d = now.day();
        edit_h = now.hour();
        edit_m = now.minute();
        edit_s = now.second();
        interrupts();
      }
    case TIMER_MODE_EDITING:
    case ALARM_MODE_EDITING:
      printHhMmSsEdit();
      break;
    case DATE_MODE_EDITING:
      printYyMmDdEdit();
    case ALARM_MODE_RINGING:
      printAlarmRinging();
      break;
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

  //lcd.setCursor(0, 0);
  //lcd.print("E");
  //lcd.setCursor(0, 1);
  //lcd.print("M");
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
      printMenuButtons('E', ALARM_ON ? byte(6) : ' ', 'M', ' ');
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("CLOCK");
      break;
    case (CLOCK_MODE_EDITING):
      printMenuButtons(byte(2) /*↑*/, byte(3) /*↓*/, ' ', byte(5) /*➡*/);
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("CLOCKE");
      break;
    case (STOPWATCH_MODE):
      printMenuButtons(' ', byte(0) /*▶*/, 'M', 'R');
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("STW");
      break;
      case (STOPWATCH_MODE_RUNNING):
      printMenuButtons(' ', byte(4) /*⏸*/, ' ', 'R');
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
      printMenuButtons(byte(2) /*↑*/, byte(3) /*↓*/, ' ', byte(5) /*➡*/);
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("TIMEREd");
      break;
    case (ALARM_MODE):
      printMenuButtons('E', ALARM_ON ? byte(6) : ' ', 'M', ' ');
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("ALARM");
      break;
    case (ALARM_MODE_EDITING):
      printMenuButtons(byte(2) /*↑*/, byte(3) /*↓*/, ' ', byte(5) /*➡*/);
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("ALARMed");
      break;
    case (ALARM_MODE_RINGING):
      printMenuButtons(' ', 'x', ' ', ' ');
      //lcd.setCursor(MENU_PADDING, 0);
      //lcd.print("ALARMed");
      break;
    default:
      break;
  }
  if (MODE != CLOCK_MODE) {
    lcd.setCursor(2, 1);
    lcd.print("          ");
  }
}

// Rotinas de interrupção
void isrBtnTL() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

  switch (MODE)
  {
    case CLOCK_MODE:
      noInterrupts();
      MODE = CLOCK_MODE_EDITING;
      edit_cursor = -1; // Flag 
      interrupts();
      setupBlinkingTimer();
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
        changeEditVariableValue(&edit_h, 99, 0, 1);
      }
      else
      {
        defaultChangeEditVariableValue(1);
      }
      break;
    case ALARM_MODE_EDITING:
    case CLOCK_MODE_EDITING:
      defaultChangeEditVariableValue(1);
      break;
    case DATE_MODE_EDITING:
      if (edit_cursor == 3)
      {
        changeEditVariableValue(&edit_y, MAX_YEAR, MIN_YEAR, 1);
      }
      else if (edit_cursor == 4)
      {
        changeEditVariableValue(&edit_mo, 12, 1, 1);
      }
      else if (edit_cursor == 5)
      {
        changeEditVariableValue(&edit_d, getMaxDay(edit_y, edit_mo), 1, 1);
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
    case CLOCK_MODE:
    case ALARM_MODE:
      noInterrupts();
      ALARM_ON = !ALARM_ON;
      interrupts();
      break;
    case ALARM_MODE_RINGING:
      resetAlarm();
      break;
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
      if (edit_cursor == 0) {
        changeEditVariableValue(&edit_h, 99, 0, -1);
      }
      else {
        defaultChangeEditVariableValue(-1);
      }
      break;
    case ALARM_MODE_EDITING:
    case CLOCK_MODE_EDITING:
      defaultChangeEditVariableValue(-1);
      break;
    case DATE_MODE_EDITING:
      if (edit_cursor == 3)
      {
        changeEditVariableValue(&edit_y, MAX_YEAR, MIN_YEAR, -1);
      }
      else if (edit_cursor == 4)
      {
        changeEditVariableValue(&edit_mo, 12, 1, -1);
      }
      else if (edit_cursor == 5)
      {
        changeEditVariableValue(&edit_d, getMaxDay(edit_y, edit_mo), 1, -1);
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
        ALARM_ON = true;
        //setupTimer(); // TODO: Adjunst ALARRME AQUI // Always update timeS before calling this
        interrupts();
      }
      break;
    case CLOCK_MODE_EDITING:
      edit_cursor++;
      if (edit_cursor > 2) {
        noInterrupts();
        timeAlarmS = edit_h * 3600UL + edit_m * 60UL + edit_s;
        MODE = DATE_MODE_EDITING;
        interrupts();
      }
      break;
    case DATE_MODE_EDITING:
      edit_cursor++;
      if (edit_cursor > 5)
      {
        noInterrupts();
        MODE = CLOCK_MODE;
        interrupts();
      }
      
      //if (edit_cursor > 2) {
      //  noInterrupts();
      //  timeAlarmS = edit_h * 3600UL + edit_m * 60UL + edit_s;
      //  MODE = DATE_MODE_EDITING;
      //  //setupTimer(); // TODO: Ajustar horário do  // Always update timeS before calling this
      //  interrupts();
      //}
      break;
    default:
      break;
  }
  if (DEBUG_MENU)
  {
    Serial.println("LR" + String(MODE));
  }
}

// Editing and Blinking Functions
int getMaxDay(int year, int month) {
  // Handle months with fixed number of days
  switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      return 31;
    case 4: case 6: case 9: case 11:
      return 30;
    case 2:
      // Leap year check
      if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        return 29;
      } else {
        return 28;
      }
    default:
      return 0; // Invalid month
  }
}

void changeEditVariableValue(volatile int * edit_variable, int max, int min, int delta) {
  noInterrupts();
  *edit_variable += delta;
  
  if (*edit_variable > max)
  *edit_variable = min;
  else if (*edit_variable < min)
  *edit_variable = max;
  interrupts();
}

void defaultChangeEditVariableValue(int delta) {
  if (edit_cursor == 0) {
    changeEditVariableValue(&edit_h, 23, 0, delta);
  }
  else if (edit_cursor == 1) {
    changeEditVariableValue(&edit_m, 59, 0, delta);
  }
  else if (edit_cursor == 2) {
    changeEditVariableValue(&edit_s, 59, 0, delta);
  }
}

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
  resetStopWatchTimer();
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
  if (timeS <= 0)
  {
    Timer1.stop();
    noInterrupts();
    MODE = TIMER_MODE;
    resetTimer();
    interrupts();
  }
  
}

void setupTimer() {
  Timer1.initialize(1000000); // 1000 microssegundos = 1 milissegundo
  Timer1.attachInterrupt(decrementTime);
  Timer1.stop();
  edit_h = timeS / 3600;
  edit_m = (timeS % 3600) / 60;
  edit_s = timeS % 60;
}

// Alarm functions
void resetAlarm() {
  noInterrupts();
  MODE = CLOCK_MODE;
  Timer1.stop();
  interrupts();
}