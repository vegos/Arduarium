#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <MemoryFree.h>

#define  Version  "     2.00b"


LiquidCrystal_I2C lcd(0x20,16,2);  // set the LCD address to 0x20 for a 16 chars and 2 line display

#define TemperaturePin   A1  // Temperature Sensor
#define WaterLevelPin    A0  // Water Sensor
#define LEDPin           13  // LED
#define BuzzerPin        11  // Buzzer
#define FanPin           A2  // Fan -> Relay
#define PumpPin          A3  // Pump -> Relay

#define TempERROR        60  // Temperatures more than this equals to error

const byte rows = 4;         // Four rows
const byte cols = 4;         // Four columns
char keys[rows][cols] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

char* MenuItems[9] = { "Set Temperature ",      // 0
                       "Set Buzzer      ",      // 1
                       "Set Fan         ",      // 2
                       "Set Pump        ",      // 3
                       "Set Overfill    ",      // 4
                       "Set Temp Drop   ",      // 5
                       "Set Backlight   ",      // 6
                       "Factory Reset   ",      // 7
                       "Information     " };    // 8          

byte rowPins[rows] = { 2, 3, 4, 6 };
byte colPins[cols] = { 5, 7, 8, 9 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

int Sum,Total,CorrectedDisplayTemp, DisplayTemp = 0;
volatile int TempThreshold = 30, Buzzer=0, TemperatureDrop=0;
boolean FanStatus = false, 
        PumpStatus = false, 
        WaterLevel = false, 
        StayInside = true, 
        FanEnabled = false, 
        PumpEnabled = false, 
        Backlight=true, 
        PumpActive = true, 
        FanActive = true;
long SampleMillis;
int OverFillDelay = 0;



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Setup / Initialize Procedure

void setup()
{
  keypad.setHoldTime(20);
  pinMode(BuzzerPin, OUTPUT);              // Buzzer
  pinMode(LEDPin, OUTPUT);                 // LED
  pinMode(FanPin, OUTPUT);                 // Fan Relay
  pinMode(PumpPin, OUTPUT);                // Pump Relay
  digitalWrite(FanPin, HIGH);              // Set relay to off, as when not powered
  digitalWrite(PumpPin, HIGH);             // Set relay to off, as when not powered
  pinMode(WaterLevelPin, INPUT_PULLUP);    // Water Level Sensor pin
  lcd.init();                              // Initialize LCD
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
  TemperatureDrop = ReadFromEEPROM(10);
  OverFillDelay = ReadFromEEPROM(12);
  digitalWrite(LEDPin, LOW);  
  InitializeScreen();
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Main Procedure

void loop()
{
  char key = keypad.getKey();

  if (key == 'A')                              // We have a keypress  - A - Entering Menu
  {
    StayInside = true;
    int MenuSelection = 0;
    lcd.clear();
    lcd.print("Setup Menu");
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
            MenuSelection = 8;
          break;
        case '#':                              // RIGHT
          MenuSelection += 1;
          if (MenuSelection > 8)
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
    lcd.print("  Manual Mode   ");
    lcd.setCursor(0,1);
    lcd.print("  FAN ENABLED   ");
    digitalWrite(FanPin, LOW);
    Beep(1);
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
    lcd.print("  Manual Mode   ");
    lcd.setCursor(0,1);
    lcd.print(" PUMP  ENABLED  ");
    Beep(1);
    digitalWrite(PumpPin, LOW);
    while (keypad.getKey()!='#') {};
    digitalWrite(PumpPin, HIGH);
    FanEnabled = false;
    FanStatus = false;
    Beep(2);
    InitializeScreen();
  }
  
  if (FanActive)                               // If fan is enabled
  {
    if ((CorrectedDisplayTemp>=TempThreshold) && (CorrectedDisplayTemp<=TempERROR))
    {
      FanStatus = true;
      if (!FanEnabled)
      {
        digitalWrite(FanPin, LOW);             // Turn ON fan
        digitalWrite(LEDPin, HIGH);
        FanEnabled = true;
        AlertSound();
      }
    }
    else
    {
      if (CorrectedDisplayTemp>=(TempThreshold+1))
      {
        FanStatus = false;
        if (FanEnabled)
        {
          digitalWrite(FanPin, HIGH);            // Turn OFF fan
          digitalWrite(LEDPin, LOW);
          FanEnabled = false;
          Beep(2);
        }
      }
    }
  }
  
  if (PumpActive)                              // If pump is enabled
  {
    if (PumpReading())
    {
      PumpStatus = true;
      if (!PumpEnabled)
      {
        if (PumpActive)
          digitalWrite(PumpPin, LOW);
        digitalWrite(LEDPin, HIGH);
        PumpEnabled = true;
        AlertSound();
      }
    }
    else
    {
      PumpStatus = false;
      if (PumpEnabled)
      {
        if (PumpActive)
        {
          if (OverFillDelay!=0)
          {
            lcd.setCursor(0,1);
            lcd.print("Overfilling...  ");
            Beep(1);
            delay(OverFillDelay*1000);
            InitializeScreen();
          }
          digitalWrite(PumpPin, HIGH);
        }
        digitalWrite(LEDPin, LOW);
       PumpEnabled = false;
       Beep(2);
      }
    }
  }


  // Displaying on LCD the status / temperature / etc
  
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

  Total+=1;
  Sum+=Temperature();
  CorrectedDisplayTemp=Sum/Total;
  if (CorrectedDisplayTemp != DisplayTemp)
  {
    DisplayTemp = CorrectedDisplayTemp;
    lcd.setCursor(6,0);
    if (CorrectedDisplayTemp > TempERROR)
      lcd.print("ER");
    else
    {
      if (CorrectedDisplayTemp<10)
        lcd.print("0");
      lcd.print(CorrectedDisplayTemp,1);
    }
  }
  if (millis()-SampleMillis>1000)
  {
    Total=0;
    Sum=0;
    SampleMillis=millis();
  }  
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Read temperature from sensor

int Temperature()
{
  int TempC =  ((analogRead(TemperaturePin) * 5.0 * 100.0) / 1024.0);
  return TempC;  
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Read Water Level from sensor

boolean PumpReading()
{
  if (digitalRead(WaterLevelPin)==HIGH)  // Replace with LOW when using NO (Normally Open)
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
// Alert Beeps

void AlertSound()
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
  lcd.print("Temp:   ");
  lcd.print(char(223));
  lcd.print("C    ");
  lcd.setCursor(0,1);
  lcd.print("Fan:    Pump:   ");
  DisplayTemp = CorrectedDisplayTemp-1;
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Main Menu / Settings

void EnterMenu(int x)
{
  int tmpInteger = TempThreshold;
  int tmpDigit1 = (tmpInteger/10), 
      tmpDigit2 = (tmpInteger % 10), 
      tmpCurrentDigit=0;
  boolean tmpBoolean;
  char KeyRead;
  boolean waitforkey = true;
  switch (x)
  {
    case 0:
      lcd.clear();
      lcd.print(MenuItems[0]);
      lcd.setCursor(0,1);
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
            tmpInteger = (tmpDigit1*10) + (tmpDigit2);         
            TempThreshold = tmpInteger;
            waitforkey = false;
            WriteToEEPROM(0,TempThreshold);            
            SavedMessage();            
            break;
          case 'D':
            NotSavedMessage();
            waitforkey = false;          
            break;
        }
        tmpInteger = (tmpDigit1*10) + (tmpDigit2);
        lcd.setCursor(13,1);
        if (tmpInteger < 10)
          lcd.print("0");
        lcd.print(tmpInteger);
      }
      StayInside = false;      
      break;
 
    case 1:
      lcd.clear();
      lcd.print(MenuItems[1]);
      lcd.setCursor(0,1);
      lcd.print("Buzzer:         ");
      tmpInteger=Buzzer;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpInteger -= 1;
            if (tmpInteger<0)
              tmpInteger=30;
            break;
          case '#':
            tmpInteger += 1;
            if (tmpInteger>30)
              tmpInteger=0;
            break;
          case 'A':
            Buzzer = tmpInteger;
            WriteToEEPROM(2,Buzzer);
            SavedMessage();
            waitforkey = false;
            break;
          case 'D':
            NotSavedMessage();            
            waitforkey = false;
            break;
        }
        lcd.setCursor(8,1);
        if (tmpInteger == 0)
          lcd.print("Disabled");
        else
        {
          if (tmpInteger<10)
            lcd.print("0");
          lcd.print(tmpInteger);
          lcd.print(" beeps");
        }
      }
      StayInside = false;      
      break;
      
    case 2:
      lcd.clear();
      lcd.print(MenuItems[2]);
      lcd.setCursor(0,1);
      lcd.print("Fan:            ");
      tmpBoolean=FanActive;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpBoolean = !(tmpBoolean);
            break;
          case '#':
            tmpBoolean = !(tmpBoolean);
            break;
          case 'A':
            FanActive = tmpBoolean;
            if (FanActive)
              WriteToEEPROM(4,1);
            else
              WriteToEEPROM(4,0);
            waitforkey = false;
            SavedMessage();
            break;
          case 'D':
            NotSavedMessage();
            waitforkey = false;
            break;
        }
        lcd.setCursor(5,1);
        if (tmpBoolean)
          lcd.print("Enabled ");
        else
          lcd.print("Disabled");
      }      
      StayInside = false;      
      break;
      
    case 3:
      lcd.clear();
      lcd.print(MenuItems[3]);
      lcd.setCursor(0,1);
      lcd.print("Pump:           ");
      tmpBoolean=PumpActive;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpBoolean = !(tmpBoolean);
            break;
          case '#':
            tmpBoolean = !(tmpBoolean);
            break;
          case 'A':
            PumpActive = tmpBoolean;
            if (PumpActive)
              WriteToEEPROM(6,1);
            else
              WriteToEEPROM(6,0);
            waitforkey = false;
            SavedMessage();
            break;
          case 'D':
            NotSavedMessage();
            waitforkey = false;
            break;
        }
        lcd.setCursor(6,1);
        if (tmpBoolean)
          lcd.print("Enabled ");
        else
          lcd.print("Disabled");
      }    
      StayInside = false;      
      break;
      
      
      
      
      
      
      
      
      
    case 4:
      lcd.clear();
      lcd.print(MenuItems[4]);
      lcd.setCursor(0,1);
      lcd.print("Overfill for   ");
      lcd.print(char(0x22));
      
      tmpInteger=OverFillDelay;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpInteger -= 1;
            if (tmpInteger<0)
              tmpInteger=30;
            break;
          case '#':
            tmpInteger += 1;
            if (tmpInteger>30)
              tmpInteger=0;
            break;
          case 'A':
            OverFillDelay = tmpInteger;
            WriteToEEPROM(12,OverFillDelay);
            SavedMessage();
            waitforkey = false;
            break;
          case 'D':
            NotSavedMessage();            
            waitforkey = false;
            break;
        }
        lcd.setCursor(13,1);
        if (tmpInteger<10)
          lcd.print("0");
        lcd.print(tmpInteger);
      }
      StayInside = false;      
      break;



    case 5:
      lcd.clear();
      lcd.print(MenuItems[5]);
      //         0123456789012345
      lcd.print("Temp Drop:   ");
      lcd.print(char(223));
      lcd.print("C ");
      tmpInteger=TemperatureDrop;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpInteger -= 1;
            if (tmpInteger<0)
              tmpInteger=10;
            break;
          case '#':
            tmpInteger += 1;
            if (tmpInteger>10)
              tmpInteger=0;
            break;
          case 'A':
            TemperatureDrop = tmpInteger;
            WriteToEEPROM(10,TemperatureDrop);
            SavedMessage();
            waitforkey = false;
            break;
          case 'D':
            NotSavedMessage();            
            waitforkey = false;
            break;
        }
        lcd.setCursor(10,1);
        if (tmpInteger<10)
          lcd.print("0");
        lcd.print(tmpInteger);
      }
      StayInside = false;      
      break;
      
    case 6:
      lcd.clear();
      lcd.print(MenuItems[6]);
      lcd.setCursor(0,1);
      lcd.print("Backlight:      ");
      tmpBoolean=Backlight;
      waitforkey = true;
      while (waitforkey)
      {
        KeyRead = keypad.getKey();
        switch (KeyRead)
        {
          case '*':
            tmpBoolean = !(tmpBoolean);
            break;
          case '#':
            tmpBoolean = !(tmpBoolean);
            break;
          case 'A':
            Backlight=tmpBoolean;
            if (Backlight)
            {
              WriteToEEPROM(8,1);
              lcd.backlight();
            }
            else
            {
              WriteToEEPROM(8,0);
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
        if (tmpBoolean)
          lcd.print("ON ");
        else
          lcd.print("OFF");
      }    
      StayInside = false;      
      break;

    case 7:
      lcd.clear();
      lcd.print(MenuItems[7]);
      lcd.setCursor(0,1);
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
            lcd.print("SETTINGS CLEARED");
            WriteToEEPROM(0,30);
            WriteToEEPROM(2,5);
            WriteToEEPROM(4,1);
            WriteToEEPROM(6,1);
            WriteToEEPROM(8,1);
            WriteToEEPROM(10,0);
            WriteToEEPROM(12,0);
            delay(1500);            
            Beep(3);
            waitforkey = false;
            break;
          case 'D':
            lcd.setCursor(0,1);
            lcd.print("   CANCELED!    ");
            Beep(2);
            delay(1500);
            waitforkey = false;
            break;
        }
      }
      StayInside = false;      
      break;
        
    case 8:
      lcd.clear();
      lcd.print("    (c) 2013    ");
      lcd.setCursor(0,1);
      lcd.print("Antonis Maglaras");
      WaitForKeypress();
      lcd.clear();
      lcd.print("    Version     ");
      lcd.setCursor(0,1);
      lcd.print(Version);
      WaitForKeypress();
      lcd.clear();
      lcd.print("Free Memory");
      lcd.setCursor(0,1);
      lcd.print(freeMemory());
      lcd.print(" bytes");
      WaitForKeypress();
      StayInside = false;    
      Beep(2);  
      break; 
  }  
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Write to EEPROM

void WriteToEEPROM(int addr, int number)
{
  EEPROM.write(addr,(number / 256));
  EEPROM.write(addr+1,(number % 256));
}  



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Read from EEPROM

int ReadFromEEPROM(int addr)
{
  return (EEPROM.read(addr)*256)+EEPROM.read(addr+1);
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



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Wait for keypress

void WaitForKeypress()
{
  while (keypad.getKey() == NO_KEY) {};  
}
