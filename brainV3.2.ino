
//Libraries needed are:
// 1. Adafruit_SSD1306, in Teensy folder if Teensy has been installed on computer. Note: the h file needs to be commented and uncommented to select the correct oled screen resolution 128 x 64
// 2. SdFat-master (in local documents folder)
// 3. Adafruit_MCP9808_Library-master (in local documents folder)
//LIBRARIES
  #include <Adafruit_MCP9808.h>//library for temp sensor900
  #include <TimeLib.h>//Time library
  #include <Wire.h>// I2C library
  #include <Snooze.h>// Sleep library for Teensy
  #include <Adafruit_GFX.h>// OLED library
  #include <Adafruit_SSD1306.h>// OLED library
  #include <IRremote.h>// IR library
  #include "SdFat.h"//SD cart library
  #include <SPI.h>// Serial library
  #include <ADC.h>// Analog to digital converter library
//DEFINITIONS
  //Version
    char sasVersion [] = "V3.2";//In version V3.2 sleep code and logging code have been elevated to the main loop, the voltage reading in the pump duration equation has been removed, and the size of the resistors on the physical board for the voltage divider have been increased from 3.9K and 1K to 390K and 100K respectively to reduce latent current draw.
  //Snooze
    SnoozeAlarm alarm;
    SnoozeDigital digital;
    SnoozeBlock config_teensy35(digital, alarm);
    long sleepSecTime;    
    int hrToSleep;// Number of hours to sleep
    int minToSleep;// Number of remaining minutes to sleep
    int secToSleep;// Number of remaining seconds to sleep
  //OLED
    #define OLED_RESET 8// OLED reset
    Adafruit_SSD1306 display(OLED_RESET);
    int oledPowerPin = 39;// Pin that powers the OLED
  //Temperature sensor setup
    Adafruit_MCP9808 tempSensor = Adafruit_MCP9808();//makes tempSensor object
    int tempPowerPin = 38;// Pin that powers the temperature sensor
    float temp;// The temperature
  //Voltage Read
    int voltageReadPin = A15;// Pin that reads the voltage
    ADC *adc = new ADC();// Creates adc object
    float voltage = 0;// The voltage of the battery backs
    float rawVoltage = 0;// The raw voltage read across the voltage divider
  //IR
    int irPowerPin = 35;// Pin that powers the IR sensor
    IRrecv irrecv(7);// Creates an IR receiving object irrecv and sets the pin that receives the IR signal
    decode_results results;// Creates a variable of type decode_results
    #define LEFTIR 16584943// IR signal read by sensor that translates to LEFTIR
    #define RIGHTIR 16601263// IR signal read by sensor that translates to RIGHTIR
    #define UPIR 16621663// IR signal read by sensor that translates to UPIR
    #define DOWNIR 16625743// IR signal read by sensor that translates to DOWNIR
    #define ENTERIR 16617583// IR signal read by sensor that translates to ENTERIR
  //Reed switch
    int REED_INTERRUPT_PIN = 6;// Pin that reed switch is attached to
  //Pumps
    int pumpAPin =37;// Pin to signal FET to drive motor 1
    int pumpBPin =36;// Pin to signal FET to drive motor 2
    int pumpACalibration = 23;// Calibration factor for pump A, number of mL per second of pump operation at full power * 10 to make integer
    int pumpBCalibration = 23;// Calibration factor for pump B, number of mL per second of pump operation at full power* 10 to make integer
  // Menu
    uint8_t menu = 0;// An index value to establish which navigation menu is currently selected
    uint8_t pos = 0;// An index value to establish which navigation position (within a menu) is currently selected
  //SD card
    SdFatSdio sd;
    File sampleParam;// Name of sample parameter file saved on SD card
    File dataLog;// Name of data logging file saved on SD card
    #define SAMPLE_PARAM_ROW 3// Number of rows in sampleParam.txt file
    #define SAMPLE_PARAM_COL 5// Number of columns in sampleParam.txt file
    int sampleParamArray[SAMPLE_PARAM_ROW][SAMPLE_PARAM_COL];// Create an sampleParamArray with SAMPLE_PARAM_ROW rows and SAMPLE_PARAM_COL columns
    int blankSdParam = 1;
  //RTC
    time_t nowSecTime;// Create a time variable called nowSecTime, representing the time now in seconds since 1970
    time_t updateSecTime;// Create a time variable called updateSecTime, representing the time to be set to the RTC in seconds since 1970
    time_t aSecTime;// Create a time variable called aSecTime, representing the time to fire pump A in seconds since 1970
    time_t bSecTime;// Create a time variable called bSecTime, representing the time to fire pump B in seconds since 1970
    int nowHr; int nowMin; int nowSec; int nowDay; int nowMon;  int nowYr;// create integers for the time now in hours, minutes, seconds, days, months, years
    int aHr;int aMin;int aSec;int aDay;int aMon;int aYr;// create integers for the time to fire A in hours, minutes, seconds, days, months, years
    int bHr;int bMin;int bSec;int bDay;int bMon;int bYr;// create integers for the time to fire B in hours, minutes, seconds, days, months, years
    long aEndTime;
    long bEndTime;
    int aAlarmFlag = 0;
    int bAlarmFlag = 0;
  //Sample Parameters. For sampleMode 0 is daily, 1 is once
    int sampleMode;// An integer to hold what sampling mode is initiated, 0 for daily, 1 for once
    int sampleVolume;// An integer for the number of mL of sample that will be collected
    int sampleVolumeChangeMl = 10;// The number of mL to change sample volume when navigating up or down

//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
//<----------------------------------------------------------------------SETUP----------------------------------------------------------------------------------->
//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
void setup(){// Run setup code on initial power up
  //Communication setup     
    Wire.begin();// NOT SURE I NEED THIS
    Serial.begin(9600);// Initiate the serial connection, used for debugging
    delay (500);// Pause for a half second
  //Temperature sensor setup
    pinMode(tempPowerPin, OUTPUT);// Set the temperature power pin to an output, because this is how we are powering the temperature sensor unit 
    digitalWrite(tempPowerPin, HIGH); delay(1000);// Turn on the temperature sensor by writing the temp power pin to high. Delay 0.1 seconds
    if (!tempSensor.begin()) {Serial.println("temperature sensor not communicating");}// If the temperature sensor doesn't start up. Print that it is not communicating to the serial connection
  //Voltage read setup
    pinMode(voltageReadPin, INPUT);// Set the voltage read pin as an input, where we will be reading the voltage of the battery pack
    adc->setAveraging(32, ADC_0); // set number of averages read in the analog to digital converter we will use to read the voltage of the battery pack
    adc->setResolution(16, ADC_0); // set bits of resolution of the analaog to digital converter we will use to read the voltage of the battery pack
    adc->setSamplingSpeed(ADC_SAMPLING_SPEED::LOW_SPEED, ADC_0); // set the sampling speed of the analaog to digital converter to low
  //RTC setup          
    setSyncProvider(getTeensy3Time);// Sets the time using the getTeensy3Time function
  //Snooze
    pinMode(REED_INTERRUPT_PIN, INPUT_PULLUP);
    delay(200);
    digital.pinMode(REED_INTERRUPT_PIN, INPUT_PULLUP, RISING);// Sets the pin the reed switch is connected to as an interrupt pin that can wake the teensy up from sleep
    alarm.setRtcTimer(0, 1, 0);// Set the alarm to go off at hour, min, sec
  //OLED Setup
    pinMode(oledPowerPin, OUTPUT);// Set the oledPowerPin to an output because this is the pin that we power the OLED from
    digitalWrite(oledPowerPin, HIGH);// Set the oledPowerPin to high to turn on the OLED
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);// Begin and initialize the OLED display
    display.clearDisplay();// Clear the OLED display
  //Infrared Setup
    pinMode(irPowerPin, OUTPUT);// Set the irPowerPin to an output because this is the pin that we power the IR sensor from
    digitalWrite(irPowerPin, HIGH);// Set the irPowerPin to high to turn on the IR sensor   
    irrecv.enableIRIn();// Starts the IR receiving object irrecv
  //Pump Setup
    pinMode(pumpAPin, OUTPUT);// Set the pumpAPin to an output because this is the pin that we will use to power pump A
    pinMode(pumpBPin, OUTPUT);// Set the pumpAPin to an output because this is the pin that we will use to power pump A 
  //SD Setup
    if (!sd.begin()) { display.setTextColor(WHITE,BLACK);  display.setTextSize(1);  display.setCursor(2,8);  display.println("MicroSD not");display.setCursor(5,16);  display.println("detected!");display.display();sd.initErrorHalt("SdFatSdio begin() failed");}// If the SD card fails to communicate, display that the card is not detected to the serial debugging
    readSampleParamArray();// Call the function that reads the sample parameters off of the SD card
    calculateInitialAlarmSecondTime();    
}

//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
//<------------------------------------------------------------------MAIN LOOP----------------------------------------------------------------------------------->
//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
void loop(){
  switch(menu){//What menu is selected?
    case 0: getNow(); statusMenuNavigation(); statusMenuDisplay(); break;//Loop the Status Menu navigation and display
    case 1: settingsMenuNavigation(); settingsMenuDisplay(); break;//Loop the Settings Menu navigation and display 
    case 2: pumpAMenuNavigation(); pumpAMenuDisplay(); break;//Loop the Pump A Menu navigation and display 
    case 3: pumpBMenuNavigation(); pumpBMenuDisplay(); break;//Loop the Pump B Menu navigation and display 
    case 4: getNow(); initiateMenuNavigation(); initiateMenuDisplay(); break;//Loop the Initiate Menu navigation and display 
    case 5: timeSetMenuNavigation(); timeSetMenuDisplay(); break;//Loop the Time Set Menu navigation and display 
    case 6: pumpCalibrationMenuNavigation(); pumpCalibrationMenuDisplay(); break;//Loop the Pump Calibration Menu navigation and display
    case 7: pumpCalibrationRunMenuNavigation(); pumpCalibrationRunMenuDisplay(); break;//Loop the Pump Calibration Menu navigation and display
    case 8: versionMenuNavigation(); versionMenuDisplay(); break;//Loop the Version Menu navigation and display
    case 9: getNow();
      //SAS SHOULD GO TO SLEEP
      if (((aAlarmFlag == 0)&&(bAlarmFlag == 3)&&(aSecTime>nowSecTime+60))||((aAlarmFlag == 3)&&(bAlarmFlag == 0)&&(bSecTime>nowSecTime+60))||((aAlarmFlag == 0)&&(bAlarmFlag == 0)&&(aSecTime>nowSecTime+60)&&(bSecTime>nowSecTime+60))||((aAlarmFlag == 3)&&(bAlarmFlag == 3))){
        //Determine sleepSecTime, the number of seconds for the SAS to sleep until next sampling event
        if ((aAlarmFlag != 3)&&(bAlarmFlag != 3)&&(aSecTime < bSecTime)){sleepSecTime = aSecTime - nowSecTime;}
        if ((aAlarmFlag != 3)&&(bAlarmFlag != 3)&&(bSecTime < aSecTime)){sleepSecTime = bSecTime - nowSecTime;}
        if ((aAlarmFlag != 3)&&(bAlarmFlag != 3)&&(aSecTime == bSecTime)){sleepSecTime = aSecTime - nowSecTime;}
        if ((aAlarmFlag != 3)&&(bAlarmFlag == 3)) {sleepSecTime = aSecTime - nowSecTime;}
        if ((aAlarmFlag == 3)&&(bAlarmFlag != 3)) {sleepSecTime = bSecTime - nowSecTime;}
        if ((aAlarmFlag == 3)&&(bAlarmFlag == 3)) {sleepSecTime = 2628000;}//sleep one month, will wake and go to sleep again after that one month
        // Calculates the hr, min, and sec to sleep      
        hrToSleep = int(sleepSecTime/3600); minToSleep = int((sleepSecTime-(3600*hrToSleep))/60); secToSleep = int(sleepSecTime-(3600*hrToSleep)-(60*minToSleep));
        serialDebug();//<--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------DEBUGGIN
        //Shuts down SAS functionality    
        tempSensor.shutdown(); digitalWrite(oledPowerPin, LOW); digitalWrite(tempPowerPin, LOW); digitalWrite(irPowerPin, LOW); // Turn off the power to various sensors
        alarm.setRtcTimer(hrToSleep, minToSleep, secToSleep);// Sets the RTC TIMER with the appropriate amount of hour, min, sec to sleep
        Serial.end();
        delay (1000);
        //Puts the SAS to sleep and determines what woke it up       
        int who; 
        who = Snooze.hibernate( config_teensy35 );// wake up the Teensy and identify who woke it up
        if (who == REED_INTERRUPT_PIN){
          delay(500);
          if (digitalRead(REED_INTERRUPT_PIN)==HIGH){
            delay(500);
            if (digitalRead(REED_INTERRUPT_PIN)==HIGH){
              digitalWrite(oledPowerPin, HIGH); digitalWrite(tempPowerPin, HIGH); digitalWrite(irPowerPin, HIGH); tempSensor.wake(); Serial.begin(9600); display.begin(SSD1306_SWITCHCAPVCC, 0x3C);// Restart SAS functionality
              display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE,BLACK); display.setCursor(0,16); display.println("Interrupt"); display.display();// Update the OLED display
              delay (500);      
              menu = 0; pos = 0;//Sets menu and position to 0
              delay(500);  
            }
            if (digitalRead(REED_INTERRUPT_PIN)==LOW){menu = 9;}
          }
          if (digitalRead(REED_INTERRUPT_PIN)==LOW){menu = 9;}
        }
        else{//alarm pin wake up
          digitalWrite(oledPowerPin, HIGH); digitalWrite(tempPowerPin, HIGH); digitalWrite(irPowerPin, HIGH); tempSensor.wake(); Serial.begin(9600); display.begin(SSD1306_SWITCHCAPVCC, 0x3C);// Restart SAS functionality
          delay (500);      
        }
      }
    //SAS SHOULD BE SAMPLING
    else{
      //PUMP A
      if (aAlarmFlag == 0){//Execute the following if aAlarmFlag is 0
        digitalWrite(pumpAPin, LOW);//Make sure pumpA is off
        if ((aSecTime != 0)&&(nowSecTime > aSecTime)){aAlarmFlag = 1;}//If the time now is past the time pumpA should go off, set aAlarmFlag to 1
      }
      if (aAlarmFlag == 1){//Execute the following if aAlarmFlag is 1
        digitalWrite(pumpAPin, HIGH);//Turn on pumpA
        aEndTime = nowSecTime + (int(sampleVolume/(pumpACalibration/10.0))); //Calculate the amount of time that pumpA should run
        logData();
        aAlarmFlag = 2;
      }
      if (aAlarmFlag == 2){//Execute the following if aAlarmFlag is 2
        digitalWrite(pumpAPin, HIGH);//Make sure pumpA is on
        if (nowSecTime > aEndTime){//Execute the following if duration of pumpA time is greater than the amount of time pumpA should run
          if (sampleMode == 1){aAlarmFlag = 3;aSecTime=0;}//If the sample mode is set to once, set aAlarmFlag to 3, i.e., never to run again.
          if (sampleMode == 0){//If the sample mode is set to daily, calculate the new aSecTime, the time for the aAlarm to wake
            aSecTime = (aSecTime + 86400);// Add 24 hrs to the time that A fires (aSecTime)
            tmElements_t updateAtm;//This is a time elements variable named updateAtm
            breakTime(aSecTime, updateAtm);// Break aSecTime into the time elements variable updateTm
            aHr = updateAtm.Hour;// Name the hour that pump A needs to fire aHr 
            aMin = updateAtm.Minute;// Name the minute that pump A needs to fire aMin 
            aSec = updateAtm.Second;// Name the second that pump A needs to fire aSec 
            aDay = updateAtm.Day;// Name the day that pump A needs to fire aDay
            aMon = updateAtm.Month;// Name the month that pump A needs to fire aMon 
            aYr = updateAtm.Year-30;// Name the year that pump A needs to fire aYr. Minus thirty because value is years since 1970, and we are dealing with years since 2000
            aAlarmFlag = 0; //Set aAlarmFlag to 0, so that it will restart and fire again when it wakes up at the appropriate time
            digitalWrite(pumpAPin, LOW);
          }
        }
      }
      if (aAlarmFlag == 3){digitalWrite(pumpAPin, LOW);}//If aAlarmFlag is 3, then make sure pumpA is off
      //PUMP B
      if (bAlarmFlag == 0){//Execute the following if bAlarmFlag is 0
        digitalWrite(pumpBPin, LOW);//Make sure pumpB is off
        if ((bSecTime != 0)&&(nowSecTime > bSecTime)){bAlarmFlag = 1;}//If the time now is past the time pumpB should go off, set bAlarmFlag to 1
      }
      if (bAlarmFlag == 1){//Execute the following if bAlarmFlag is 1
        digitalWrite(pumpBPin, HIGH);//Turn on pumpB
        bEndTime = nowSecTime + (int(sampleVolume/(pumpBCalibration/10.0))); //Calculate the amount of time that pumpB should run
        logData();
        bAlarmFlag = 2;
      }
      if (bAlarmFlag == 2){//Execute the following if bAlarmFlag is 2
        digitalWrite(pumpBPin, HIGH);//Make sure pumpB is on
        if (nowSecTime > bEndTime){//Execute the following if duration of pumpB time is greater than the amount of time pumpB should run
          if (sampleMode == 1){bAlarmFlag = 3;bSecTime=0;}//If the sample mode is set to once, set bAlarmFlag to 3, i.e., never to run again.
          if (sampleMode == 0){//If the sample mode is set to daily, calculate the new bSecTime, the time for the bAlarm to wake
            bSecTime = (bSecTime + 86400);// Add 24 hrs to the time that B fires (bSecTime)
            tmElements_t updateBtm;//This is a time elements variable named updateBtm
            breakTime(bSecTime, updateBtm);// Break bSecTime into the time elements variable updateTm
            bHr = updateBtm.Hour;// Name the hour that pump B needs to fire bHr 
            bMin = updateBtm.Minute;// Name the minute that pump B needs to fire bMin 
            bSec = updateBtm.Second;// Name the second that pump B needs to fire bSec 
            bDay = updateBtm.Day;// Name the day that pump B needs to fire bDay
            bMon = updateBtm.Month;// Name the month that pump B needs to fire bMon 
            bYr = updateBtm.Year-30;// Name the year that pump B needs to fire bYr. Minus thirty because value is years since 1970, and we are dealing with years since 2000
            bAlarmFlag = 0; //Set bAlarmFlag to 0, so that it will restart and fire again when it wakes up at the appropriate time
            digitalWrite(pumpBPin, LOW);
          }
        }
      }
      if (bAlarmFlag == 3){digitalWrite(pumpBPin, LOW);}//If aAlarmFlag is 3, then make sure pumpB is off
      samplingDisplay();
    }  
  }
}

//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
//<---------------------------------------------------------------MENU NAVIGATION-------------------------------------------------------------------------------->
//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
void statusMenuNavigation(){
  if(irrecv.decode(&results)){// If there is an IR signal, return true and store signal in "results"
    switch(pos){// What is the selected position within this menu?
     case 0:// Position is 0
      switch (results.value){// What was the signal received from the IR remote?
        case LEFTIR: menu = 8; break; case RIGHTIR: menu = 1; break;
      };break;
    }  
    irrecv.resume();// Continue receiving looking for IR signals
  }
}

void settingsMenuNavigation(){
  if(irrecv.decode(&results)){// If there is an IR signal, return true and store signal in "results"
    switch(pos){// What is the selected position within this menu?
      case 0:// Position is 0
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: menu = 0; break; case RIGHTIR: menu = 2; break; case UPIR: pos = 0; break; case DOWNIR: pos = 1; break;// Navigate           
        };break;
      case 1:// Position is 1
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 0; break; case RIGHTIR: pos = 11; break; case UPIR: pos = 0; break; case DOWNIR: pos = 2; break;// Navigate            
        };break;
      case 11:// Position is 11
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 1; break; case RIGHTIR: pos = 2; break; case UPIR: sampleMode++; break; case DOWNIR: sampleMode--; break;// Navigate or change sample mode  
        }break;
      case 2:// Position is 2
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 11; break; case RIGHTIR: pos = 21; break; case UPIR: pos = 1; break; case DOWNIR: pos = 3; break;// Navigate 
        }break;
      case 21:// Position is 21
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 2; break; case RIGHTIR: pos = 3; break; case UPIR: sampleVolume=sampleVolume+sampleVolumeChangeMl; break; case DOWNIR: sampleVolume=sampleVolume-sampleVolumeChangeMl; break;// Navigate or change sample volume by sampleVolumeChangeMl in mL
        }break;
      case 3:
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 21; break; case RIGHTIR: pos = 3; break; case UPIR: pos = 2; break; case DOWNIR: pos = 3; break; case ENTERIR: writeSampleParamArray(); break;// Navigate or write sample parameters to the SD card     
        }break;
    }  
    irrecv.resume();// Continue receiving looking for IR signals
  }
}

void pumpAMenuNavigation(){
  if(irrecv.decode(&results)){// If there is an IR signal, return true and store signal in "results"
    switch(pos){// What is the selected position within this menu?
      case 0:// Position is 0
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: menu = 1; break; case RIGHTIR: menu = 3; break; case UPIR: pos = 0; break; case DOWNIR: pos = 1; break;// Navigate    
        }break;
      case 1:// Position is 1
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 0; break; case RIGHTIR: pos = 11; break; case UPIR: pos = 0; break; case DOWNIR: pos = 2; break;// Navigate  
        }break;
      case 11:// Position is 11
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 1; break; case RIGHTIR: pos = 12; break; case UPIR: aHr++; break; case DOWNIR: aHr--; break;// Navigate or change hour Pump A fires  
        }break;
      case 12:// Position is 12
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 11; break; case RIGHTIR: pos = 2; break; case UPIR: aMin++; break; case DOWNIR: aMin--; break;// Navigate or change minute Pump A fires   
        }break;
      case 2:// Position is 2
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 12; break; case RIGHTIR: pos = 21; break; case UPIR: pos = 1; break; case DOWNIR: pos = 3; break;// Navigate
        }break;
      case 21:// Position is 21
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 2; break; case RIGHTIR: pos = 22; break; case UPIR: aDay++; break; case DOWNIR: aDay--; break;// Navigate or change day Pump A fires   
        }break;
      case 22:// Position is 22
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 21; break; case RIGHTIR: pos = 23; break; case UPIR: aMon++; break; case DOWNIR: aMon--; break;// Navigate or change month Pump A fires   
        }break;
      case 23:// Position is 23
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 22; break; case RIGHTIR: pos = 3; break; case UPIR: aYr++; break; case DOWNIR: aYr--; break;// Navigate or change year Pump A fires  
        }break;
      case 3:// Position is 3
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 23; break; case RIGHTIR: pos = 3; break; case UPIR: pos = 2; break; case DOWNIR: pos = 3; break; case ENTERIR: writeSampleParamArray(); calculateInitialAlarmSecondTime(); break;// Navigate or save sample parameters to the SD card
        }break;
    }  
    irrecv.resume();// Continue receiving looking for IR signals
  }
}

void pumpBMenuNavigation(){
  if(irrecv.decode(&results)){// If there is an IR signal, return true and store signal in "results"
    switch(pos){// What is the selected position within this menu?
      case 0:// Position is 0
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: menu = 2; break; case RIGHTIR: menu = 4; break; case UPIR: pos = 0; break; case DOWNIR: pos = 1; break;// Navigate    
        }break;
      case 1:// Position is 1
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 0; break; case RIGHTIR: pos = 11; break; case UPIR: pos = 0; break; case DOWNIR: pos = 2; break;// Navigate  
        }break;
      case 11:// Position is 11
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 1; break; case RIGHTIR: pos = 12; break; case UPIR: bHr++; break; case DOWNIR: bHr--; break;// Navigate or change hour Pump B fires    
        }break;
      case 12:// Position is 12
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 11; break; case RIGHTIR: pos = 2; break; case UPIR: bMin++; break; case DOWNIR: bMin--; break;// Navigate or change minute Pump B fires    
        }break;
      case 2:// Position is 2
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 12; break; case RIGHTIR: pos = 21; break; case UPIR: pos = 1; break; case DOWNIR: pos = 3; break;// Navigate  
        }break;
      case 21:// Position is 21
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 2; break; case RIGHTIR: pos = 22; break; case UPIR: bDay++; break; case DOWNIR: bDay--; break;// Navigate or change day Pump B fires    
        }break;
      case 22:// Position is 22
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 21; break; case RIGHTIR: pos = 23; break; case UPIR: bMon++; break; case DOWNIR: bMon--; break;// Navigate or change month Pump B fires    
        }break;
      case 23:// Position is 23
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 22; break; case RIGHTIR: pos = 3; break; case UPIR: bYr++; break; case DOWNIR: bYr--; break;// Navigate or change year Pump B fires    
        }break;
      case 3:// Position is 3
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 23; break; case RIGHTIR: pos = 3; break; case UPIR: pos = 2; break; case DOWNIR: pos = 3; break; case ENTERIR: writeSampleParamArray(); calculateInitialAlarmSecondTime(); break;// Navigate or save sample parameters to the SD card     
        }break;
    }  
    irrecv.resume();// Continue receiving looking for IR signals
  }
}
void initiateMenuNavigation(){
  if(irrecv.decode(&results)){// If there is an IR signal, return true and store signal in "results"
    switch(pos){// What is the selected position within this menu?
      case 0:// Position is 0
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: menu = 3; break; case RIGHTIR: menu = 5; break; case DOWNIR: pos = 1; break;//Navigate   
        };break;
      case 1:// Position is 1
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 1; break; case RIGHTIR: pos = 1; break; case UPIR: pos = 0; break; case DOWNIR: pos = 1; break; case ENTERIR: initialAlarmFlag(); menu = 9; break;  //check the alarms then start sleeping then calculates the amount of time to sleep
        }break;
    }  
    irrecv.resume();// Continue receiving looking for IR signals
  }
}
void timeSetMenuNavigation(){
  if(irrecv.decode(&results)){// If there is an IR signal, return true and store signal in "results"
    switch(pos){// What is the selected position within this menu?
      case 0:// Position is 0
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: menu = 4; break; case RIGHTIR: menu = 6; break; case UPIR: pos = 0; break; case DOWNIR: pos = 1; break;// Navigate  
        }break;
      case 1:// Position is 1
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 0; break; case RIGHTIR: pos = 11; break; case UPIR: pos = 0; break; case DOWNIR: pos = 2; break;// Navigate  
        }break;
      case 11:// Position is 11
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 1; break; case RIGHTIR: pos = 12; break; case UPIR: nowHr++; break; case DOWNIR: nowHr--; break;// Navigate or set the hour it is now
        }break;
      case 12:// Position is 12
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 11; break; case RIGHTIR: pos = 13; break; case UPIR: nowMin++; break; case DOWNIR: nowMin--; break;// Navigate or set the minute it is now  
        }break;
      case 13:// Position is 13
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 12; break; case RIGHTIR: pos = 2; break; case UPIR: nowSec++; break; case DOWNIR: nowSec--; break;// Navigate or set the second it is now    
        }break;
      case 2:// Position is 2
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 13; break; case RIGHTIR: pos = 21; break; case UPIR: pos = 1; break; case DOWNIR: pos = 3; break;// Navigate  
        }break;
      case 21:// Position is 21
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 2; break; case RIGHTIR: pos = 22; break; case UPIR: nowDay++; break; case DOWNIR: nowDay--; break;// Navigate or set the day it is now    
        }break;
      case 22:// Position is 22
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 21; break; case RIGHTIR: pos = 23; break; case UPIR: nowMon++; break; case DOWNIR: nowMon--; break;// Navigate or set the month it is now    
        }break;
      case 23:// Position is 23
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 22; break; case RIGHTIR: pos = 3; break; case UPIR: nowYr++; break; case DOWNIR: nowYr--; break;// Navigate or set the year it is now    
        }break;
      case 3:// Position is 3
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 23; break; case RIGHTIR: pos = 3; break; case UPIR: pos = 2; break; case DOWNIR: pos = 3; break; case ENTERIR: calcUpdateSecTime(); Teensy3Clock.set(updateSecTime); display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE,BLACK); display.setCursor(0,0); display.println(" <SAVED>"); display.display(); delay(1000); break;//Navigate or 1. calculate the updated time in secons, set the clock to that second time, display "<SAVED>"     
        }break;
    }  
    irrecv.resume();// Continue receiving looking for IR signals
  }
}
void pumpCalibrationMenuNavigation(){
  if(irrecv.decode(&results)){// If there is an IR signal, return true and store signal in "results"
    switch(pos){// What is the selected position within this menu?
      case 0:// Position is 0
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: menu = 5; break; case RIGHTIR: menu = 7; break; case UPIR: pos = 0; break; case DOWNIR: pos = 1; break;// Navigate           
        };break;
      case 1:// Position is 1
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 0; break; case RIGHTIR: pos = 11; break; case UPIR: pos = 0; break; case DOWNIR: pos = 2; break;// Navigate           
        };break;
      case 11:// Position is 11
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 1; break; case RIGHTIR: pos = 2; break; case UPIR: pumpACalibration++; break; case DOWNIR: pumpACalibration--; break;// Navigate or change pump A calibration  
        }break;
      case 2:// Position is 2
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 11; break; case RIGHTIR: pos = 21; break; case UPIR: pos = 1; break; case DOWNIR: pos = 3; break;// Navigate  
        }break;
      case 21:// Position is 21
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 2; break; case RIGHTIR: pos = 3; break; case UPIR: pumpBCalibration++; break; case DOWNIR: pumpBCalibration--; break;// Navigate or change pump A calibration    
        }break;
      case 3:// Position is 3
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 21; break; case RIGHTIR: pos = 3; break; case UPIR: pos = 2; break; case DOWNIR: pos = 3; break; case ENTERIR: writeSampleParamArray(); break;// Navigate or save sample parameters to the SD card     
        }break;
      }
    irrecv.resume();// Continue receiving looking for IR signals
    }
  }
void pumpCalibrationRunMenuNavigation(){
  if(irrecv.decode(&results)){// If there is an IR signal, return true and store signal in "results"
    switch(pos){// What is the selected position within this menu?
      case 0:// Position is 0
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: menu = 6; break; case RIGHTIR: menu = 8; break; case UPIR: pos = 0; break; case DOWNIR: pos = 1; break;// Navigate           
        };break;
      case 1:// Position is 1
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 0; break; case RIGHTIR: pos = 11; break; case UPIR: pos = 0; break; case DOWNIR: pos = 2; break;// Navigate           
        };break;
      case 11:// Position is 11
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 1; break; case ENTERIR: runPumpA10sec(); break; case RIGHTIR: pos = 2; break;// Run pump A for 10 second calibration
        }break;
      case 2:// Position is 2
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 1; break; case RIGHTIR: pos = 21; break; case UPIR: pos = 1; break; // Navigate  
        }break;
      case 21:// Position is 21
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: pos = 2; break; case ENTERIR: runPumpB10sec(); break; case RIGHTIR: pos = 0; break;// Run pump B for 10 second calibration
        }break;
    }
    irrecv.resume();// Continue receiving looking for IR signals
  }
}
void versionMenuNavigation(){
  if(irrecv.decode(&results)){// If there is an IR signal, return true and store signal in "results"
    switch(pos){// What is the selected position within this menu?
      case 0:// Position is 0
        switch (results.value){// What was the signal received from the IR remote?
          case LEFTIR: menu = 7; break; case RIGHTIR: menu = 0; break; case UPIR: pos = 0; break; case DOWNIR: pos = 0; break;// Navigate           
        };break;
    }
    irrecv.resume();// Continue receiving looking for IR signals
  }
}

//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
//<--------------------------------------------------------------------MENU DISPLAY------------------------------------------------------------------------------>
//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
void statusMenuDisplay(){// Text to display in the status menu
  getVoltage();// Read the voltage of the battery pack
  temp=tempSensor.readTempC();// Read the temperature sensor
  display.clearDisplay();// Clear the display 
  display.setTextSize(1);// Set the text size to 1 
  if (pos==0){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 0 is selected, highlight text in that position
  display.setCursor(0,0); display.println("    <STATUS MENU>    ");// Display "    <STATUS MENU>    " on the first line
  display.setTextColor(WHITE,BLACK);// Set the color of the text to normal
  display.setCursor(0,8); display.println("T:"); display.setCursor(12,8); display.println(nowHr); display.setCursor(24,8); display.println(":"); display.setCursor(30,8); display.println(nowMin); display.setCursor(42,8); display.println(":"); display.setCursor(48,8); display.println(nowSec); display.setCursor(60,8); display.println(" D:"); display.setCursor(78,8); display.println(nowDay); display.setCursor(90,8); display.println("/"); display.setCursor(96,8); display.println(nowMon); display.setCursor(108,8); display.println("/"); display.setCursor(114,8); display.println(nowYr);// Display the time and date line
  display.setCursor(0,16); display.println("Temperature:"); display.setCursor(70,16); display.println(temp);// Display temperature line
  display.setCursor(0,24); display.println("Sample mode:");// Display "Sample mode:" text
  display.setCursor(72,24);// Set cursor postion to after "Sample mode:" text
  switch(sampleMode){// What is the sample mode?
    case 0: display.println("Daily"); break;// If the sample mode is 0, display "Daily"
    case 1: display.println("Once"); break;// If the sample mode is 1, display "Once"
  }
  //Pump A line
  display.setCursor(0,32); display.println("A:");// Display "A:" on the sixth line
  display.setCursor(12,32);// Set the cursor to immediately following the "A:"
  switch(sampleMode){// What is the sample mode?
    case 0: display.println("Daily@"); display.setCursor(66,32); display.println(aHr); display.setCursor(78,32); display.println(":"); display.setCursor(84,32); display.println(aMin); break;// If the sample mode is 0, display the time that Pump A will fire daily as Hour:Minute
    case 1: display.println("T:"); display.setCursor(24,32); display.println(aHr); display.setCursor(36,32); display.println(":"); display.setCursor(42,32); display.println(aMin); display.setCursor(54,32); display.println("D:"); display.setCursor(66,32); display.println(aDay); display.setCursor(78,32); display.println("/"); display.setCursor(84,32); display.println(aMon); display.setCursor(96,32); display.println("/"); display.setCursor(102,32); display.println(aYr); break;// If the sample mode is 1, display the time that Pump A will fire daily as T: Hour:Minute D: Day/Month/Year
  }
  //Pump B line
  display.setCursor(0,40); display.println("B:");// Display "B:" on the seventh line
  display.setCursor(12,40); // Set the cursor to immediately following the "B:"
  switch(sampleMode){// What is the sample mode?
    case 0: display.println("Daily@"); display.setCursor(66,40); display.println(bHr); display.setCursor(78,40); display.println(":"); display.setCursor(84,40); display.println(bMin); break;// If the sample mode is 0, display the time that Pump A will fire daily as Hour:Minute
    case 1: display.println("T:"); display.setCursor(24,40); display.println(bHr); display.setCursor(36,40); display.println(":"); display.setCursor(42,40); display.println(bMin); display.setCursor(54,40); display.println("D:"); display.setCursor(66,40); display.println(bDay); display.setCursor(78,40); display.println("/"); display.setCursor(84,40); display.println(bMon); display.setCursor(96,40); display.println("/"); display.setCursor(102,40); display.println(bYr); break;// If the sample mode is 1, display the time that Pump A will fire daily as T: Hour:Minute D: Day/Month/Year
  }
  //Volume and voltage line
  display.setCursor(0,48); display.println("Vol:"); display.setCursor(24,48); display.println(sampleVolume);  display.setCursor(48,48); display.println("Batt:"); display.setCursor(78,48); display.println(voltage); display.setCursor(108,48); display.println("v");// Display sample "Vol:" followed by sample volume, then "Batt:" followed by battery voltage and "v"
  display.display();// Update the display
}

void settingsMenuDisplay(){// Text to display in the settings menu
  display.clearDisplay();// Clear the display 
  display.setTextSize(2);// Set the text size to 2 
  if (pos==0){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 0 is selected, highlight text in that position
  display.setCursor(0,0); display.println("<SETTINGS>");// Set cursor to first line and display "<SETTINGS>"
  if (pos==1){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 1 is selected, highlight text in that position
  display.setCursor(0,16); display.println("Mode:");// Set cursor to second line and display "Mode:"
  if (pos==11){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 11 is selected, highlight text in that position
  switch(sampleMode){// What is the sample mode?
    case 0: display.setCursor(60,16); display.println("Daily"); break;// If sample mode is 0, set cursor to after "Mode" and display "Daily"
    case 1:   display.setCursor(60,16); display.println("Once"); break;// If sample mode is 1, set cursor to after "Mode" and display "Once"
  }
  if (pos==2){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 2 is selected, highlight text in that position
  display.setCursor(0,32); display.println("VOL:");// Set cursor to third line and display "VOL:"
  if (pos==21){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 21 is selected, highlight text in that position
  display.setCursor(48,32); display.println(sampleVolume);// Set cursor to after "VOL:" and display the sample volume to be collected
  if (pos==3){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 3 is selected, highlight text in that position
  display.setCursor(0,48); display.println("ENTER SET");// Set cursor to fourth line and display "ENTER SET"
  numberCorrect();// Correct all displayed numbers
  display.display();// Update the display
}

void pumpAMenuDisplay(){// Text to display in the Pump A menu
  display.clearDisplay();// Clear the display  
  display.setTextSize(2);// Set the text size to 2 
  if (pos==0){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 0 is selected, highlight text in that position
  display.setCursor(0,0); display.println(" <PUMP A> ");// Set cursor to first line and display " <PUMP A> "
  //TIME DISPLAY
  if (pos==1){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 1 is selected, highlight text in that position
  display.setCursor(0,16); display.println("T:");// Set cursor to second line and display "T:"
  if (pos==11){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 11 is selected, highlight text in that position
  display.setCursor(24,16); display.println(aHr);// Set cursor to after "T:" and display the hour pump A will fire
  display.setTextColor(WHITE,BLACK); display.setCursor(48,16); display.println(":");// Set cursor to after the hour pump A will fire and display ":"
  if (pos==12){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 12 is selected, highlight text in that position
  display.setCursor(60,16); display.println(aMin);// Set cursor to after ":" and display the minute pump A will fire
  //DAY DISPLAY
  if (pos==2){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 2 is selected, highlight text in that position
  display.setCursor(0,32);  display.println("D:");// Set cursor to third line and display "D:"
  if (pos==21){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 21 is selected, highlight text in that position
  display.setCursor(24,32); display.println(aDay);// Set cursor to after "D:" and display the day pump A will fire
  display.setTextColor(WHITE,BLACK); display.setCursor(48,32); display.println("/");// Set cursor to after the hour pump A will fire and display "/"
  if (pos==22){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 22 is selected, highlight text in that position
  display.setCursor(60,32); display.println(aMon);// Set cursor to after "/" and display the month pump A will fire
  display.setTextColor(WHITE,BLACK); display.setCursor(84,32); display.println("/");// Set cursor to after the month pump A will fire and display "/"
  if (pos==23){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 23 is selected, highlight text in that position
  display.setCursor(96,32); display.println(aYr);// Set cursor to after "/" and display the year pump A will fire
  if (pos==3){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 3 is selected, highlight text in that position
  display.setCursor(0,48); display.println("ENTER SET");// Set cursor to fourth line and display "ENTER SET"
  numberCorrect();// Correct all displayed numbers  
  display.display();// Update the display
}

void pumpBMenuDisplay(){// Text to display in the Pump B menu
  display.clearDisplay();// Clear the display  
  display.setTextSize(2);// Set the text size to 2 
  if (pos==0){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 0 is selected, highlight text in that position
  display.setCursor(0,0); display.println(" <PUMP B> ");// Set cursor to first line and display " <PUMP B> "
  //TIME DISPLAY
  if (pos==1){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 1 is selected, highlight text in that position
  display.setCursor(0,16); display.println("T:");// Set cursor to second line and display "T:"
  if (pos==11){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 11 is selected, highlight text in that position
  display.setCursor(24,16); display.println(bHr);// Set cursor to after "T:" and display the hour pump B will fire
  display.setTextColor(WHITE,BLACK); display.setCursor(48,16); display.println(":");// Set cursor to after the hour pump B will fire and display ":" 
  if (pos==12){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 12 is selected, highlight text in that position
  display.setCursor(60,16); display.println(bMin);// Set cursor to after ":" and display the minute pump B will fire
  //DAY DISPLAY
  if (pos==2){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 2 is selected, highlight text in that position
  display.setCursor(0,32);  display.println("D:");// Set cursor to third line and display "D:"
  if (pos==21){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 21 is selected, highlight text in that position
  display.setCursor(24,32); display.println(bDay); // Set cursor to after "D:" and display the day pump B will fire
  display.setTextColor(WHITE,BLACK); display.setCursor(48,32); display.println("/"); // Set cursor to after the hour pump B will fire and display "/"
  if (pos==22){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 22 is selected, highlight text in that position
  display.setCursor(60,32); display.println(bMon);// Set cursor to after "/" and display the month pump B will fire
  display.setTextColor(WHITE,BLACK); display.setCursor(84,32); display.println("/"); // Set cursor to after the month pump B will fire and display "/"
  if (pos==23){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 23 is selected, highlight text in that position
  display.setCursor(96,32); display.println(bYr);// Set cursor to after "/" and display the year pump B will fire
  if (pos==3){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 3 is selected, highlight text in that position
  display.setCursor(0,48); display.println("ENTER SET");// Set cursor to fourth line and display "ENTER SET"
  numberCorrect();// Correct all displayed numbers  
  display.display();// Update the display
}

void initiateMenuDisplay(){// Text to display in the initiate menu
  display.clearDisplay();// Clear the display  
  display.setTextSize(2);// Set the text size to 2 
  if (pos==0){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 0 is selected, highlight text in that position
  display.setCursor(0,0); display.println("<INITIATE>");// Set cursor to first line and display "INITIATE"
  display.setTextSize(1);// Set the text size to 1
  if (pos==1){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 1 is selected, highlight text in that position
  display.setCursor(0,16); display.println("Press Enter");// Set cursor to second line and display "Press Enter"
  display.display();// Update the display
}

void timeSetMenuDisplay(){// Text to display in the time setting menu
  display.clearDisplay();// Clear the display  
  display.setTextSize(2);// Set the text size to 2 
  if (pos==0){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 0 is selected, highlight text in that position
  display.setCursor(0,0); display.println("<TIME SET>");// Set cursor to first line and display "TIME SET>"
   //TIME DISPLAY
  if (pos==1){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 1 is selected, highlight text in that position
  display.setCursor(0,16); display.println("T:");// Set cursor to second line and display "T:"
  if (pos==11){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 11 is selected, highlight text in that position
  display.setCursor(24,16); display.println(nowHr);// Set cursor to after "T:" and display the hour the clock should be
  display.setTextColor(WHITE,BLACK); display.setCursor(48,16); display.println(":");// Set cursor to after the hour the clock should be and display ":" 
  if (pos==12){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 12 is selected, highlight text in that position
  display.setCursor(60,16); display.println(nowMin);// Set cursor to after ":" and display the minute the clock should be
  display.setTextColor(WHITE,BLACK); display.setCursor(84,16); display.println(":");// Set cursor to after the minute the clock should be and display ":" 
  if (pos==13){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 13 is selected, highlight text in that position
  display.setCursor(96,16); display.println(nowSec);// Set cursor to after ":" and display the second the clock should be
  //DAY DISPLAY
  if (pos==2){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 2 is selected, highlight text in that position
  display.setCursor(0,32);  display.println("D:");// Set cursor to third line and display "D:"
  if (pos==21){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 21 is selected, highlight text in that position
  display.setCursor(24,32); display.println(nowDay);  // Set cursor to after "D:" and display the day the clock should be
  display.setTextColor(WHITE,BLACK); display.setCursor(48,32); display.println("/"); // Set cursor to after the hour the clock should be and display "/"
  if (pos==22){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 22 is selected, highlight text in that position
  display.setCursor(60,32); display.println(nowMon);// Set cursor to after "/" and display the month the clock should be
  display.setTextColor(WHITE,BLACK); display.setCursor(84,32); display.println("/"); // Set cursor to after the month the clock should be and display "/" 
  if (pos==23){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 23 is selected, highlight text in that position
  display.setCursor(96,32); display.println(nowYr);// Set cursor to after "/" and display the year the clock should be
  if (pos==3){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 3 is selected, highlight text in that position
  display.setCursor(0,48); display.println("ENTER SET");// Set cursor to fourth line and display "ENTER SET"
  numberCorrect();// Correct all displayed numbers  
  display.display();// Update the display
}
void pumpCalibrationMenuDisplay(){// Text to display in the calibration menu
  display.clearDisplay();// Clear the display  
  display.setTextSize(2);// Set the text size to 2  
  if (pos==0){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 0 is selected, highlight text in that position
  display.setCursor(0,0); display.println("<PUMPCAL>");// Set cursor to first line and display " <PUMPCAL> "
  if (pos==1){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 1 is selected, highlight text in that position
  display.setCursor(0,16); display.println("PA:");// Set cursor to second line and display "PA:"
  if (pos==11){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 11 is selected, highlight text in that position
  display.setCursor(48,16); display.println(pumpACalibration);// Set cursor to after "PA:" and display Pump A Calibration
  if (pos==2){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 2 is selected, highlight text in that position
  display.setCursor(0,32); display.println("PB:");// Set cursor to third line and display "PB:"
  if (pos==21){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 21 is selected, highlight text in that position
  display.setCursor(48,32); display.println(pumpBCalibration);// Set cursor to after "PB:" and display Pump B Calibration
  if (pos==3){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 3 is selected, highlight text in that position
  display.setCursor(0,48); display.println("ENTER SET");// Set cursor to fourth line and display "ENTER SET"
  numberCorrect();// Correct all displayed numbers
  display.display();// Update the display
}
void pumpCalibrationRunMenuDisplay(){// Text to display in the calibration menu
  display.clearDisplay();// Clear the display  
  display.setTextSize(2);// Set the text size to 2  
  if (pos==0){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 0 is selected, highlight text in that position
  display.setCursor(0,0); display.println("<RUNCAL>");// Set cursor to first line and display " <RUNCAL> "
  if (pos==1){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 1 is selected, highlight text in that position
  display.setCursor(0,16); display.println("PA:");// Set cursor to second line and display "PA:"
  if (pos==11){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 11 is selected, highlight text in that position
  display.setCursor(48,16); display.println("START");// Set cursor to after "PA:" and show "START"
  if (pos==2){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 2 is selected, highlight text in that position
  display.setCursor(0,32); display.println("PB:");// Set cursor to third line and display "PB:"
  if (pos==21){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 21 is selected, highlight text in that position
  display.setCursor(48,32); display.println("START");// Set cursor to after "PA:" and show "START"
  display.display();// Update the display
}
void versionMenuDisplay(){// Text to display in the version menu
  display.clearDisplay();// Clear the display  
  display.setTextSize(2);// Set the text size to 2  
  if (pos==0){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 0 is selected, highlight text in that position
  display.setCursor(0,0); display.println("<VERSION>  ");// Set cursor to first line and display " <VERSION> "
  if (pos==1){display.setTextColor(BLACK,WHITE);} else{display.setTextColor(WHITE,BLACK);}// If position 1 is selected, highlight text in that position
  display.setCursor(0,16); display.println(sasVersion);// Set cursor to second line and display the version (e.g. V2J)
  display.display();// Update the display
}

//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
//<-------------------------------------------------------------------SAMPLING MODE------------------------------------------------------------------------------>
//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->

void samplingDisplay(){//Updates the display during sampling
  if ((aAlarmFlag == 2)&&(bAlarmFlag !=2)){display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE,BLACK); display.setCursor(0,16); display.println("Pump A");}// Display Pump A
  if ((aAlarmFlag != 2)&&(bAlarmFlag ==2)){display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE,BLACK); display.setCursor(0,16); display.println("Pump B");}// Display Pump B
  if ((aAlarmFlag == 2)&&(bAlarmFlag ==2)){display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE,BLACK); display.setCursor(0,16); display.println("Both Pumps");}// Display Both Pumps
  if ((aAlarmFlag != 2)&&(bAlarmFlag !=2)){display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE,BLACK); display.setCursor(0,16); display.println("Wait...");}// Display Wait...
  display.display();// Update the OLED display
}

void initialAlarmFlag(){//sets the alarmFlags immediately upon sleep, i.e., the initial alarm flags
  if (nowSecTime > aSecTime){aAlarmFlag = 3;}//if aSecTime is in the past, set it to never run, i.e., aAlarmFlag is 3
  if (nowSecTime < aSecTime){aAlarmFlag = 0;}//if aSectTime is in the future, set aAlarmFlag to 0
  if (nowSecTime > bSecTime){bAlarmFlag = 3;}//if bSecTime is in the past, set it to never run, i.e., bAlarmFlag is 3
  if (nowSecTime < bSecTime){bAlarmFlag = 0;}//if bSectTime is in the future, set bAlarmFlag to 0
}

//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
//<--------------------------------------------------------------------SD CARD CODE------------------------------------------------------------------------------>
//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
void readSampleParamArray(){
  sampleParam = sd.open("SAMPLEPARAM.TXT", FILE_WRITE);// Open SAMPLEPARAM.txt file on sd card
  if (!sampleParam){Serial.println("open failed");}// If opening failed, write "open failed" to the serial port for debugging
  sampleParam.rewind();// Rewind the file for read.
  // Array for data.
  int i = 0;// Index for array rows
  int j = 0;// Index for array columns
  size_t n;// Length of returned field with delimiter.
  char str[20];// Longest field with delimiter and zero byte.
  char *ptr;// Test for valid field.
  // Read the file and store the data.
  for (i = 0; i < SAMPLE_PARAM_ROW; i++) {// Go through each row of the sampleParam.txt file
    for (j = 0; j < SAMPLE_PARAM_COL; j++) {// Within each row, go through each column of data
      n = readField(&sampleParam, str, sizeof(str), ",\n");// Read each character
      if (n == 0) {Serial.println("Too few lines");}
      sampleParamArray[i][j] = strtol(str, &ptr, 10);
      if (ptr == str) {Serial.println("bad number");}
      while (*ptr == ' ') {ptr++;}
      if (*ptr != ',' && *ptr != '\n' && *ptr != '\0') {Serial.println("extra characters in field");}
      if (j < (SAMPLE_PARAM_COL-1) && str[n-1] != ',') {Serial.println("line with too few fields");}
    }
    if (str[n-1] != '\n' && sampleParam.available()) {Serial.println("missing endl");}    
  }
  sampleParam.sync(); delay(500);
  //Read in data
  sampleMode = sampleParamArray[0][0]; pumpACalibration = sampleParamArray[0][1]; pumpBCalibration = sampleParamArray[0][2]; sampleVolume = sampleParamArray[0][3]; blankSdParam = blankSdParam;// Read first line of data from sampleParamArray and assign the values to 1. Sample mode, 2. Pump A calibration, 3. Pump B calibration, 4. Sample volume, 5. A blank value
  aHr = sampleParamArray[1][0];aMin = sampleParamArray[1][1];aDay = sampleParamArray[1][2];aMon = sampleParamArray[1][3];aYr = sampleParamArray[1][4];// Read second line of data from sampleParamArray and assign the values to the time Pump A will fire as 1. Hour, 2. Minute, 3. Day, 4. Month, 5. Year
  bHr = sampleParamArray[2][0];bMin = sampleParamArray[2][1];bDay = sampleParamArray[2][2];bMon = sampleParamArray[2][3];bYr = sampleParamArray[2][4];// Read second line of data from sampleParamArray and assign the values to the time Pump B will fire as 1. Hour, 2. Minute, 3. Day, 4. Month, 5. Year  
}

size_t readField(File* sampleParam, char* str, size_t size, const char* delim) {
  char ch;
  size_t n = 0;
  while ((n + 1) < size && sampleParam->read(&ch, 1) == 1) {
    if (ch == '\r') {continue;}// Delete Carriage Return.
    str[n++] = ch;
    if (strchr(delim, ch)) {break;}
  }
  str[n] = '\0';
  return n;
}

void writeSampleParamArray(){
  sampleParam = sd.open("SAMPLEPARAM.TXT", FILE_WRITE);// Open the SAMPLEPARAM.TXT file on the SD card
  if (!sampleParam) {Serial.println("open failed");}// If opening the file failed, write a open failed to serial debugging
  //CREATE SAMPLE PARAMETER STRINGS (one per line)
  String sampleParamString0 = String(sampleMode) +","+String(pumpACalibration)+","+String(pumpBCalibration)+","+String(sampleVolume)+","+String(blankSdParam)+"\r\n";// Create string of the first line of sample parameters
  String sampleParamString1 = String(aHr) +","+String(aMin)+","+String(aDay)+","+String(aMon)+","+String(aYr)+"\r\n";// Create string of the second line of sample parameters
  String sampleParamString2 = String(bHr) +","+String(bMin)+","+String(bDay)+","+String(bMon)+","+String(bYr)+"\r\n";// Create string of the third line of sample parameters
  //WRITE STRINGS TO SD CARD
  sampleParam.rewind(); delay(50);// Rewind to the beginning of the SAMPLEPARAM.txt file
  sampleParam.print(sampleParamString0); delay(50);// Overwrite the first line of the SAMPLEPARAM.txt file
  sampleParam.print(sampleParamString1); delay(50);// Overwrite the second line of the SAMPLEPARAM.txt file
  sampleParam.print(sampleParamString2); delay(50);// Overwrite the third line of the SAMPLEPARAM.txt file
  sampleParam.sync(); delay(500);// Sync the SAMPLEPARAM.txt file

  display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE,BLACK); display.setCursor(0,0); display.println(" <SAVED>");// Put "<SAVED>" on the first line
  display.display();// Update the display
  delay(1000);
}

void logData(){// Called to check if need to log temperature
    temp=tempSensor.readTempC();// Read the temperature sensor and pass it to temp
    getVoltage();// Read the voltage
    delay(200);
    dataLog = sd.open("dataLog.TXT", FILE_WRITE);// Open dataLog.TXT file
    delay(200);
    if (!dataLog){Serial.println("open failed");}// If open dataLog.txt file doesn't open, write an error to the serial for debugging
    String dataLogString = String(nowHr) + ":" + String(nowMin) + ":" + String(nowSec) + "," + String(nowDay) + "/" + String(nowMon) + "/" + String(nowYr+2000) + "," + String(temp)+ "," + String(voltage)+ "," +String(aAlarmFlag)+ "," +String(bAlarmFlag);// Create a data logging string
    dataLog.println(dataLogString); delay(100);// Write the datalog string to the dataLog.txt file
    dataLog.sync();// Sync the dataLog.txt file
    delay(200);
}

//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
//<-------------------------------------------------------------------------OTHER CODE--------------------------------------------------------------------------->
//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
void runPumpA10sec(){
  digitalWrite(pumpAPin, HIGH); delay(10000);// Power on Pump A for 10 seconds
  digitalWrite(pumpAPin, LOW);  // turn off Pump A
}
void runPumpB10sec(){
  digitalWrite(pumpBPin, HIGH); delay(10000);// Power on Pump B for 10 seconds
  digitalWrite(pumpBPin, LOW);  // turn off Pump B
}
void getVoltage(){// Called when we need to know the voltage of the battery pack
  int adcVoltage = adc->analogRead(voltageReadPin);// 
  rawVoltage = adcVoltage*3.3/65536;// Raw voltage is equal to that read by the ADC times the max voltage, divided by the max ADC value (for 16-bit integer)
  voltage = 3.930*rawVoltage;// Voltage divider. R1 = 390k ohms and R2 = 100k ohms. 3.930 is variable for resistance of whole system
}
void numberCorrect(){
//Settings correct
  if (sampleMode>1){sampleMode = 0;}// If sample mode is greater than 1, make it 0
  if (sampleMode<0){sampleMode = 1;}// If sample mode is less than 0, make it 1
  if (sampleVolume<10){sampleVolume = 900;}// If sample volume is less than 10, make it 900
  if (sampleVolume>900){sampleVolume = 10;}// If sample volume is greater than 900, make it 10
  if (pumpACalibration<0){pumpACalibration = 100;}// If pump A calibration is less than 0, make it 100
  if (pumpACalibration>100){pumpACalibration = 0;}// If pump A calibration is greater than 100, make it 0
  if (pumpBCalibration<0){pumpBCalibration = 100;}// If pump B calibration is less than 0, make it 100
  if (pumpBCalibration>100){pumpBCalibration = 0;}// If pump B calibration is greater than 100, make it 0
//Date and Time Correct
  if (((nowYr != 20)&&(nowYr != 24)&&(nowYr != 28)&&(nowYr != 32)&&(nowYr != 36)&&(nowYr != 40)&&(nowYr != 44))&&(nowMon==2)&&(nowDay > 28)){nowDay=1;}// Sets max day in Feb for non leap years up to 2044, and loops to 1 if you exceed
  if (((nowYr == 20)||(nowYr == 24)||(nowYr == 28)||(nowYr == 32)||(nowYr == 36)||(nowYr == 40)||(nowYr == 44))&&(nowMon==2)&&(nowDay > 29)){nowDay=1;}// Sets max day in Feb for leap years up to 2044, and loops to 1 if you exceed
  if (((nowMon == 4)||(nowMon == 6)||(nowMon == 9)||(nowMon == 11))&&(nowDay > 30)){nowDay=1;}// Sets max days for April, June, September, November, and loops to 1 if you exceed
  if (((nowMon == 1)||(nowMon == 3)||(nowMon == 5)||(nowMon == 7)||(nowMon == 8)||(nowMon == 10)||(nowMon == 12))&&(nowDay > 31)){nowDay=1;}// Sets max days for January, March, May, July, August, October, December, and loops to 1 if you exceed
  if (((nowYr != 20)&&(nowYr != 24)&&(nowYr != 28)&&(nowYr != 32)&&(nowYr != 36)&&(nowYr != 40)&&(nowYr != 44))&&(nowMon==2)&&(nowDay < 1)){nowDay=28;}// Sets max day in Feb for non leap years up to 2044, and loops to max if you go lower than 1
  if (((nowYr == 20)||(nowYr == 24)||(nowYr == 28)||(nowYr == 32)||(nowYr == 36)||(nowYr == 40)||(nowYr == 44))&&(nowMon==2)&&(nowDay < 1)){nowDay=29;}// Sets max day in Feb for leap years up to 2044, and loops to max if you go lower than 1
  if (((nowMon == 4)||(nowMon == 6)||(nowMon == 9)||(nowMon == 11))&&(nowDay < 1)){nowDay=30;}// Sets max days for April, June, September, November, and loops to max if you go lower than 1
  if (((nowMon == 1)||(nowMon == 3)||(nowMon == 5)||(nowMon == 7)||(nowMon == 8)||(nowMon == 10)||(nowMon == 12))&&(nowDay < 1)){nowDay=31;}// Sets max days for January, March, May, July, August, October, and loops to max if you go lower than 1
  if (nowYr < 0){nowYr=0;}// Sets minimum year to 0 or 2000
  if (nowMon > 12){nowMon=1;}// If max month (December) is exceeded, loop to January
  if (nowMon < 1){nowMon=12;}// If min month (January) is exceeded, loop to December
  if (nowHr>23){nowHr=0;}// If hr is greater than 23, set to 0
  if (nowHr<0){nowHr=23;}// If hr is less than 0, set to 23
  if (nowMin>59){nowMin=0;}// If min is greater than 59, set to 0
  if (nowMin<0){nowMin=59;}// If min is less than 0, set to 59
  if (nowSec>59){nowSec=0;}// If sec is greater than 59, set to 0
  if (nowSec<0){nowSec=59;}// If sec is less than 0, set to 59
//Pump A Correct
  if (((aYr != 20)&&(aYr != 24)&&(aYr != 28)&&(aYr != 32)&&(aYr != 36)&&(aYr != 40)&&(aYr != 44))&&(aMon==2)&&(aDay > 28)){aDay=1;}// Sets max day in Feb for non leap years up to 2044, and loops to 1 if you exceed
  if (((aYr == 20)||(aYr == 24)||(aYr == 28)||(aYr == 32)||(aYr == 36)||(aYr == 40)||(aYr == 44))&&(aMon==2)&&(aDay > 29)){aDay=1;}// Sets max day in Feb for leap years up to 2044, and loops to 1 if you exceed
  if (((aMon == 4)||(aMon == 6)||(aMon == 9)||(aMon == 11))&&(aDay > 30)){aDay=1;}// Sets max days for April, June, September, November, and loops to 1 if you exceed
  if (((aMon == 1)||(aMon == 3)||(aMon == 5)||(aMon == 7)||(aMon == 8)||(aMon == 10)||(aMon == 12))&&(aDay > 31)){aDay=1;}// Sets max days for January, March, May, July, August, October, December, and loops to 1 if you exceed
  if (((aYr != 20)&&(aYr != 24)&&(aYr != 28)&&(aYr != 32)&&(aYr != 36)&&(aYr != 40)&&(aYr != 44))&&(aMon==2)&&(aDay < 1)){aDay=28;}// Sets max day in Feb for non leap years up to 2044, and loops to max if you go lower than 1
  if (((aYr == 20)||(aYr == 24)||(aYr == 28)||(aYr == 32)||(aYr == 36)||(aYr == 40)||(aYr == 44))&&(aMon==2)&&(aDay < 1)){aDay=29;}// Sets max day in Feb for leap years up to 2044, and loops to max if you go lower than 1
  if (((aMon == 4)||(aMon == 6)||(aMon == 9)||(aMon == 11))&&(aDay < 1)){aDay=30;}// Sets max days for April, June, September, November, and loops to max if you go lower than 1
  if (((aMon == 1)||(aMon == 3)||(aMon == 5)||(aMon == 7)||(aMon == 8)||(aMon == 10)||(aMon == 12))&&(aDay < 1)){aDay=31;}// Sets max days for January, March, May, July, August, October, and loops to max if you go lower than 1
  if (aYr < 0){aYr=0;}// Sets minimum year to 0 or 2000
  if (aMon > 12){aMon=1;}// If max month (December) is exceeded, loop to January
  if (aMon < 1){aMon=12;}// If min month (January) is exceeded, loop to December
  if (aHr>23){aHr=0;}// If hr is greater than 23, set to 0
  if (aHr<0){aHr=23;}// If hr is less than 0, set to 23
  if (aMin>59){aMin=0;}// If min is greater than 59, set to 0
  if (aMin<0){aMin=59;}// If min is less than 0, set to 59
//pumpB Correct
  if (((bYr != 20)&&(bYr != 24)&&(bYr != 28)&&(bYr != 32)&&(bYr != 36)&&(bYr != 40)&&(bYr != 44))&&(bMon==2)&&(bDay > 28)){bDay=1;}// Sets max day in Feb for non leap years up to 2044, and loops to 1 if you exceed
  if (((bYr == 20)||(bYr == 24)||(bYr == 28)||(bYr == 32)||(bYr == 36)||(bYr == 40)||(bYr == 44))&&(bMon==2)&&(bDay > 29)){bDay=1;}// Sets max day in Feb for leap years up to 2044, and loops to 1 if you exceed
  if (((bMon == 4)||(bMon == 6)||(bMon == 9)||(bMon == 11))&&(bDay > 30)){bDay=1;}// Sets max days for April, June, September, November, and loops to 1 if you exceed
  if (((bMon == 1)||(bMon == 3)||(bMon == 5)||(bMon == 7)||(bMon == 8)||(bMon == 10)||(bMon == 12))&&(bDay > 31)){bDay=1;}// Sets max days for January, March, May, July, August, October, December, and loops to 1 if you exceed
  if (((bYr != 20)&&(bYr != 24)&&(bYr != 28)&&(bYr != 32)&&(bYr != 36)&&(bYr != 40)&&(bYr != 44))&&(bMon==2)&&(bDay < 1)){bDay=28;}// Sets max day in Feb for non leap years up to 2044, and loops to max if you go lower than 1
  if (((bYr == 20)||(bYr == 24)||(bYr == 28)||(bYr == 32)||(bYr == 36)||(bYr == 40)||(bYr == 44))&&(bMon==2)&&(bDay < 1)){bDay=29;}// Sets max day in Feb for leap years up to 2044, and loops to max if you go lower than 1
  if (((bMon == 4)||(bMon == 6)||(bMon == 9)||(bMon == 11))&&(bDay < 1)){bDay=30;}// Sets max days for April, June, September, November, and loops to max if you go lower than 1
  if (((bMon == 1)||(bMon == 3)||(bMon == 5)||(bMon == 7)||(bMon == 8)||(bMon == 10)||(bMon == 12))&&(bDay < 1)){bDay=31;}// Sets max days for January, March, May, July, August, October, and loops to max if you go lower than 1
  if (bYr < 0){bYr=0;}// Sets minimum year to 0 or 2000
  if (bMon > 12){bMon=1;}// If max month (December) is exceeded, loop to January
  if (bMon < 1){bMon=12;}// If min month (January) is exceeded, loop to December
  if (bHr>23){bHr=0;}// If hr is greater than 23, set to 0
  if (bHr<0){bHr=23;}// If hr is less than 0, set to 23
  if (bMin>59){bMin=0;}// If min is greater than 59, set to 0
  if (bMin<0){bMin=59;}// If min is less than 0, set to 59
}


//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
//<--------------------------------------------------------------------TIME CODE--------------------------------------------------------------------------------->
//<-------------------------------------------------------------------------------------------------------------------------------------------------------------->
time_t getTeensy3Time(){// Time variable getTeensy3Time
  return Teensy3Clock.get();// Get the time from the Teensy clock
}

void calcUpdateSecTime(){
  tmElements_t updateNowTm;// This is a time elements variable named updateNowTm with each of the different elements as below
  updateNowTm.Hour = nowHr;// Makes the hour of updateNowTm equal to nowHr
  updateNowTm.Minute = nowMin;// Makes the minute of updateNowTm equal to nowMin
  updateNowTm.Second = nowSec;// Makes the second of updateNowTm equal to nowSec
  updateNowTm.Day = nowDay;// Makes the day of updateNowTm equal to nowDay
  updateNowTm.Month = nowMon;// Makes the month of updateNowTm equal to nowMon
  updateNowTm.Year = nowYr+30;// Makes the year of updateNowTm equal to nowYr + 30 (because time elements deal with seconds since 1970 and nowYr is from 2000)
  updateSecTime = makeTime(updateNowTm);// Compile all time elements into seconds since 1970 and name it updateSecTime
}

void getNow(){
  setSyncProvider(getTeensy3Time);// Get the time from the teensy NOW!
  nowSec = second();// Name seconds nowSec
  nowMin = minute();// Name minutes nowMin
  nowHr = hour();// Name hour nowHr
  nowDay = day();// Name day nowDay
  nowMon = month();// Name month nowMon
  nowYr = (year()-2000);// Name years since 2000 nowYr

  tmElements_t nowTm;//This is a time elements variable named nowTm with each of the different elements as below
  nowTm.Hour = nowHr;// Makes the hour of nowTm equal to nowHr
  nowTm.Minute = nowMin;// Makes the minute of nowTm equal to nowMin
  nowTm.Second = nowSec;// Makes the second of nowTm equal to nowSec
  nowTm.Day = nowDay;// Makes the day of nowTm equal to nowDay
  nowTm.Month = nowMon;// Makes the month of nowTm equal to nowMon
  nowTm.Year = nowYr+30;// Makes the year of nowTm equal to nowYr + 30 (because time elements deal with seconds since 1970 and nowYr is from 2000)
  nowSecTime = makeTime(nowTm);// Compile all time elements into seconds since 1970 and name it nowSecTime
}

void calculateInitialAlarmSecondTime(){
  tmElements_t aTm;//This is a time elements variable named aTm with each of the different elements as below
  aTm.Hour = aHr;// Makes the hour of aTm equal to aHr
  aTm.Minute = aMin;// Makes the minute of aTm equal to aMin
  aTm.Second = 0;// Makes the second of aTm equal to zero
  aTm.Day = aDay;// Makes the day of aTm equal to aDay
  aTm.Month = aMon;// Makes the month of aTm equal to aMon
  aTm.Year = aYr+30;// Makes the year of aTm equal to aYr + 30 (because time elements deal with seconds since 1970 and aYr is from 2000)
  aSecTime = makeTime(aTm);// Compile all time elements into seconds since 1970 and name it aSecTime
  
  tmElements_t bTm;//This is a time elements variable named bTm with each of the different elements as below
  bTm.Hour = bHr;// Makes the hour of bTm equal to bHr
  bTm.Minute = bMin;// Makes the minute of bTm equal to bMin
  bTm.Second = 0;// Makes the second of bTm equal to zero
  bTm.Day = bDay;// Makes the day of bTm equal to bDay
  bTm.Month = bMon;// Makes the month of bTm equal to bMon
  bTm.Year = bYr+30;// Makes the year of bTm equal to bYr + 30 (because time elements deal with seconds since 1970 and bYr is from 2000)
  bSecTime = makeTime(bTm);// Compile all time elements into seconds since 1970 and name it bSecTime
}

void serialDebug(){
  Serial.flush(); delay(200); 
  Serial.print("aSecTime: ");
  Serial.println(aSecTime);
  Serial.print("bSecTime: ");
  Serial.println(bSecTime);
  Serial.print("sleepSecTime: ");  
  Serial.println(sleepSecTime);
  Serial.print("hrToSleep: ");
  Serial.println(hrToSleep);
  Serial.print("minToSleep: ");      
  Serial.println(minToSleep);
  Serial.print("secToSleep: ");      
  Serial.println(secToSleep);  
  Serial.print("aAlarmFlag: ");      
  Serial.println(aAlarmFlag);
  Serial.print("bAlarmFlag: ");
  Serial.println(bAlarmFlag);    
  Serial.print("nowSecTime: ");
  Serial.println(nowSecTime);  
  Serial.print("Now: ");Serial.print(nowHr);Serial.print(":");Serial.print(nowMin); Serial.print(":");Serial.print(nowSec);Serial.print(" ");Serial.print(nowDay); Serial.print(":");Serial.print(nowMon); Serial.print(":");Serial.println(nowYr);
  Serial.print("Alarm A: ");Serial.print(aHr); Serial.print(":");Serial.print(aMin); Serial.print(" ");Serial.print(aDay); Serial.print(":");Serial.print(aMon); Serial.print(":");Serial.println(aYr);
  Serial.print("Alarm B: ");Serial.print(bHr); Serial.print(":");Serial.print(bMin); Serial.print(" ");Serial.print(bDay); Serial.print(":");Serial.print(bMon); Serial.print(":");Serial.println(bYr);
}
