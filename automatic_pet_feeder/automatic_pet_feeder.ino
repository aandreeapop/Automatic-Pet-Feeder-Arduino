//C++ code
//
#include <Servo.h>
#include <LiquidCrystal.h>
#include "RTClib.h"
#include <Wire.h>
#include <SoftwareSerial.h>

//servo opening (deg) and open time(miliseconds)
#define OPEN_DEG 60
#define FEED_WAIT 500   

#define rxPin 5
#define txPin 6

SoftwareSerial bt(txPin, rxPin);

int buttonState = 0;

char lastInp='-';
DateTime lastInpTime;

LiquidCrystal lcd(A0, 7, A1, 10, 11, 12, 13);
RTC_DS1307 rtc;

int hf[10], mf[10], sf[10], aux[10];

int auxIdx, feedIdx, feedCount;

DateTime lastFeed;

Servo servo_10;

void setup_lcd()
{
  lcd.begin(16,2);
}


void bt_setup()  {

    bt.begin(9600);    
    // Set the baud rate for the SoftwareSerial object
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Started");
  pinMode(A3, INPUT);
  servo_10.attach(3, 500, 2500);


  setup_lcd();

  Wire.begin();

  rtc.begin(); 

  bt_setup();

  rtc.adjust(DateTime(__DATE__, __TIME__));
  
}


void loop()
{
  DateTime now = rtc.now();

  lcd.clear();

  if(feedCount>0)
  {
    lcd.print("Next feed:");
    lcd.print(hf[feedIdx]);
    lcd.print(":");
    if(mf[feedIdx]<=9)  // add a 0 in front of digits
    {
      lcd.print(0);
    }
    lcd.print(mf[feedIdx]);
  }
  else
  {
    lcd.print("No feed set");
  }

  //second line
  lcd.setCursor(0,1);

  switch (lastInp){
    case '-':
      lcd.print("Input command");
      break;

    case 'e':
      lcd.print("Duplicate time");
      break;

    case 'E':
      lcd.print("Input error");
      break;

    case 'D':
      lcd.print("Command done");
      break;

    case 'f':
      lcd.print("Input feed");
      Serial.print("Command f");
      auxIdx = 0;
      while(auxIdx<5)
      {
        if(bt.available())
        {
          char a = bt.read();
          if(isdigit(a) || (a==':' && auxIdx==2))
          {
            lcd.print(a);
            Serial.print(a);
            aux[auxIdx++]=a;
          }
          delay(20);
        }
      }
      Serial.println("Finished time input");
      int hour = (aux[0]-'0')*10 + (aux[1]-'0');
      int minute = (aux[3]-'0')*10 + (aux[4]-'0');
      if(hour>23 || hour<0 || minute > 59 || minute < 0)
      {
        lastInp = 'E';  //error generic
        break;
      }

      //insert in order
      int i=0;
      while(i<feedCount && (hour>hf[i] || (hour==hf[i] && minute>mf[i])))
      {
        i++;
      }
      if(i==feedCount)   //this is the latest time yet; add
      {
        hf[i]=hour;
        mf[i]=minute;
        feedCount++;
        Serial.println(feedCount);
      }
      else
      {
        if(hour==hf[i] && minute==mf[i])   //this exact time is already added
        {
          lastInp = 'e';  //error duplicate 
          break;
        }
        else        //shift and add
        {
          for(int j=feedCount;j>i;j--)
          {
            hf[j]=hf[j-1];
            mf[j]=mf[j-1];
          }
          hf[i]=hour;
          mf[i]=minute;
          feedCount++;
        }
      }
      lastInp = 'D';
      break;
        
  }
  
  if(lastInp=='l')
  {
    Serial.print("Command l");
    char listComm=' ';
    int printIdx = feedIdx;
    while(listComm!='e')
    {
      if(printIdx==feedCount)
      {
        printIdx=0;
      }
      lcd.clear();
      lcd.print("List.Comm:n,r,e");
      lcd.setCursor(0,1);
      lcd.print(printIdx+1);
      lcd.print("-");
      lcd.print(hf[printIdx]);
      lcd.print(":");
      if(mf[printIdx]<=9)  // add a 0 in front of digits
      {
        lcd.print(0);
      }
      lcd.print(mf[printIdx]);
      if(bt.available())
      {
        char a=bt.read();
        if(isalpha(a))
        {
          listComm = a;
        }
      }
      if(listComm=='n')
      {
        printIdx++;
        listComm=' ';
      }
      if(listComm=='r')
      {
        //shift to remove
        for(int j=printIdx;j<feedCount;j++)
        {
          hf[j]=hf[j+1];
          mf[j]=mf[j+1];
        }
        feedCount--;
        listComm=' ';
      }

      delay(20);
    }
    lastInp='D';
  }
  now = rtc.now();

  if((now-lastInpTime).seconds() > 5)
  {
    lastInp = '-';
  }
  
  

  if(now.hour()==hf[feedIdx] && now.minute()==mf[feedIdx])
  {
    lcd.clear();
    lcd.print("Feeding.....");
    servo_10.write(OPEN_DEG);
    delay(FEED_WAIT);
    servo_10.write(0);
    feedIdx++;
  }

  //put feedIdx in the correct spot
  while(feedIdx<feedCount && (now.hour()>hf[feedIdx] || (now.hour()==hf[feedIdx] && now.minute()>mf[feedIdx])))
  {
    feedIdx++;
  }
  if(feedIdx==feedCount)
  {
    feedIdx=0;
  }

  if(now.hour()==hf[feedIdx] && now.minute()==mf[feedIdx])
  {
    feedIdx++;
    lcd.clear();
    lcd.print("Just fed");
    delay(60000); //wait a minute
  }

  if(bt.available())
  {
    char inp = bt.read();
    Serial.print(inp);
    if(isalpha(inp))
    {
      lastInp = inp;
      lastInpTime = rtc.now();
    }
  }

  delay(20); 
}

