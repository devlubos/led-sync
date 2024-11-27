/*
 * PINOUT
 * ARDUINO NANO DESCRIPTION
 * 3  - Interrupt pin for Gesture Sensor
 * 9  - PWM Red color
 * 10 - PWM Green color
 * 11 - PWM Blue color
 * 18 - SDA pin for Gesture Sensor
 * 19 - SCL pin for Gesture Sensor
 * 
 * Additional pins:
 * 5V (pin 27) wire to Gesture Sensor & to Power Supply
 * GND wire to Gesture Sensor & Power Supply
 */

 /*APDS definitions */
#include <Wire.h>
#include "SparkFun_APDS9960.h"
#include <EEPROM.h>

// Pins
#define APDS9960_INT 3 // Needs to be an interrupt pin
#define EEPROM_color 0
#define EEPROM_mode 2

// Global Variables
SparkFun_APDS9960 apds = SparkFun_APDS9960();
int isr_flag = 0;
/*ADPS def ends*/

/*pwm outputs*/
const int red = 9;
const int green = 10;
const int blue = 11;
int sign = 1;

//needs to be better implemented
int breatheTime = 6.0; //time in s for a full cycle

//colors
byte red_color, green_color, blue_color;
byte modeFromEEPROM;

float colors_pallete [13]{
  1, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330, 359
  };
byte color_position;

//paramteters
float hue, saturation, value;

enum mode { SOLID, ALIVE, DISCO, POWERUPCOLOR, COLOROFF };
mode displayMode;
mode lastMode;

String serialString;

void setup() {
  Serial.begin(9600);
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(blue, OUTPUT);
  value = 0.0;
  saturation = 100.0;
  hue = 120.0;

  modeFromEEPROM = EEPROM.read(EEPROM_mode);
  switch(modeFromEEPROM){
    case 0 :
      lastMode = SOLID;
      break;
    case 1 :
      lastMode = ALIVE; 
      break;
    case 2 :
      lastMode = DISCO;
      break;
    
    default:
      break;
  }
  displayMode = POWERUPCOLOR;
  
  color_position = EEPROM.read(EEPROM_color);
  hue = colors_pallete[color_position];

  /*ADPS setup*/
  //Set interrupt pin as input
  pinMode(APDS9960_INT, INPUT);

   // Initialize interrupt service routine
  attachInterrupt(digitalPinToInterrupt(APDS9960_INT), interruptRoutine, FALLING);

  // Initialize APDS-9960 (configure I2C and initial values)
  if ( apds.init() ) {
    Serial.println(F("APDS-9960 initialization complete"));
  } else {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }
  
  // Start running the APDS-9960 gesture sensor engine
  if ( apds.enableGestureSensor(true) ) {
    Serial.println(F("Gesture sensor is now running"));
  } else {
    Serial.println(F("Something went wrong during gesture sensor init!"));
  }
  /* ADPS ends*/
}

// the loop function runs over and over again forever
void loop() {

  /*** serial communication handling ***/
  if (Serial.available() > 0) {
    serialString = Serial.readString();
    serialString.trim();
    serialString.toUpperCase();

    if(serialString == "SOLID") {
        displayMode = SOLID;
        
        Serial.println("SOLID mode enabled.");
    }
    if(serialString == "DISCO") {
        displayMode = DISCO;
        Serial.println("DISCO mode enabled.");
    }
    if(serialString == "ALIVE"){
        displayMode = ALIVE;
        Serial.println("ALIVE mode enabled.");
    }    
    if(serialString.startsWith("HUE=")) {
      hueAdjust(serialString.substring(4).toFloat());
      Serial.print("HUE setted to: ");
      Serial.println(hue);
    }
    if(serialString.startsWith("VALUE=")) {
      value = serialString.substring(6).toFloat();
      Serial.print("VALUE setted to: ");
      Serial.println(value);
    }

   if(serialString.startsWith("SAT=")) {
      saturation = serialString.substring(4).toFloat();
      Serial.print("SATURATION setted to: ");
      Serial.println(saturation);
   }
   if(serialString.startsWith("SET=")) {            //SET=001,001,001      
      hueAdjust(serialString.substring(4,7).toFloat());      
      saturation=serialString.substring(8,11).toFloat();
      value=serialString.substring(12).toFloat();
      Serial.print("New settings:");
      Serial.print(hue);
      Serial.print(",");
      Serial.print(saturation);
      Serial.print(",");
      Serial.println(value);      
   }
  }

  /*** light system management ***/
  switch(displayMode) {
    case SOLID :
      break;
    case DISCO :
      disco();
      break;
    case ALIVE :
      breathing();
      break;
    case POWERUPCOLOR :
      powerUpColor();
      break;
    case COLOROFF :
      value = 0.0;
      break;  
  }  
  setColor();

  /*** ADPS handling ***/
  if( isr_flag == 1 ) {
    
    detachInterrupt(digitalPinToInterrupt(APDS9960_INT));
    handleGesture();
    isr_flag = 0;
    attachInterrupt(digitalPinToInterrupt(APDS9960_INT), interruptRoutine, FALLING);
  }
  delay(50);
}

void breathing(){
  value += (10.0/breatheTime)*sign;
    if (value > 100) 
  {
    value = 100;
    sign = -1;
  }
  if (value < 0) 
  {
    value = 0;
    sign = 1;
  }
}

void powerUpColor(){
  value += 2*sign;
  if (value >= 100) 
  {
    value = 100;
    displayMode = lastMode;
  }
}

void disco(){
  value = 100;
  saturation = 100;

  hue += 2*sign;
  if (hue >= 360) 
  {
    hue = 359;
    sign = -1;
  }
  if (hue <= 0) 
  {
    hue = 0;
    sign = 1;
  }
}

void hueAdjust(float newHue){
  Serial.println("Starting to adjust the color");
  if((hue>355)&&(newHue<5)){
    hue=0;
    color_position =  0;
  }
  if((hue<5)&&(newHue>355)){
    hue=355;
    color_position = 12;
  }
  
  float differenece = newHue-hue;
  
  
  
  for (int i =0; i<10; i++){
    hue += (differenece/10);
    if (hue>359) 
    {
      hue = 359;
    }
    if (hue<0) 
    {
      hue = 0;
    }    
    setColor();
    
    delay(50);
  }
  
}

/******/

void setColor(){
  HSV_to_RGB(hue,saturation,value);
  analogWrite(red,red_color);
  analogWrite(green,green_color);
  analogWrite(blue,blue_color);
}

void HSV_to_RGB(float h, float s, float v)
{
  int i;
  float f,p,q,t;
  
  h = max(0.0, min(360.0, h));
  s = max(0.0, min(100.0, s));
  v = max(0.0, min(100.0, v));
  
  s /= 100;
  v /= 100;
  
  if(s == 0) {
    // Achromatic (grey)
    red_color = green_color = blue_color = round(v*255);
    return;
  }

  h /= 60; // sector 0 to 5
  i = floor(h);
  f = h - i; // factorial part of h
  p = v * (1 - s);
  q = v * (1 - s * f);
  t = v * (1 - s * (1 - f));
  switch(i) {
    case 0:
      red_color = round(255*v);
      green_color = round(255*t);
      blue_color = round(255*p);
      break;
    case 1:
      red_color = round(255*q);
      green_color = round(255*v);
      blue_color = round(255*p);
      break;
    case 2:
      red_color = round(255*p);
      green_color = round(255*v);
      blue_color = round(255*t);
      break;
    case 3:
      red_color = round(255*p);
      green_color = round(255*q);
      blue_color = round(255*v);
      break;
    case 4:
      red_color = round(255*t);
      green_color = round(255*p);
      blue_color = round(255*v);
      break;
    default: // case 5:
      red_color = round(255*v);
      green_color = round(255*p);
      blue_color = round(255*q);
    }
}

void interruptRoutine() {
  isr_flag = 1;
}

void handleGesture() {
    if ( apds.isGestureAvailable() ) {
    switch ( apds.readGesture() ) {
      case DIR_UP:
        Serial.println("UP");
       
        if(displayMode != COLOROFF){
          lastMode = displayMode;
          displayMode = COLOROFF;            
        }
        break;
        
      case DIR_DOWN:     
        Serial.println("DOWN");        

        if(displayMode == COLOROFF)
        {
          displayMode = POWERUPCOLOR;
        }
        else {
          if(displayMode == SOLID)
          {
            displayMode = ALIVE;  
            EEPROM.write(EEPROM_mode, 1);
          }
          else {
            displayMode = SOLID;
            EEPROM.write(EEPROM_mode, 0);
          }      
        }           
        break;
        
      case DIR_LEFT:
        Serial.println("LEFT");
        if(color_position == 0) {
         color_position = 12; 
        }
        else{
          color_position -= 1;  
        }        
        hueAdjust(colors_pallete[color_position]);

        EEPROM.write(EEPROM_color, color_position);
        
        break;
      case DIR_RIGHT:
        Serial.println("RIGHT");
        if(color_position == 12) {
         color_position = 0; 
        }
        else{
          color_position += 1;  
        } 
        hueAdjust(colors_pallete[color_position]);

        EEPROM.write(EEPROM_color, color_position);
        
        break;
      case DIR_NEAR:
        Serial.println("NEAR");
        break;
      case DIR_FAR:
        Serial.println("FAR");
        break;
      default:
        Serial.println("NONE");
    }
  }
}
