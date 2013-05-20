#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <MemoryFree.h>


#define  Version  "   0.01 alpha"
//                 01234567890123456


LiquidCrystal_I2C lcd(0x20,16,2);  // set the LCD address to 0x20 for a 16 chars and 2 line display

#define TemperaturePin   A0
#define WaterLevelPin    A1
#define LEDPin           13
#define BuzzerPin        11
#define FanPin           A2
#define PumpPin          A3


const byte rows = 4; //four rows
const byte cols = 4; //three columns
char keys[rows][cols] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
//                    0123456789012345
char* MenuItems[6] = { "Set Temperature ",      // 0
                       "Set Buzzer      ",      // 1
                       "Set Fan         ",      // 2
                       "Set Pump        ",      // 3
                       "Set Backlight   ",      // 4
                       "Information     " };    // 5
                       


// ---- Keypad pins: 2, 3, 4, 5, 6, 7, 8, 9 ----

byte rowPins[rows] = { 2, 3, 4, 6 };
byte colPins[cols] = { 5, 7, 8, 9 };

//byte rowPins[rows] = {6, 8, 7, 9};  // 5, 4, 3, 2}; //connect to the row pinouts of the keypad
//byte colPins[cols] = {5, 4, 3, 2 }; //8, 7, 6, 9}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );


float CurrentTemp = 0;
volatile int TempThreshold = 30;
boolean FanStatus = false, PumpStatus = false, WaterLevel = false, Buzzer = false, StayInside = true, FanEnabled = false, PumpEnabled = false, Backlight=true;


#define    Debug    true


// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Setup / Initialize Procedure

void setup()
{
  keypad.setHoldTime(30);
  pinMode(PumpPin, INPUT_PULLUP);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(LEDPin, OUTPUT);
  lcd.init();                      // initialize the lcd 
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
  if (ReadFromEEPROM(2)==1)
    Buzzer = true;
  else
    Buzzer = false;
  if (ReadFromEEPROM(4)==1)
    FanStatus = true;
  else
    FanStatus = false;
  if (ReadFromEEPROM(6)==1)
    PumpStatus = true;
  else
    PumpStatus = false;
  for (int i=0; i<5; i++)
  {
    digitalWrite(LEDPin, HIGH);
    delay(100);
    digitalWrite(LEDPin, LOW);
    delay(100);
  }
  if (Debug)
  {
    Serial.begin(9600);
    Serial.println("Arduarium Started!");
    Serial.println("");
    Serial.print("Current Temperature: ");
    Serial.print(CurrentTemp);
    Serial.println("C");
    Serial.print("Temperature Threshold: ");
    Serial.print(TempThreshold);
    Serial.println("C");
    Serial.print("Buzzer: ");
    if (Buzzer)
      Serial.println("Enabled");
    else
      Serial.println("Disabled");
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
  }
  InitializeScreen();
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Main Procedure

void loop()
{
  char key = keypad.getKey();
// ---
  if (key == 'A')    // We have a keypress  - A - Entering Menu
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
        case '*':                        // LEFT
          MenuSelection -= 1;
          if (MenuSelection < 0)
            MenuSelection = 5;
          break;
        case '#':                        // RIGHT
          MenuSelection += 1;
          if (MenuSelection > 5)
            MenuSelection = 0;
          break;
        case 'A':                        // ENTER MENU
          EnterMenu(MenuSelection);
          InitializeScreen();
          break;
        case 'D':                        // EXIT
          StayInside = false;
          InitializeScreen();
          break;
      }
    }
  }
  if (key == 'B')
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
  
  if (key == 'C')
  {
    lcd.clear();
    //         0123456789012345
    lcd.print("  Manual Mode   ");
    lcd.setCursor(0,1);
    lcd.print("  FAN ENABLED   ");
    digitalWrite(FanPin, HIGH);
    while (keypad.getKey()!='C') {};
    digitalWrite(FanPin, LOW);
    InitializeScreen();
  }
// ---  
  
  if (Temperature()>=TempThreshold)
  {
    FanStatus = true;
    if (!FanEnabled)
    {
      digitalWrite(FanPin, HIGH);
      digitalWrite(LEDPin, HIGH);
      FanEnabled = true;
      Beep(1);
    }
  }
  else
  {
    FanStatus = false;
    if (FanEnabled)
    {
      digitalWrite(FanPin, LOW);
      digitalWrite(LEDPin, LOW);
      FanEnabled = false;
      Beep(2);
    }
  }
  if (PumpReading())
  {
    PumpStatus = true;
    if (!PumpEnabled)
    {
      digitalWrite(PumpPin, HIGH);
      digitalWrite(LEDPin, HIGH);
      PumpEnabled = true;
      Beep(1);
    }
  }
  else
  {
    PumpStatus = false;
    if (PumpEnabled)
    {
      digitalWrite(PumpPin, LOW);
      digitalWrite(LEDPin, LOW);
      PumpEnabled = false;
      Beep(2);
    }
  }

  
  if (FanStatus)
  {
    lcd.setCursor(4,1);
    lcd.print("ON ");
    Beep(1);
  }
  else
  {
    lcd.setCursor(4,1);
    lcd.print("OFF");
    Beep(2);
  }
  if (PumpStatus)
  {
    lcd.setCursor(13,1);
    lcd.print("ON ");
    Beep(1);
  }
  else
  {
    lcd.setCursor(13,1);
    lcd.print("OFF");
    Beep(2);
  }
  if (Temperature() != CurrentTemp)
  {
    CurrentTemp = Temperature();
    lcd.setCursor(6,0);
    lcd.print(CurrentTemp,2);
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
  if (digitalRead(PumpPin)==HIGH)
    return true;
  else
    return false;
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Make Sounds

void Beep(int x)
{
  if (Buzzer)
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
// Clear Screen / Standby mode

void InitializeScreen()
{
  lcd.clear();
  //         0123456789012345
  lcd.print("Temp:      C    ");
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
  boolean tmpBuzzer = Buzzer;
  boolean tmpPumpEnabled = PumpEnabled;
  boolean tmpFanEnabled = FanEnabled;
  boolean tmpBacklight = Backlight;
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
            Beep(1);
            break;
          case 'D':
            Beep(2);
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
            tmpBuzzer = !(tmpBuzzer);
            break;
          case '#':
            tmpBuzzer = !(tmpBuzzer);
            break;
          case 'A':
            Buzzer = tmpBuzzer;
            if (Buzzer)
              SaveToEEPROM(2,1);
            else
              SaveToEEPROM(2,0);            
            waitforkey = false;
            Beep(1);
            break;
          case 'D':
            Beep(2);
            waitforkey = false;
            break;
        }
        lcd.setCursor(8,1);
        if (tmpBuzzer)
          lcd.print("ON ");
        else
          lcd.print("OFF");
      }
      StayInside = false;      
      break;
      
    case 2:
      lcd.clear();
      lcd.print(MenuItems[2]);
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Fan:            ");
      tmpFanEnabled=FanEnabled;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpFanEnabled = !(tmpFanEnabled);
            break;
          case '#':
            tmpFanEnabled = !(tmpFanEnabled);
            break;
          case 'A':
            FanEnabled = tmpFanEnabled;
            if (FanEnabled)
              SaveToEEPROM(4,1);
            else
              SaveToEEPROM(4,0);
            waitforkey = false;
            Beep(1);
            break;
          case 'D':
            Beep(2);
            waitforkey = false;
            break;
        }
        lcd.setCursor(5,1);
        if (tmpFanEnabled)
          lcd.print("ON ");
        else
          lcd.print("OFF");
      }      
      StayInside = false;      
      break;
      
      
      
    case 3:
      lcd.clear();
      lcd.print(MenuItems[3]);
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Pump:           ");
      tmpPumpEnabled=PumpEnabled;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpPumpEnabled = !(tmpPumpEnabled);
            break;
          case '#':
            tmpPumpEnabled = !(tmpPumpEnabled);
            break;
          case 'A':
            PumpEnabled = tmpPumpEnabled;
            if (PumpEnabled)
              SaveToEEPROM(6,1);
            else
              SaveToEEPROM(6,0);
            waitforkey = false;
            Beep(1);
            break;
          case 'D':
            Beep(2);
            waitforkey = false;
            break;
        }
        lcd.setCursor(6,1);
        if (tmpPumpEnabled)
          lcd.print("ON ");
        else
          lcd.print("OFF");
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
            Beep(1);
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
            break;
          case 'D':
            Beep(2);
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
      //         0123456789012345
      lcd.print("    (c) 2013    ");
      lcd.setCursor(0,1);
      lcd.print("Antonis Maglaras");
      while (keypad.getKey() == NO_KEY) {};
      lcd.clear();
      //         0123456789012345
      lcd.print("    Version");
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
