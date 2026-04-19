#include <SoftwareSerial.h>

//well pulse relay
const byte relay8 = 4;
//submersible pump relay
const byte relay7 = 5;
//led reflector
const byte relay6 = 6;
//hidro
const byte relay5 = 7;
//empty
const byte relay4 = 8;
//12V ball vale - bidirectional movement
const byte relay3 = 9;
//12V DC to DC converter
const byte relay2 = 10;
//dead
//byte relay1 = 11;

const byte builtInLed = 13;

//ultrasonic sensor pins
const byte powerPin = A0;
unsigned char data[4]={};
int distanceInMillimeter = -1;
SoftwareSerial mySerial(12, 11);

// to detect the signal from the flow detector
const byte sensorInterrupt = 0;
const byte sensorPin       = 2;

bool isWellIdle = true;
bool isWellActive = false;
bool isPumpActive = false;
bool irrigationActive = false;
bool ledState = false;
volatile byte pulseCount;
int totalPulseCountInPeriod = 0;  
unsigned long oldTime;
//it starts with value 4 to make sure that well will not be switched on immediatelly
byte tankLevel = 4;
//version number
//const String versionNumber = "2025-04-22";
/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}

void setup()
{
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);
  pinMode(sensorInterrupt, INPUT);
  digitalWrite(sensorInterrupt, HIGH);

  pinMode(builtInLed, OUTPUT);

  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin, LOW);
  delay(2500);

  mySerial.begin(9600);

  /*pinMode(relay1, OUTPUT);
  digitalWrite(relay1, HIGH);
  delay(250);*/
  pinMode(relay2, OUTPUT);
  digitalWrite(relay2, HIGH);
  delay(250);
  pinMode(relay3, OUTPUT);
  digitalWrite(relay3, HIGH);
  delay(250);
  pinMode(relay4, OUTPUT);
  digitalWrite(relay4, HIGH);
  delay(250);
  pinMode(relay5, OUTPUT);
  digitalWrite(relay5, HIGH);
  delay(250);
  pinMode(relay6, OUTPUT);
  digitalWrite(relay6, HIGH);
  delay(250);
  pinMode(relay7, OUTPUT);
  digitalWrite(relay7, HIGH);
  delay(250);
  pinMode(relay8, OUTPUT);
  digitalWrite(relay8, HIGH);
  //set the default value for variable.
  pulseCount        = 0;
  //set the default value for variable.
  oldTime           = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  // close the valve to make sure its in corrrectly known position
  executeCommand(105);
}

//gives back the actual state of the pin so we do not need to store it
int digitalReadOutputPin(byte pin)
{
  if (digitalRead(pin) == 1)
    return 1;
  else 
    return 0;
}

void changeWellState(){
  //we can only change the state if the well is inactive to avoid sparks
  if(isWellIdle) {
    //well is controlled with a pulse switch. same pulse for on or off
    digitalWrite(relay8, LOW);
    delay(500);
    digitalWrite(relay8, HIGH);
    //invert the state of the well variable
    isWellActive = !isWellActive;
  }
}

//we received a command from the master
//these are related to Pin numbers
void executeCommand(int command)
{
  if (command == 40) {
    changeWellState();
  } else if(command == 50) {
      digitalWrite(relay7, LOW);
  } else if(command == 55) {
      digitalWrite(relay7, HIGH);
  } else if(command == 60) {
      digitalWrite(relay6, LOW);
  } else if(command == 65) {
      digitalWrite(relay6, HIGH);
  } else if(command == 70) {
      digitalWrite(relay5, LOW);
  } else if(command == 75) {
      digitalWrite(relay5, HIGH);
  } else if(command == 80) {
      digitalWrite(relay4, LOW);
  } else if(command == 85) {
      digitalWrite(relay4, HIGH);
  } else if(command == 90) {
      digitalWrite(relay3, LOW);
  } else if(command == 95) {
      digitalWrite(relay3, HIGH);
  } else if(command == 100) {
      activateBallValve(true);
      //digitalWrite(relay2, LOW);
  } else if(command == 105) {
      activateBallValve(false);
      //digitalWrite(relay2, HIGH);
  } else if(command == 200) {
      //increase trigger value
      //currently does nothing
  } else if(command == 210) {
      //decrease trigger value
      //currently does nothing
  } else if(command == 999) {
      sendDebugMessage();
  }
  //in all other cases or after all execution update the master
  sendStatusUpdate();
}
// if direction is true we will open the valve and close otherwise
void activateBallValve(bool direction) {
  if (direction) {
    irrigationActive = true;
    digitalWrite(relay2, LOW);
    delay(5000);
  } else {
    irrigationActive = false;
    digitalWrite(relay3, LOW);
    delay(1000);
    digitalWrite(relay2, LOW);
    delay(5000);
  }
  //switching off the 12V before releasing the direction relay
  digitalWrite(relay2, HIGH);
  delay(5000);
  digitalWrite(relay3, HIGH);
}

//Message will look like:
//tank status 1-4 25-50-75-100%
//relay 8-5
//relay 4-3 is empty - not included into message
//relay 2-1 is for ball valve
void sendStatusUpdate()
{
  String message = "response:";
  message = message + String(tankLevel) + ";"; 
  if (irrigationActive) {
    message = message + "0;";
  } else {
    message = message + "1;";
  }
  if (digitalReadOutputPin(relay5) == 1 ) {
    message = message + "1;";
  } else {
    message = message + "0;";
  }
  if (digitalReadOutputPin(relay6) == 1 ) {
    message = message + "1;";
  } else {
    message = message + "0;";
  }
  if (digitalReadOutputPin(relay7) == 1 ) {
    message = message + "1;";
  } else {
    message = message + "0;";
  }
  if (isWellActive) {
    message = message + "0;";
  } else {
    message = message + "1;";
  }
  Serial.println(message);
}

void sendDebugMessage()
{
  String debugMessage = "debugMessage:";
  debugMessage += "isWellIdle=" + String(isWellIdle) + ";";
  debugMessage += "isWellActive=" + String(isWellActive) + ";";
  debugMessage += "isPumpActive=" + String(isPumpActive) + ";";
  debugMessage += "irrigationActive=" + String(irrigationActive) + ";";
  debugMessage += "tankLevel=" + String(tankLevel) + ";";
  debugMessage += "totalPulseCountInPeriod=" + String(totalPulseCountInPeriod) + ";";
  debugMessage += "pulseCount=" + String(pulseCount) + ";";
  debugMessage += "distanceInMillimeter=" + String(distanceInMillimeter) + ";";
  Serial.println(debugMessage);
}

void sendDebugInformationMessage(String paramMessage)
{
  String debugMessage = "debugMessage:" + paramMessage;
  Serial.println(debugMessage);
}

void setLedState() {
  if (ledState) {
    digitalWrite(builtInLed, HIGH);
    //Serial.println("Led high");
    ledState = false;
  } else {
    digitalWrite(builtInLed, LOW);
    //Serial.println("Led low");
    ledState = true;
  }
  delay(1000);
}

void doCalculation() {
  float distance;
  if(data[0]==0xff)
    {
      int sum;
      sum = (data[0]+data[1]+data[2])&0x00FF;
      if(sum == data[3])
      {
        distance=(data[1]<<8)+data[2];
        distanceInMillimeter = distance;
        if(distance > 30) {
            if (distance <= 200) {
              tankLevel = 4;
            } else if (distance <= 400) {
              tankLevel = 3;
            } else if (distance <= 600) {
              tankLevel = 2;
            } else if (distance <= 800) {
              tankLevel = 1;
            }
          //Serial.print("distance[/10 for centimeter]=");
          //Serial.println(distance);
          } else {
              tankLevel = 0;
              //Serial.println("Below the lower limit");
          }
      } else {
        //Serial.println("ERROR! Unable to read water level sensor!");
      }
    }
}

void emptySerialBuffer() {
  while (mySerial.available()) {
    mySerial.read();
  }
}

void changeCommStatus() {
  //Serial.println("Powering ON ultrasonic sensor.");
  digitalWrite(powerPin, HIGH);

  for(int w=0; w<=20000; w++) {
    delay(150);
    //read the tanklevel
    if (mySerial.available()) {
      if (mySerial.read()==0xFF) {
        data[0] = 0xFF;
        for(int i=1;i<4;i++) {
          data[i] = mySerial.read();
        }
        doCalculation();
        memset(data, 0, sizeof(data));
      }
    }
    w+= 150;
  }

  //Serial.println("Powering OFF ultrasonic sensor.");
  digitalWrite(powerPin, LOW);
  emptySerialBuffer();
}

/**
 * Main program loop
 */
void loop()
{
  if (Serial.available()){
    String incoming = Serial.readString();
    //Serial.print();
    if (incoming.indexOf("command:") > -1) {
      String commandNumber = incoming.substring(incoming.indexOf(":") +1 );
      executeCommand(commandNumber.toInt());
    } else {
      Serial.print("Incoming command:");
      Serial.print(incoming);
      Serial.println("-is not valid command!");
    }
  }

  //Serial.println("After serial check...");
  if(pulseCount >= 20) {
    totalPulseCountInPeriod = totalPulseCountInPeriod + pulseCount;
    pulseCount = 0;
    oldTime = millis();
  }
  //Serial.println("After pulseCount check...");
  if (totalPulseCountInPeriod >= 500) {
      //Serial.println("Pulse count > 0 setting well variables...");
      isWellIdle = false;
      isWellActive = true;
  }

  //count the pulses for two minutes making sure that the well is in idle mode 90000
  if((millis() - oldTime) > 90000) {
    changeCommStatus();
    detachInterrupt(sensorInterrupt);
    if (totalPulseCountInPeriod <= 100) {
      //Serial.println("well IS idle ...");
      isWellIdle = true;
    }
    oldTime = millis();   

    //the resevoir is full or we have an error and the well is idle but active we need to switch it off.
    if (tankLevel == 4 || tankLevel == 0) {
      if (isWellIdle && isWellActive) {
        //this means we need to switch OFF the well
        executeCommand(40);
      }
    } else {
      if (isWellIdle && !isWellActive) {
        //this means we need to switch ON the well
        executeCommand(40);
      }        
    }
    // Reset the pulse counter so we can start incrementing again
    totalPulseCountInPeriod = 0;
    attachInterrupt(sensorInterrupt, pulseCounter, CHANGE);
  }
  //this means we are at 1/4
  if (tankLevel < 2) {
    //we need to switch off the hidrofor
    digitalWrite(relay5, HIGH);
    //we need to switch off the irrigation
    if(irrigationActive){   
      sendDebugInformationMessage("Water tanklevel is less then 2, switching irrigation off!");
      sendDebugMessage();
      executeCommand(105);
    }
  }
  setLedState();
}
