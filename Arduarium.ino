#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <MemoryFree.h>


#define  Version  "     1.20b"
//                 0123456789012345


LiquidCrystal_I2C lcd(0x20,16,2);  // set the LCD address to 0x20 for a 16 chars and 2 line display

#define TemperaturePin   A1  // Temperature Sensor
#define WaterLevelPin    A0  // Water Sensor
#define LEDPin           13  // LED
#define BuzzerPin        11  // Buzzer
#define FanPin           A2  // Fan -> Relay
#define PumpPin          A3  // Pump -> Relay


const byte rows = 4; //four rows
const byte cols = 4; //three columns
char keys[rows][cols] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
//                    0123456789012345
char* MenuItems[8] = { "Set Temperature ",      // 0
                       "Set Buzzer      ",      // 1
                       "Set Fan         ",      // 2
                       "Set Pump        ",      // 3
                       "Set Backlight   ",      // 4
                       "Set Serial port ",      // 5
                       "Factory Reset   ",      // 6
                       "Information     " };    // 7
                       


// ---- Keypad pins: 2, 3, 4, 5, 6, 7, 8, 9 ----

byte rowPins[rows] = { 2, 3, 4, 6 };
byte colPins[cols] = { 5, 7, 8, 9 };

//byte rowPins[rows] = {6, 8, 7, 9};  // 5, 4, 3, 2}; //connect to the row pinouts of the keypad
//byte colPins[cols] = {5, 4, 3, 2 }; //8, 7, 6, 9}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );


float CurrentTemp = 0;
volatile int TempThreshold = 30, Buzzer=0;
boolean FanStatus = false, PumpStatus = false, WaterLevel = false, StayInside = true, FanEnabled = false, PumpEnabled = false, Backlight=true, PumpActive = true, FanActive = true;

long UpdateMillis;

volatile boolean Debug = true;


// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Setup / Initialize Procedure

void setup()
{
  if (ReadFromEEPROM(10)==1)
  {
    Debug = true;
    delay(1000);
  }
  else
    Debug = false;
  keypad.setHoldTime(20);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(LEDPin, OUTPUT);
  digitalWrite(LEDPin, HIGH);
  pinMode(FanPin, OUTPUT);
  pinMode(PumpPin, OUTPUT);
  digitalWrite(FanPin, HIGH);              // Set relays to off, as when not powered
  digitalWrite(PumpPin, HIGH);             // 
  pinMode(WaterLevelPin, INPUT_PULLUP);
  lcd.init();                              // initialize the lcd 
  if (ReadFromEEPROM(8)==1)
  {
    lcd.backlight();
    Backlight=true;
  }
  else
  {
    lcd.noBacklight();
    Backlight=false;
  }
  lcd.clear();
  //         0123456789012345
  lcd.print("   Arduarium");
  lcd.setCursor(0,1);
  lcd.print("    WELCOME");
  delay(1000);
  TempThreshold = ReadFromEEPROM(0);
  Buzzer=ReadFromEEPROM(2);
  if (ReadFromEEPROM(4)==1)
    FanActive = true;
  else
    FanActive = false;
  if (ReadFromEEPROM(6)==1)
    PumpActive = true;
  else
    PumpActive = false;
  if (Debug)
  {
    Serial.begin(9600);
    Serial.println("Arduarium Started!");
    Serial.println("");
    Serial.print("Current Temperature: ");
    Serial.print(Temperature());
    Serial.println("C");
    Serial.print("Temperature Alert: ");
    Serial.print(TempThreshold);
    Serial.println("C");
    Serial.print("Buzzer: ");
    if (Buzzer==0)
      Serial.println("Disabled");
    else
    {
      Serial.print("Enabled (");
      Serial.print(Buzzer);
      Serial.println(" beeps)");
    }
    Serial.print("Fan: ");
    if (FanStatus)
      Serial.println("Enabled");
    else
      Serial.println("Disabled");
    Serial.print("Pump: ");
    if (PumpStatus)
      Serial.println("Enabled");
   else
      Serial.println("Disabled");
   Serial.print("LCD Backlight: ");
   if (Backlight)
     Serial.println("Enabled");
   else
     Serial.println("Disabled");
     
   Serial.println("");
   Serial.println("Started!");
   UpdateMillis=millis();
  }
  digitalWrite(LEDPin, LOW);  
  InitializeScreen();
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Main Procedure

void loop()
{
  char key = keypad.getKey();
  if (Debug)
  {
    if (millis()-UpdateMillis>5000)
    {
      Serial.println("- Update:");
      Serial.print("  Temperature: ");
      Serial.print(CurrentTemp);
      Serial.println("C");
      Serial.print("  Fan Status: ");
      if (FanStatus)
        Serial.println("Enabled");
      else
        Serial.println("Disabled");
      Serial.print("  Pump Status: ");
      if (PumpStatus)
        Serial.println("Enabled");
      else
        Serial.println("Disabled");
      UpdateMillis=millis();
    }
  }


  if (key == 'A')                              // We have a keypress  - A - Entering Menu
  {
    StayInside = true;
    int MenuSelection = 0;
    lcd.clear();
    lcd.print("Menu");
    while (StayInside)
    {
      lcd.setCursor(0,1);
      lcd.print(MenuItems[MenuSelection]);
      key = keypad.getKey();
      switch (key)
      {
        case '*':                              // LEFT
          MenuSelection -= 1;
          if (MenuSelection < 0)
            MenuSelection = 7;
          break;
        case '#':                              // RIGHT
          MenuSelection += 1;
          if (MenuSelection > 7)
            MenuSelection = 0;
          break;
        case 'A':                              // ENTER MENU
          EnterMenu(MenuSelection);
          InitializeScreen();
          break;
        case 'D':                              // EXIT
          StayInside = false;
          InitializeScreen();
          break;
      }
    }
  }
  if (key == 'B')                              // ENABLE/DISABLE LCD BACKLIGHT
  {
    if (Backlight)
    {
      lcd.noBacklight();
      Backlight=false;
    }
    else
    {
      lcd.backlight();
      Backlight=true;
    }
  }
  
  if (key == '*')                              // MANUAL ENABLE FAN 
  {
    lcd.clear();
    Beep(1);
    //         0123456789012345
    lcd.print("  Manual Mode   ");
    lcd.setCursor(0,1);
    lcd.print("  FAN ENABLED   ");
    digitalWrite(FanPin, LOW);
    while (keypad.getKey()!='*') {};
    digitalWrite(FanPin, HIGH);
    Beep(2);
    FanEnabled = false;
    FanStatus = false;
    InitializeScreen();
  }

  if (key == '#')                              // MANUAL ENABLE PUMP 
  {
    lcd.clear();
    //         0123456789012345
    lcd.print("  Manual Mode   ");
    lcd.setCursor(0,1);
    lcd.print("  PUMP ENABLED  ");
    Beep(1);
    digitalWrite(PumpPin, LOW);
    while (keypad.getKey()!='#') {};
    digitalWrite(PumpPin, HIGH);
    FanEnabled = false;
    FanStatus = false;
    Beep(2);
    InitializeScreen();
  }
  
  if (FanActive)
  {
    if (abs(Temperature())>=TempThreshold)
    {
      FanStatus = true;
      if (!FanEnabled)
      {
        if (Debug)
        {
          Serial.print("Temperature is ");
          Serial.print(abs(Temperature()));
          Serial.println("C - Activating Fan!");
        }
        digitalWrite(FanPin, LOW);
        digitalWrite(LEDPin, HIGH);
        FanEnabled = true;
        MoreBeeps();
      }
    }
    else
    {
      FanStatus = false;
      if (FanEnabled)
      {
        if (Debug)
        {
          Serial.print("Temperature is ");
          Serial.print(abs(Temperature()));
          Serial.println("C - De-activating Fan!");
        }
        digitalWrite(FanPin, HIGH);
        digitalWrite(LEDPin, LOW);
        FanEnabled = false;
        Beep(2);
      }
    }
  }  
  
  if (PumpActive)
  {
    if (PumpReading())
    {
      PumpStatus = true;
      if (!PumpEnabled)
      {
        if (Debug)
          Serial.println("Water level is LOW. Activating Pump!");
        if (PumpActive)
          digitalWrite(PumpPin, LOW);
        digitalWrite(LEDPin, HIGH);
        PumpEnabled = true;
        MoreBeeps();
      }
    }
    else
    {
      PumpStatus = false;
      if (PumpEnabled)
      {
        if (Debug)
          Serial.println("Water level is normal. De-activating Pump!");
        if (PumpActive)
          digitalWrite(PumpPin, HIGH);
        digitalWrite(LEDPin, LOW);
       PumpEnabled = false;
       Beep(2);
      }
    }
  }

  if (FanStatus)
  {
    lcd.setCursor(4,1);
    lcd.print("ON ");
  }
  else
  {
    lcd.setCursor(4,1);
    lcd.print("OFF");
  }
  if (PumpStatus)
  {
    lcd.setCursor(13,1);
    lcd.print("ON ");
  }
  else
  {
    lcd.setCursor(13,1);
    lcd.print("OFF");
  }
  if (Temperature() != CurrentTemp)
  {
    CurrentTemp = Temperature();
    lcd.setCursor(6,0);
    lcd.print(CurrentTemp,1);
  }
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Read temperature from sensor

float Temperature()
{
  int Reading=analogRead(TemperaturePin);
  float TempC = Reading * 5.0;
  TempC /= 1024.0;
  float Output = (TempC - 0.5) * 100;
  return Output;  
}


boolean PumpReading()
{
  if (digitalRead(WaterLevelPin)==HIGH)
    return true;
  else
    return false;
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Make Beep

void Beep(int x)
{
  if (Buzzer!=0)
  {
    for (int y=0; y<x; y++)
    {
      digitalWrite(BuzzerPin, HIGH);
      delay(50);
      digitalWrite(BuzzerPin, LOW);
      delay(50);
    }
  }
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Make Beeps for alert

void MoreBeeps()
{
  if (Buzzer!=0)
  {
    for (int y=0; y<Buzzer; y++)
    {
      digitalWrite(BuzzerPin, HIGH);
      delay(50);
      digitalWrite(BuzzerPin, LOW);
      delay(50);
    }
  }
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Clear Screen / Standby mode

void InitializeScreen()
{
  lcd.clear();
  //         0123456789012345
  lcd.print("Temp:     C     ");
  lcd.setCursor(0,1);
  lcd.print("Fan:    Pump:   ");
  CurrentTemp = Temperature()-1;
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Main Menu / Settings

void EnterMenu(int x)
{
  int tmpTempThreshold = TempThreshold;
  int tmpDigit1 = (tmpTempThreshold/10), tmpDigit2 = (tmpTempThreshold % 10), tmpCurrentDigit=0;
  int tmpBuzzer = Buzzer;
  boolean tmpPumpActive = PumpActive;
  boolean tmpFanActive = FanActive;
  boolean tmpBacklight = Backlight;
  boolean tmpDebug = Debug;
  char KeyRead;
  boolean waitforkey = true;
  switch (x)
  {
    case 0:
      lcd.clear();
      lcd.print(MenuItems[0]);
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Temperature:   C");
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '0':
            if (tmpCurrentDigit==0)
            {
              tmpDigit1=0;
              tmpCurrentDigit=1;
            }
            else
            {
              tmpDigit2=0;
              tmpCurrentDigit=0;
            }
            break;          
          case '1':
            if (tmpCurrentDigit==0)
            {
              tmpDigit1=1;
              tmpCurrentDigit=1;
            }
            else
            {
              tmpDigit2=1;
              tmpCurrentDigit=0;
            }
            break;
          case '2':
            if (tmpCurrentDigit==0)
            {
              tmpDigit1=2;
              tmpCurrentDigit=1;
            }
            else
            {
              tmpDigit2=2;
              tmpCurrentDigit=0;
            }
            break;
          case '3':
            if (tmpCurrentDigit==0)
            {
              tmpDigit1=3;
              tmpCurrentDigit=1;
            }
            else
            {
              tmpDigit2=3;
              tmpCurrentDigit=0;
            }
            break;
          case '4':
            if (tmpCurrentDigit==0)
            {
              tmpDigit1=4;
              tmpCurrentDigit=1;
            }
            else
            {
              tmpDigit2=4;
              tmpCurrentDigit=0;
            }
            break;
          case '5':
            if (tmpCurrentDigit==0)
            {
              tmpDigit1=5;
              tmpCurrentDigit=1;
            }
            else
            {
              tmpDigit2=5;
              tmpCurrentDigit=0;
            }
            break;
          case '6':
            if (tmpCurrentDigit==0)
            {
              tmpDigit1=6;
              tmpCurrentDigit=1;
            }
            else
            {
              tmpDigit2=6;
              tmpCurrentDigit=0;
            }
            break;
          case '7':
            if (tmpCurrentDigit==0)
            {
              tmpDigit1=7;
              tmpCurrentDigit=1;
            }
            else
            {
              tmpDigit2=7;
              tmpCurrentDigit=0;
            }
            break;
          case '8':
            if (tmpCurrentDigit==0)
            {
              tmpDigit1=8;
              tmpCurrentDigit=1;
            }
            else
            {
              tmpDigit2=8;
              tmpCurrentDigit=0;
            }
            break;
          case '9':
            if (tmpCurrentDigit==0)
            {
              tmpDigit1=9;
              tmpCurrentDigit=1;
            }
            else
            {
              tmpDigit2=9;
              tmpCurrentDigit=0;
            }
            break;
          case 'A':
            tmpTempThreshold = (tmpDigit1*10) + (tmpDigit2);         
            TempThreshold = tmpTempThreshold;
            waitforkey = false;
            SaveToEEPROM(0,TempThreshold);            
            SavedMessage();            
            break;
          case 'D':
            NotSavedMessage();
            waitforkey = false;          
            break;
        }
        tmpTempThreshold = (tmpDigit1*10) + (tmpDigit2);
        lcd.setCursor(13,1);
        if (tmpTempThreshold < 10)
          lcd.print("0");
        lcd.print(tmpTempThreshold);
      }
      StayInside = false;      
      break;
 
    case 1:
      lcd.clear();
      lcd.print(MenuItems[1]);
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Buzzer:         ");
      tmpBuzzer=Buzzer;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpBuzzer -= 1;
            if (tmpBuzzer<0)
              tmpBuzzer=30;
            break;
          case '#':
            tmpBuzzer += 1;
            if (tmpBuzzer>30)
              tmpBuzzer=0;
            break;
          case 'A':
            Buzzer = tmpBuzzer;
            SaveToEEPROM(2,Buzzer);
            SavedMessage();
            waitforkey = false;
            break;
          case 'D':
            NotSavedMessage();            
            waitforkey = false;
            break;
        }
        lcd.setCursor(8,1);
        if (tmpBuzzer == 0)
          lcd.print("Disabled");
        else
        {
          if (tmpBuzzer<10)
            lcd.print("0");
          lcd.print(tmpBuzzer);
          lcd.print(" beeps");
        }
      }
      StayInside = false;      
      break;
      
    case 2:
      lcd.clear();
      lcd.print(MenuItems[2]);
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Fan:            ");
      tmpFanActive=FanActive;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpFanActive = !(tmpFanActive);
            break;
          case '#':
            tmpFanActive = !(tmpFanActive);
            break;
          case 'A':
            FanActive = tmpFanActive;
            if (FanActive)
              SaveToEEPROM(4,1);
            else
              SaveToEEPROM(4,0);
            waitforkey = false;
            SavedMessage();
            break;
          case 'D':
            NotSavedMessage();
            waitforkey = false;
            break;
        }
        lcd.setCursor(5,1);
        if (tmpFanActive)
          lcd.print("Active    ");
        else
          lcd.print("Not Active");
      }      
      StayInside = false;      
      break;
      
    case 3:
      lcd.clear();
      lcd.print(MenuItems[3]);
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Pump:           ");
      tmpPumpActive=PumpActive;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpPumpActive = !(tmpPumpActive);
            break;
          case '#':
            tmpPumpActive = !(tmpPumpActive);
            break;
          case 'A':
            PumpActive = tmpPumpActive;
            if (PumpActive)
              SaveToEEPROM(6,1);
            else
              SaveToEEPROM(6,0);
            waitforkey = false;
            SavedMessage();
            break;
          case 'D':
            NotSavedMessage();
            waitforkey = false;
            break;
        }
        lcd.setCursor(6,1);
        if (tmpPumpActive)
          lcd.print("Active    ");
        else
          lcd.print("Not Active");
      }    
      StayInside = false;      
      break;
      
    case 4:
      lcd.clear();
      lcd.print(MenuItems[4]);
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Backlight:      ");
      tmpBacklight=Backlight;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpBacklight = !(tmpBacklight);
            break;
          case '#':
            tmpBacklight = !(tmpBacklight);
            break;
          case 'A':
            Backlight=tmpBacklight;
            if (Backlight)
            {
              SaveToEEPROM(8,1);
              lcd.backlight();
            }
            else
            {
              SaveToEEPROM(8,0);
              lcd.noBacklight();
            }
            waitforkey = false;
            SavedMessage();            
            break;
          case 'D':
            NotSavedMessage();
            waitforkey = false;
            break;
        }
        lcd.setCursor(11,1);
        if (tmpBacklight)
          lcd.print("ON ");
        else
          lcd.print("OFF");
      }    
      StayInside = false;      
      break;

    case 5:
      lcd.clear();
      lcd.print(MenuItems[5]);
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Serial:         ");
      tmpDebug=Debug;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpDebug = !(tmpDebug);
            break;
          case '#':
            tmpDebug = !(tmpDebug);
            break;
          case 'A':
            Debug=tmpDebug;
            if (Debug)
              SaveToEEPROM(10,1);
            else
              SaveToEEPROM(10,0);
            waitforkey = false;
            SavedMessage();            
            break;
          case 'D':
            NotSavedMessage();
            waitforkey = false;
            break;
        }
        lcd.setCursor(8,1);
        if (tmpDebug)
          lcd.print("Enabled ");
        else
          lcd.print("Disabled");
      }    
      StayInside = false;      
      break;

    case 6:
      lcd.clear();
      lcd.print(MenuItems[6]);
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Are you sure?   ");
      Beep(3);
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case 'A':
            lcd.setCursor(0,1);
            //         0123456789012345
            lcd.print("SETTINGS CLEARED");
            SaveToEEPROM(0,30);
            SaveToEEPROM(2,5);
            SaveToEEPROM(4,1);
            SaveToEEPROM(6,1);
            SaveToEEPROM(8,1);
            SaveToEEPROM(10,0);
            delay(1500);            
            Beep(3);
            waitforkey = false;
            break;
          case 'D':
            lcd.setCursor(0,1);
            //         0123456789012345
            lcd.print("   CANCELED!    ");
            Beep(2);
            delay(1500);
            waitforkey = false;
            break;
        }
      }
      StayInside = false;      
      break;
        
    case 7:
      lcd.clear();
      //         0123456789012345
      lcd.print("    (c) 2013    ");
      lcd.setCursor(0,1);
      lcd.print("Antonis Maglaras");
      while (keypad.getKey() == NO_KEY) {};
      lcd.clear();
      //         0123456789012345
      lcd.print("    Version     ");
      lcd.setCursor(0,1);
      lcd.print(Version);
      while (keypad.getKey() == NO_KEY) {};
      lcd.clear();
      lcd.print("Free Memory");
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print(freeMemory());
      lcd.print(" bytes");
      while (keypad.getKey() == NO_KEY) {};
      StayInside = false;    
      Beep(2);  
      break; 
  }  
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Write to EEPROM

void SaveToEEPROM(int addr, int number)
{
  int a = number/256;
  int b = number % 256;
  EEPROM.write(addr,a);
  EEPROM.write(addr+1,b);
}  



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Read from EEPROM

int ReadFromEEPROM(int addr)
{
  int a=EEPROM.read(addr);
  int b=EEPROM.read(addr+1);

  return a*256+b;
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Display Not Saved Message and beep if enabled

void NotSavedMessage()
{
  lcd.setCursor(0,1);
  lcd.print("   NOT SAVED!   ");
  Beep(2);
  delay(1000);
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Display Saved Message and beep if enabled

void SavedMessage()
{
  lcd.setCursor(0,1);
  lcd.print("     SAVED!     ");
  Beep(1);
  delay(1000);
}
