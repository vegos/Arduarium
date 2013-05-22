Arduarium
=========

Aquarium Controller (work in progress)
Created for the Aquarium of my friend, GeGeor :)
It controls a fan for cooling down the aquarium and a water pump for filling 
water.



Hardware:
---------

- ATmega328
  Capacitors, resistors, push switch, LED, 7805 Voltage Regulator etc, as it 
  created on a general-purpose PCB.
- LCD display (16x2) & I2C Driver (for using 2 wires)
- Two (2) Relay Module for powering on/off the water pump and the fan.
  In the future, it will be expanded with more functions.
- 4x4 Matrix Keyboard. For easier access to the menu system/settings.
- Water Sensor (actually, a water-triggered button) for metering the water 
  level.
- LM35 Temperature Sensor (and waterproffing it using silicone).



Photos: 
-------

https://picasaweb.google.com/104656736936976952947/Arduarium



Video:
------

http://www.youtube.com/watch?v=CcVkWe7an_k

(Temperature sensor is not installed --  that's the reasing for the weird 
results)



Wiring:
-------

- I guess you can find out the wiring following the PIN assignments on the 
  code.



Usage:
------

The Arduarium is menu-driven and is optimized to work with a 4x4 Matrix
Keypad.
In standby screen, it shows the current water temperature and the status of
the fan and the water pump.
You can enable manually the fan, by pressing the star (*) key. Pressing it 
again will stop the fan.
By pressing the dash (#), it will start the water pump. Again to stop.
To enter the menu, press the "A" key. This key is also used as Enter (to
confirm a data entry). You can move to the menus by pressing the * for
left/previous or # for right/next menu. To cancel/return you can press the
"D" key.
In standby mode, pressing the "B" button, will change the backlight (on or
off). It's not saved in memory.

All settings are saved in EEPROM for power-loss. They are recalled when
powered on.

The temperature is displayed in 2 digits plus one decimal. The temperature
threshold is an integer. When comparing, the Arduarium checks for the
absolute values (e.g. 29.9 is not equal to 30, but 30.1 and 30.9 is equal 
to 30).

For water level sensor, you can use any NC (normally-closed) switch.



To be continued...
