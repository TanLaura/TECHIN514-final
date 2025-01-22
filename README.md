# TECHIN514-final

## Daily Habit Tracker
The device tracks the number of task completed (chips in the collector/sensing device) and show the progress with the stepper gauge needle. The LED light can be on the scale of red to green based on percentage of completion. It relies on the load cell and amplifier to calculate the number of chips of completed tasks and completion rate based on the set goal of the day. The stepper motor gauge and LED will display the progress. The display device also have the option to set the goals.

![Overview Graph](Images/Overall_device.jpeg)


## Sensor device
The sensor device looks like a bowl overall. There is a load cell and amplifier that read the weight and translate it to voltage. It calculates the percentage of task completion and send the data to display device for further information display.
![Sensing Device](Images/Sensing_device.jpeg)

## Display Device
The outer shape can be varied based on the final PCB shape and size. The diplay has a stepper motor gauge and LED light both shows the completion rate of the task in different forms. It has a reset button and button to adjust the goal of the day.
![Display Device](Images/Diplay_device.jpeg)

## Device Communicatioin 

![System architecture](Images/System_architecture.jpeg)


### Component Lists
- [Seeduino ESP32C3](Datasheets/Seeduino ESP32c3.pdf)
- [Load cell](https://www.adafruit.com/product/4541#description): The load cell seems to only have a webpage with product details instead of a datasheet.
- [HX711](Datasheets/adafruit-hx711-24-bit-adc.pdf)
- [Stepper Motor Gauage](Datasheets/stepd-01-data-sheet-1143075.pdf)
- [LED WS2812](Datasheets/WS2812.pdf)

### Potential Component to Add after Discussion 
- A small display screen
- The reset button (and its form)
- A charging circuit?
- Sleep mode?

These are all questions I want to go over with the instruction team to finalize the component list and structure. 