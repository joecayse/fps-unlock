/* 
	FPS_Enroll.ino - Library example for controlling the GT-511C1R Finger Print Scanner (FPS)
	Created by Josh Hawley, July 23rd 2013
	Licensed for non-commercial use, must include this license message
	basically, Feel free to hack away at it, but just give me credit for my work =)
	TLDR; Wil Wheaton's Law
*/

/* Modifications to Josh Hawley's sketches for FPS Enrollment and ID made by Joe Cayse for the purposes of
   SmartFrame prototype. Commercial production will utilize a different
   FPS and microcontroller. SmartFrame prototype is not for sale. 
*/

#include "FPS_GT511C1R.h"
#include "SoftwareSerial.h"
#include <avr/wdt.h>

// Hardware setup - FPS connected to:
//	  digital pin 4(arduino rx, fps tx)
//	  digital pin 5(arduino tx - 560ohm resistor fps tx - 1000ohm resistor - ground)
//		this brings the 5v tx line down to about 3.2v so we dont fry our fps

FPS_GT511C1R fps(4, 5);
int lock_pin = 2;   //lock pin
int off_pin = 3;     //output to turn off circuit
int error_LED = 6;   //red, indicates enrollment error
int yellow_LED3 = 7; //indicates first print captured
int yellow_LED2 = 8;  //indicates second print captured
int yellow_LED1 = 9;  //indicates third print captured
int ready_LED = 10;  //green, indicates ready to press finger
int buzzer = 11;    //beeps to indicate successfull enroll
int delete_PB = 12; //user holds to delete all fingerprint profiles
int mode_switch = 13;  //toggle between program and normal modes

void setup()
{
  //initializing the watchdog timer. when the wdt times out, and interrupt will send pin 3 high to turn off the circuit. 
 watchdogSetup();     
 delay(100);
 Serial.begin(9600);  //turn on serial comm
  delay(100);
  fps.Open();   //start comm with FPS
  fps.SetLED(true); //turn on LED
  for(int i=6; i<=13; i++)
  {
  pinMode(i, OUTPUT);
  }
  pinMode(delete_PB, INPUT);
  pinMode(mode_switch, INPUT); 
  pinMode(off_pin, OUTPUT);   
  digitalWrite(off_pin, LOW); //ensures this pin is not floating
  pinMode(lock_pin, OUTPUT);
}

void watchdogSetup(void)
{
 cli();
 wdt_reset();
/*
 WDTCSR configuration:
 bit 6 WDIE = 1: Interrupt Enable
 bit 5 WDP3 = 1 :For 8000ms Time-out
 bit 4 WDCE = 0 not in configure mode
 bit 3 WDE = 1 :Reset Enable
 bit 2 WDP2 = 0 :For 8000ms Time-out
 bit 1 WDP1 = 1 :For 8000ms Time-out
 bit 0 WDP0 = 0 :For 1000ms Time-out
*/
// Enter Watchdog Configuration mode:
WDTCSR |= B00011000; // sets WDCE and WDE to 1 for setup mode
// Set Watchdog settings:
WDTCSR = B01100001; // sets to interrupt mode, no reset, and 8000ms timeout
sei();
}

void loop()
{
  for(int i=6; i<=10; i++)  //sets LED pins 6-10 LOW so they do not remain on after changing modes
  digitalWrite(i, LOW);
  while (digitalRead(mode_switch) == HIGH)
  {
    enrollID(); //runs "program mode" when toggled HIGH
  }
  
  IDprint();  //runs in normal operating mode when toggled LOW
}


void IDprint()
{
// Identify fingerprint test
  if (fps.IsPressFinger())
  { 
    wdt_reset();
    fps.CaptureFinger(false);
    int id = fps.Identify1_N();
    if (id <200)
    {
      digitalWrite(lock_pin, HIGH);
      delay (1000);
      digitalWrite(lock_pin, LOW);
      wdt_reset();
    }
    else
    {
      delay (100);
    }
  }
  else
  {
   delay(100);
  }
  delay(100);
}

void enrollID(){
// find open enroll id
  int enrollcount = fps.GetEnrollCount();
  if (enrollcount == 20){             //prompt the user if the enrollments are full (20 max)
    digitalWrite(error_LED, HIGH);
    delay(250);
    digitalWrite(error_LED, LOW);
    delay(250);
  }
    
  int enrollid = 0;
  bool usedid = true;
  while (usedid == true)
  {
    usedid = fps.CheckEnrolled(enrollid);
    if (usedid==true) enrollid++;
  }
  fps.EnrollStart(enrollid);

  // enroll
  digitalWrite(ready_LED, HIGH);        //prompts user to press finger
  checkDelete_PB();
  bool bret = fps.CaptureFinger(true);
  if (bret != false)      
  {
    wdt_reset();
    digitalWrite(yellow_LED1, HIGH);      //first scan received
    digitalWrite(ready_LED, LOW);
    fps.Enroll1(); 
    while(fps.IsPressFinger() == true) delay(100);
    digitalWrite(ready_LED, HIGH);        //ready to scan again
    while(fps.IsPressFinger() == false) delay(100);
    bret = fps.CaptureFinger(true);
    if (bret != false)
    {
      wdt_reset();
      digitalWrite(yellow_LED2, HIGH);    //second scan received
      digitalWrite(ready_LED, LOW);
      fps.Enroll2();
      while(fps.IsPressFinger() == true) delay(100);
      digitalWrite(ready_LED, HIGH);      //ready to scan again
      while(fps.IsPressFinger() == false) delay(100);
      bret = fps.CaptureFinger(true);
      if (bret != false)
      {
        wdt_reset();
        digitalWrite(yellow_LED3, HIGH);  //3rd scan received
        digitalWrite(ready_LED, LOW);
        fps.Enroll3();
        delay(100);
        int newenrollcount = fps.GetEnrollCount();
        if (newenrollcount > enrollcount)
        {
          tone (buzzer, 880, 100);
          delay (100);
          tone (buzzer, 1318, 100);
          delay (100);
          blinkLEDs();
          blinkLEDs();
          blinkLEDs();
          wdt_reset();
        }
          else
          {
          error();
          }
      }
      else error();
    }
    else error();
  }
  else delay(100);
}



void checkDelete_PB()     //user must press and hold delete_PB for three seconds to delete all enrollments
{
  int loopcount=0;
  while (digitalRead(delete_PB) == HIGH)  
    {
    wdt_reset();
    delay(100);
    loopcount++;
    if (loopcount==30)
      {
        fps.DeleteAll();
        blinkLEDs();
        blinkLEDs();
        blinkLEDs();
        wdt_reset();
      }
    }
}


void blinkLEDs()
{
  for(int i=7; i<=10; i++){ //ready and yellow 1-3 LED's
  digitalWrite(i, HIGH);}
  delay(100);
  for(int i=7; i<=10; i++){
  digitalWrite(i, LOW);}
  delay(100);
}

void error()
{
  wdt_reset();
  tone (buzzer, 1318, 100);
  delay(100);
  tone (buzzer, 880, 100);
  digitalWrite(ready_LED, LOW); //turn all LEDS OFF regardless of state
  digitalWrite(yellow_LED1, LOW);
  digitalWrite(yellow_LED2, LOW);
  digitalWrite(yellow_LED3, LOW);
  digitalWrite(error_LED, HIGH);  //flash the ERROR LED 3 times
  delay(100);
  digitalWrite(error_LED, LOW);
  delay(100);
  digitalWrite(error_LED, HIGH);
  delay(100);
  digitalWrite(error_LED, LOW);
  delay(100);
  digitalWrite(error_LED, HIGH);
  delay(100);
  digitalWrite(error_LED, LOW);
  delay(100);
}

ISR(WDT_vect) // Watchdog timer interrupt.
{
digitalWrite(off_pin, HIGH);
delay(100);
digitalWrite(off_pin, LOW);
delay(100);
digitalWrite(off_pin, HIGH);
delay(100);
digitalWrite(off_pin, LOW);
delay(100);
digitalWrite(off_pin, HIGH);
delay(100);
digitalWrite(off_pin, LOW);
delay(100);
digitalWrite(off_pin, HIGH);
delay(100);
digitalWrite(off_pin, LOW);
wdt_reset();
}

