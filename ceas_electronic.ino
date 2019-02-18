#define SSD1306_LCDHEIGHT 64
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "RTClib.h"

#define OLED_RESET 4
RTC_DS1307 rtc;

unsigned char temperature_icon16x16[] =
{
  0b00000001, 0b11000000, //        ###      
  0b00000011, 0b11100000, //       #####     
  0b00000111, 0b00100000, //      ###  #     
  0b00000111, 0b11100000, //      ######     
  0b00000111, 0b00100000, //      ###  #     
  0b00000111, 0b11100000, //      ######     
  0b00000111, 0b00100000, //      ###  #     
  0b00000111, 0b11100000, //      ######     
  0b00000111, 0b00100000, //      ###  #     
  0b00001111, 0b11110000, //     ########    
  0b00011111, 0b11111000, //    ##########   
  0b00011111, 0b11111000, //    ##########   
  0b00011111, 0b11111000, //    ##########   
  0b00011111, 0b11111000, //    ##########   
  0b00001111, 0b11110000, //     ########    
  0b00000111, 0b11100000, //      ######     
};

 unsigned char clock_icon16x16[] =
{
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, //                 
  0b00000011, 0b11100000, //       #####     
  0b00000111, 0b11110000, //      #######    
  0b00001100, 0b00011000, //     ##     ##   
  0b00011000, 0b00001100, //    ##       ##  
  0b00110000, 0b00000110, //   ##         ## 
  0b00110000, 0b00000110, //   ##         ## 
  0b00110000, 0b11111110, //   ##    ####### 
  0b00110000, 0b10000110, //   ##    #    ## 
  0b00110000, 0b10000110, //   ##    #    ## 
  0b00011000, 0b10001100, //    ##   #   ##  
  0b00001100, 0b00011000, //     ##     ##   
  0b00000111, 0b11110000, //      #######    
  0b00000011, 0b11100000, //       #####     
  0b00000000, 0b00000000, //                 
};

char daysOfTheWeek[7][12] = {"duminica", "luni", "marti", "miercuri", "joi", "vineri", "sambata"};

Adafruit_SSD1306 display(OLED_RESET);
//Temperature variables
int Vo;
float R1 = 10000;
float logR2, R2, T, Tc, Tf;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
long previousMillis = 0;

//Menus variables
volatile byte set_mode = 0;
volatile int set_menu = -1;
volatile bool needAdjustment = 0;
volatile bool needDateAdjustment = 0;
volatile bool format2412 = 1;
volatile bool ampm = 1; //0 pentru AM, 1 pentru PM
volatile bool alarmSetted = 0;
volatile bool stopAlarm = 0;
volatile bool ring = 0;
volatile int userHour;
volatile int userHourAlarm;
volatile int userMinute;
volatile int userMinuteAlarm;
volatile int userSecond;
volatile int userDay = 1;
volatile int userMonth = 1;
volatile int userYear = 2000;

//Microphone Detection variables
int lastSoundValue;
int soundValue;
long lastNoiseTime = 0;
long currentNoiseTime = 0;
long lastNoiseChange = 0;
int soundMenuStatus = 0;
unsigned long t1; //Interval de timp pentru bucla ce se ocupa de afisare

void setup()   {

  //activare int0 si int1
  EIMSK |= (1 << INT0);
  EIMSK |= (1 << INT1);
  //activare PCI2 (buton 3 - PCINT20 - PCI2)
  PCICR  |= (1 << PCIE2);
  PCMSK2 |= (1<<PCINT20);

  pinMode(A0, INPUT);
  pinMode(7, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(10,0);
  display.display();

//    following line sets the RTC to the date & time this sketch was compiled
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//    This line sets the RTC with an explicit date & time, for example to set
//    January 21, 2014 at 3am you would call:
//    rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  Vo = analogRead(0); //Pin thermistor = A0, initial reading (avoiding -273.15 for first 4 seconds);
  t1 = millis();

}

void loop() {
      soundValue = digitalRead(7);
            if (soundValue == 0) { // if there is currently a noise
      currentNoiseTime = millis();
      if (
        (currentNoiseTime > lastNoiseTime + 200) && // to debounce a sound occurring in more than a loop cycle as a single noise
        (lastSoundValue == 1) &&  // if it was silent before
        (currentNoiseTime < lastNoiseTime + 800) && // if current clap is less than 0.8 seconds after the first clap
        (currentNoiseTime > lastNoiseChange + 1000) // to avoid taking a third clap as part of a pattern
      ) {
  
        soundMenuStatus = !soundMenuStatus;
        lastNoiseChange = currentNoiseTime;
       }
  
       lastNoiseTime = currentNoiseTime;
      }
  
      lastSoundValue = soundValue;
      
    DateTime now = rtc.now();
    temperatureCheck();
    
    if(millis() - t1 > 300) {
      
    if(userHourAlarm == now.hour() && userMinuteAlarm == now.minute() && set_menu && alarmSetted == 1) {
      if(stopAlarm) {
        digitalWrite(LED_BUILTIN, HIGH);
        noTone(8);
      }
      else {
        tone(8, 1000, 500);
        ring = 1;
      }
    }
    if( now.minute() == userMinuteAlarm+1 && alarmSetted == 1)
       alarmSetted = 0;

    if(soundMenuStatus) {
      display.clearDisplay();
      display.drawCircle(64,32,30,WHITE);
      display.fillCircle(55,24,3,WHITE);
      display.fillCircle(75,24,3,WHITE);
      display.drawPixel(54,45,WHITE);
      display.drawPixel(55,46,WHITE); 
      display.drawPixel(55,47,WHITE);
      display.drawPixel(56,47,WHITE);
      display.drawPixel(56,48,WHITE);
      display.drawPixel(57,48,WHITE);
      display.drawPixel(58,49,WHITE);
      for(int i = 57;i<75;i++)
        display.drawPixel(i,49,WHITE);
      for(int i = 57;i<75;i++)
        display.drawPixel(i,50,WHITE);
      display.drawPixel(54,45,WHITE); 
      display.drawPixel(73,48,WHITE); 
      display.drawPixel(74,49,WHITE);
      display.drawPixel(75,48,WHITE);
      display.drawPixel(76,48,WHITE);
      display.drawPixel(76,47,WHITE);
      display.drawPixel(77,46,WHITE);
      display.drawPixel(78,45,WHITE);           
      display.display();
    }
    else {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,0);
    if(format2412) {
      int hr_12, hr_24;
      hr_24 = now.hour();
      if(hr_24 == 0)
        hr_12 = 12;
      else if(hr_24 == 12)
        hr_12 = hr_24;
      else
        hr_12 = hr_24%12;
  
      display.print(hr_12, DEC);
    }
  else
    display.print(now.hour(), DEC);
  display.print(":");
  if(now.minute() < 10)
    display.print(0);
  display.print(now.minute(), DEC);
  display.print(":");
  if(now.second() < 10)
    display.print(0);
  display.print(now.second(), DEC);
  if(format2412) {
    if(now.hour() < 12)
      display.print("AM");
    else
      display.print("PM");
  }
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,30);
  display.print(now.day(), DEC);
  display.print("/");
  display.print(now.month(), DEC);
  display.print("/");
  display.print(now.year(), DEC);
  display.print("   ");
  display.print(daysOfTheWeek[now.dayOfTheWeek()]);
  display.drawBitmap(0,48,temperature_icon16x16, 16,16,1);
  display.setCursor(16,50);
  display.print(" ");
  display.print(Tc);
  display.print((char)247);display.print("C");
  if(alarmSetted == 1) {  
    display.drawBitmap(100,48,clock_icon16x16, 16,16,1);   
  }
  
  display.setCursor(0,0);
  
  if (set_mode!=0) 
   {  
     switch (set_mode) {
       case 1:
          display.clearDisplay();
          display.setCursor(0,0);
          display.print("#######SETARI#######");
          display.setCursor(0,10);
          display.print("> Ora");
          display.setCursor(0,20);
          display.print("Data");
          display.setCursor(0,30);
          display.print("Alarma");
          display.setCursor(0,40);
          display.print("Iesire");
          break;
       case 2:
          display.clearDisplay();
          display.setCursor(0,0);
          display.print("#######SETARI#######");
          display.setCursor(0,10);
          display.print("Ora");
          display.setCursor(0,20);
          display.print("> Data");
          display.setCursor(0,30);
          display.print("Alarma");
          display.setCursor(0,40);
          display.print("Iesire");
          break;
       case 3:
          display.clearDisplay();
          display.setCursor(0,0);
          display.print("#######SETARI#######");
          display.setCursor(0,10);
          display.print("Ora");
          display.setCursor(0,20);
          display.print("Data");
          display.setCursor(0,30);
          display.print("> Alarma");
          display.setCursor(0,40);
          display.print("Iesire");
          break;
       case 4:
          display.clearDisplay();
          display.setCursor(0,0);
          display.print("#######SETARI#######");
          display.setCursor(0,10);
          display.print("Ora");
          display.setCursor(0,20);
          display.print("Data");
          display.setCursor(0,30);
          display.print("Alarma");
          display.setCursor(0,40);
          display.print("> Iesire");
          break;
     }
   }
   
   if (set_menu>-1) {  
     switch (set_menu) { //de la 0 la 5 -> menu ora
                         //de la 6 la 10 -> menu data
                         //de la 11 la 14 -> menu alarma
       case 0:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(0,0);
          if(format2412) {
            display.print("12h");
            //TODO AM/PM
            display.setCursor(100,0);
            if(ampm)
              display.print("PM");
            else
              display.print("AM");
          }
          else
            display.print("24h");
          display.drawLine(0,16,20,16,WHITE);
          display.setCursor(15,20);
          if(userHour < 10)
            display.print(0);
          display.print(userHour, DEC); 
          display.print(":");
          if(userMinute < 10)
            display.print(0);
          display.print(userMinute, DEC);
          display.print(":");
          if(userSecond < 10)
            display.print(0);
          display.print(userSecond, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;         
       case 1:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(0,0);
          if(format2412) {
            display.print("12h");
            //TODO AM/PM
            display.setCursor(100,0);
            if(ampm)
              display.print("PM");
            else
              display.print("AM");
          }
          else
            display.print("24h");       
          display.setCursor(15,40);
          display.drawLine(15,40,35,40,WHITE);
          display.setCursor(15,20);
          if(userHour < 10)
            display.print(0);
          display.print(userHour, DEC); 
          display.print(":");
          if(userMinute < 10)
            display.print(0);
          display.print(userMinute, DEC);
          display.print(":");
          if(userSecond < 10)
            display.print(0);
          display.print(userSecond, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;
       case 2:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(0,0);
          if(format2412) {
            display.print("12h");
            //TODO AM/PM
            display.setCursor(100,0);
            if(ampm)
              display.print("PM");
            else
              display.print("AM");   
          }         
          else
            display.print("24h");          
          display.setCursor(50,40);
          display.drawLine(50,40,70,40,WHITE);
          display.setCursor(15,20);
          if(userHour < 10)
            display.print(0);
          display.print(userHour, DEC); 
          display.print(":");
          if(userMinute < 10)
            display.print(0);
          display.print(userMinute, DEC);
          display.print(":");
          if(userSecond < 10)
            display.print(0);
          display.print(userSecond, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;
       case 3:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(0,0);
          if(format2412) {
            display.print("12h");
            //TODO AM/PM
            display.setCursor(100,0);
            if(ampm)
              display.print("PM");
            else
              display.print("AM");
          }
          else
            display.print("24h");     
          display.setCursor(85,40);
          display.drawLine(85,40,105,40,WHITE);
          display.setCursor(15,20);
          if(userHour < 10)
            display.print(0);
          display.print(userHour, DEC); 
          display.print(":");
          if(userMinute < 10)
            display.print(0);
          display.print(userMinute, DEC);
          display.print(":");
          if(userSecond < 10)
            display.print(0);
          display.print(userSecond, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;
       case 4:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(0,0);
          if(format2412) {
            display.print("12h");
            //TODO AM/PM
            display.setCursor(100,0);
            if(ampm)
              display.print("PM");
            else
              display.print("AM");
          }
          else
            display.print("24h"); 
          display.setCursor(15,20);
          if(userHour < 10)
            display.print(0);
          display.print(userHour, DEC); 
          display.print(":");
          if(userMinute < 10)
            display.print(0);
          display.print(userMinute, DEC);
          display.print(":");
          if(userSecond < 10)
            display.print(0);
          display.print(userSecond, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          display.setCursor(0,60);
          display.drawLine(0,60,40,60,WHITE);
          break;
       case 5:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(0,0);
          if(format2412) {
            display.print("12h");
            //TODO AM/PM
            display.setCursor(100,0);
            if(ampm)
              display.print("PM");
            else
              display.print("AM");
          }
          else
            display.print("24h");
          display.setCursor(15,20);
          if(userHour < 10)
            display.print(0);
          display.print(userHour, DEC); 
          display.print(":");
          if(userMinute < 10)
            display.print(0);
          display.print(userMinute, DEC);
          display.print(":");
          if(userSecond < 10)
            display.print(0);
          display.print(userSecond, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          display.setCursor(0,60);
          display.drawLine(95,60,115,60,WHITE);
          break;
       case 6:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(5,40);
          display.drawLine(5,40,25,40,WHITE);
          display.setCursor(5,20);
          if(userDay < 10)
            display.print(0);
          display.print(userDay, DEC);
          display.print(":");
          if(userMonth < 10)
            display.print(0);
          display.print(userMonth, DEC);
          display.print(":");
          display.print(userYear, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;
       case 7:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(50,40);
          display.drawLine(40,40,60,40,WHITE);
          display.setCursor(5,20);
          if(userDay < 10)
            display.print(0);
          display.print(userDay, DEC);
          display.print(":");
          if(userMonth < 10)
            display.print(0);
          display.print(userMonth, DEC);
          display.print(":");
          display.print(userYear, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;
       case 8:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(75,40);
          display.drawLine(75,40,115,40,WHITE);
          display.setCursor(5,20);
          if(userDay < 10)
            display.print(0);
          display.print(userDay, DEC);
          display.print(":");
          if(userMonth < 10)
            display.print(0);
          display.print(userMonth, DEC);
          display.print(":");
          display.print(userYear, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;
       case 9:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(5,20);
          if(userDay < 10)
            display.print(0);
          display.print(userDay, DEC);
          display.print(":");
          if(userMonth < 10)
            display.print(0);
          display.print(userMonth, DEC);
          display.print(":");
          display.print(userYear, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          display.setCursor(0,60);
          display.drawLine(0,60,40,60,WHITE);
          break;
       case 10:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.setCursor(5,20);
          if(userDay < 10)
            display.print(0);
          display.print(userDay, DEC);
          display.print(":");
          if(userMonth < 10)
            display.print(0);
          display.print(userMonth, DEC);
          display.print(":");
          display.print(userYear, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          display.setCursor(0,60);
          display.drawLine(95,60,115,60,WHITE);
          break;
       case 11:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(1);
          display.setCursor(0,0);
          display.print("Format: HH:MM");
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.drawLine(40,38,60,38,WHITE);
          display.setCursor(40,20);
          if(userHourAlarm < 10)
            display.print(0);
          display.print(userHourAlarm, DEC);
          display.print(":");
          if(userMinuteAlarm < 10)
            display.print(0);
          display.print(userMinuteAlarm, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;
       case 12:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(1);
          display.setCursor(0,0);
          display.print("Format: HH:MM");
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.drawLine(75,38,95,38,WHITE);
          display.setCursor(40,20);
          if(userHourAlarm < 10)
            display.print(0);
          display.print(userHourAlarm, DEC);
          display.print(":");
          if(userMinuteAlarm < 10)
            display.print(0);
          display.print(userMinuteAlarm, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;
       case 13:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(1);
          display.setCursor(0,0);
          display.print("Format: HH:MM");
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.drawLine(0,60,40,60,WHITE);
          display.setCursor(40,20);
          if(userHourAlarm < 10)
            display.print(0);
          display.print(userHourAlarm, DEC);
          display.print(":");
          if(userMinuteAlarm < 10)
            display.print(0);
          display.print(userMinuteAlarm, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;
       case 14:
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(1);
          display.setCursor(0,0);
          display.print("Format: HH:MM");
          display.setTextColor(WHITE);
          display.setTextSize(2);
          display.drawLine(95,60,115,60,WHITE);
          display.setCursor(40,20);
          if(userHourAlarm < 10)
            display.print(0);
          display.print(userHourAlarm, DEC);
          display.print(":");
          if(userMinuteAlarm < 10)
            display.print(0);
          display.print(userMinuteAlarm, DEC);
          display.setCursor(0,45);
          display.print("Back    OK");
          break;
     }
   }
   if(needAdjustment) { //verificare flag din ISR
      if(userHour != 0 || userMinute != 0 || userSecond != 0)
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), userHour, userMinute, userSecond));     
      needAdjustment = 0;
   }
   if(needDateAdjustment) { //verificare flag din ISR
        rtc.adjust(DateTime(userYear, userMonth, userDay, now.hour(), now.minute(), now.second()));
      needDateAdjustment = 0;
   }

}
   
   display.display();
   t1 = millis();
    }

}

void temperatureCheck() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > 4000) {
    previousMillis = currentMillis;   
    Vo = analogRead(0); //Pin thermistor = A0;
  }
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  Tc = T - 273.15;
}

ISR(INT0_vect) {
  //alegere ecran din meniul de setari
  if (set_mode==0)
    set_mode = 1;
  else if (set_mode==4)
    set_mode = 0;
    
  else if (set_mode==1) {//accesare ecran setare ora
    set_menu++;
    if(set_menu > 5)
       set_menu = 0;
  }
  else if (set_mode==2 && set_menu >= 6) {//accesare ecran setare data
    set_menu++;
    if(set_menu > 10)
      set_menu = 6;
  }
  else if (set_mode==3 && set_menu >= 11) {
    set_menu++;
    if(set_menu == 15)
      set_menu = 11;
  }
  else if (set_mode==2) {
      set_menu=6;
  }
  else if (set_mode==3) {
      set_menu=11;
  }

  _delay_ms(400);
}

ISR(INT1_vect) { //buton +
   if(set_mode==1 && set_menu > -1) { //control menu ora -> buton 2
    if (set_menu == 0){
      if(format2412)
        format2412 = 0;
      else
        format2412 = 1;
    }
    if (set_menu == 1) {
      if(userHour == 23)
        userHour = 0;
      else
        userHour++;  
    }
    if (set_menu == 2) {
      if(userMinute == 59)
        userMinute = 0;
      else
        userMinute++;   
    }
    if (set_menu == 3) {
      if(userSecond == 59)
        userSecond = 0;
      else
        userSecond++;   
    }
    if (set_menu == 4) {
      set_menu = -1;
      set_mode = 1; 
    }
    if (set_menu == 5) {
      set_menu = -1;
      set_mode = 0; 
      needAdjustment = 1;
    }
  }
  else if(set_mode==2 && set_menu > 5) {//control menu data
    if (set_menu == 6) {
      if(userDay == 31)
        userDay = 1;
      else
        userDay++;   
    }
    if (set_menu == 7) {
      if(userMonth == 12)
        userMonth = 1;
      else
        userMonth++;   
    }
    if (set_menu == 8) {
        userYear++;   
    }
    if (set_menu == 9) {
      set_menu = -1;
      set_mode = 2; 
    }
    if (set_menu == 10) {
      set_menu = -1;
      set_mode = 0; 
      needDateAdjustment = 1;
    }
  }
  else if(set_mode==3 && set_menu > 10) {//control menu alarma
    if (set_menu == 11) {
      if(userHourAlarm == 23)
        userHourAlarm = 0;
      else
        userHourAlarm++;  
    }
    if (set_menu == 12) {
      if(userMinuteAlarm == 59)
        userMinuteAlarm = 0;
      else
        userMinuteAlarm++;  
  }
    if (set_menu == 13) {
      set_menu = -1;
      set_mode = 3; 
    }
    if (set_menu == 14) {
      set_menu = -1;
      set_mode = 0; 
      alarmSetted = 1;
      stopAlarm = 0;
    }
 }
      
   else if(set_mode==1)
      set_mode = 2;
   else if(set_mode==2)
      set_mode=3;
   else if(set_mode==3)
      set_mode=4;
   else if(set_mode==4)
      set_mode=1;

   if(ring == 1) {
    stopAlarm=1;
    alarmSetted=0;   
  }
  _delay_ms(400);
}

ISR(PCINT2_vect) { //buton -
  _delay_ms(120);
  if (!(PIND & 0x10)) {
    
  if(set_mode==1 && set_menu > 0) { //control menu ora -> buton 3
    if (set_menu == 1) {
      if(userHour == 0)
        userHour = 23;
      else
        userHour--;  
      }
    }
    if (set_menu == 2) {
      if(userMinute == 0)
        userMinute = 59;
      else
        userMinute--;   
    }
    if (set_menu == 3) {
      if(userSecond == 0)
        userSecond = 59;
      else
        userSecond--;   
    }
    if (set_menu == 4) {
      set_menu = -1;
      set_mode = 1; 
    }
    if (set_menu == 5) {
      set_menu = -1;
      set_mode = 0; 
      needAdjustment = 1;
    }
  }
  else if(set_mode==2 && set_menu > 5) {//control menu data
    if (set_menu == 6) {
      if(userDay == 1)
        userDay = 31;
      else
        userDay--;   
    }
    if (set_menu == 7) {
      if(userMonth == 1)
        userMonth = 12;
      else
        userMonth--;   
    }
    if (set_menu == 8) {
        userYear--;   
    }
    if (set_menu == 9) {
      set_menu = -1;
      set_mode = 2; 
    }
    if (set_menu == 10) {
      set_menu = -1;
      set_mode = 0; 
      needDateAdjustment = 1;
    }
  }
  else if(set_mode==3 && set_menu > 10) {//control menu alarma
    if (set_menu == 11) {
      if(userHourAlarm == 0)
        userHourAlarm = 23;
      else
        userHourAlarm--;  
    }
    if (set_menu == 12) {
      if(userMinuteAlarm == 0)
        userMinuteAlarm = 59;
      else
        userMinuteAlarm--;  
    }
    if (set_menu == 13) {
      set_menu = -1;
      set_mode = 3; 
    }
    if (set_menu == 14) {
      set_menu = -1;
      set_mode = 0; 
      alarmSetted = 1;
      stopAlarm = 0;
    }    
  }
  
   else if(set_mode==1)
      set_mode=4;
   else if(set_mode==2)
      set_mode = 1;
   else if(set_mode==3)
      set_mode=2;
   else if(set_mode==4)
      set_mode=3;

}
