#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>



LiquidCrystal_I2C lcd(0x20,16,2);  // set the LCD address to 0x20 for a 16 chars and 2 line display

#define TemperaturePin   A0
#define WaterLevelPin    A1
#define LEDPin           13
#define BuzzerPin        11
#define FanPin            9
#define PumpPin          10


const byte rows = 4; //four rows
const byte cols = 4; //three columns
char keys[rows][cols] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'#','0','*','D'}
};
//                    0123456789012345
char* MenuItems[5] = { "Set Temperature ",      // 0
                       "Set Buzzer      ",      // 1
                       "Set Fan         ",      // 2
                       "Set Pump        ",      // 3
                       "Exit            " };    // 4


// ---- Keypad pins: 2, 3, 4, 5, 6, 7, 8, 9 ----

byte rowPins[rows] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[cols] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );


float CurrentTemp = 0;
volatile int TempThreshold = 30;
boolean FanStatus = false, PumpStatus = false, WaterLevel = false, Buzzer = false, StayInside = true, FanEnabled = false, PumpEnabled = false;


#define    Debug    true


// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Setup / Initialize Procedure

void setup()
{
  pinMode(PumpPin, INPUT_PULLUP);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(LEDPin, OUTPUT);
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  InitializeScreen();
  TempThreshold = ReadFromEEPROM(0);
  Buzzer = ReadFromEEPROM(2);
  FanStatus = ReadFromEEPROM(4);
  PumpStatus = ReadFromEEPROM(6);
  Serial.begin(9600);
  Serial.println("Arduarium Started!");
  Serial.print("Buzzer: ");
  Serial.println(Buzzer);
  Serial.print("Fan: ");
  Serial.println(FanStatus);
  Serial.println("");
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------------
// Main Procedure

void loop()
{
  char key = keypad.getKey();
// ---
  if (key != NO_KEY)
  {
    StayInside = true;
    lcd.clear();
    lcd.print("Menu");
    lcd.setCursor(0,1);
    //         0123456789012345
    lcd.print("Set Temperature ");
    
    if (key == 'A')                        // ENTER MENU
    {
      int MenuSelection = 0;
      while (StayInside)
      {
        lcd.setCursor(0,1);
        lcd.print(MenuItems[MenuSelection]);
        switch (key)
        {
          case '*':                        // LEFT
            MenuSelection -= 1;
            if (MenuSelection < 0)
              MenuSelection = 4;
            break;
          case '#':                        // RIGHT
            MenuSelection += 1;
            if (MenuSelection > 4)
              MenuSelection = 0;
            break;
          case 'A':                        // ENTER
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
  int tmpDigit1 = 0, tmpDigit2 = 0, tmpCurrentDigit=0;
  boolean tmpBuzzer = Buzzer;
  boolean tmpPumpEnabled = PumpEnabled;
  boolean tmpFanEnabled = FanEnabled;
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
            TempThreshold = tmpTempThreshold;
            waitforkey = false;
            SaveToEEPROM(0,TempThreshold);
            break;
          case 'D':
            waitforkey = false;          
            break;
        }
        tmpTempThreshold = (tmpDigit1*10) + (tmpDigit2);
        lcd.setCursor(13,1);
        if (tmpTempThreshold < 10)
          lcd.print("0");
        lcd.print(tmpTempThreshold);
      }
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
            SaveToEEPROM(2,Buzzer);
            waitforkey = false;
            break;
          case 'D':
            waitforkey = false;
            break;
        }
        lcd.setCursor(8,1);
        if (Buzzer)
          lcd.print("ON ");
        else
          lcd.print("OFF");
      }
      break;
      
    case 2:
      lcd.clear();
      lcd.print(MenuItems[2]);
      lcd.setCursor(0,1);
      //         0123456789012345
      lcd.print("Pump:           ");
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
            SaveToEEPROM(6,FanEnabled);
            waitforkey = false;
            break;
          case 'D':
            waitforkey = false;
            break;
        }
        lcd.setCursor(8,1);
        if (tmpFanEnabled)
          lcd.print("ON ");
        else
          lcd.print("OFF");
      }      
      break;
      
      
      
    case 3:
      lcd.clear();
      lcd.print(MenuItems[2]);
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
            SaveToEEPROM(6,PumpEnabled);
            waitforkey = false;
            break;
          case 'D':
            waitforkey = false;
            break;
        }
        lcd.setCursor(8,1);
        if (tmpPumpEnabled)
          lcd.print("ON ");
        else
          lcd.print("OFF");
      }    
      break;
    case 4: 
      StayInside = false;      
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
