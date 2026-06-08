#include <SoftwareSerial.h>

//well relay
const byte relay8 = 4;
//submersible pump relay
const byte relay7 = 5;
//led reflectorfloatSwitch
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
//byte relay1 = 11;
const byte floatSwitchInputPin = A0;

bool irrigationActive = false;
bool ledState = false;
//if true the tank is full
bool floatSwitchStatus = true;
unsigned long oldTime;
unsigned long wellLastONTime;

void setup()
{
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);
  pinMode(builtInLed, OUTPUT);

  // Set pin to default state
  pinMode(floatSwitchInputPin, INPUT_PULLUP); 

  delay(2500);

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
  oldTime = 0;
  wellLastONTime = 0;

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

//we received a command from the master
//these are related to Pin numbers
void executeCommand(int command)
{
  if (command == 40) {
    digitalWrite(relay8, LOW);
  } else if(command == 45) {
      digitalWrite(relay8, HIGH);
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
//tank status 
//relay 8-5
//relay 4-3 is empty - not included into message
//relay 2-1 is for ball valve
void sendStatusUpdate()
{
  String message = "response:";
  message = message + String(digitalReadOutputPin(floatSwitchInputPin)) + ";";
  if (irrigationActive) {
    message = message + "0;";
  } else {
    message = message + "1;";
  }
  message = message + String(digitalReadOutputPin(relay5)) + ";";
  message = message + String(digitalReadOutputPin(relay6)) + ";";
  message = message + String(digitalReadOutputPin(relay7)) + ";";
  message = message + String(digitalReadOutputPin(relay8)) + ";";
  Serial.println(message);
}

void sendDebugMessage()
{
  String debugMessage = "debugMessage:";
  debugMessage += "isWellActive=" + String(digitalReadOutputPin(relay8)) + ";";
  debugMessage += "isPumpActive=" + String(digitalReadOutputPin(relay7)) + ";";
  debugMessage += "irrigationActive=" + String(irrigationActive) + ";";
  debugMessage += "floatSwitchStatus=" + String(floatSwitchStatus) + ";";
  debugMessage += "versionNumber=2026-06-01";
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

  floatSwitchStatus = digitalReadOutputPin(floatSwitchInputPin);

  //water comes from the well in every 1 minutes in every 10 minutes.
  //we need to switch on and off the relay making sure it wont worn out quickly.


  if((millis() - oldTime) > 90000) {
    //if the well relay is ON
    if (digitalReadOutputPin(relay8) == 0) {
      //Serial.println("The 90s period has passed, Switching Well OFF.");
      //this means we need to switch OFF the well
      executeCommand(45);
      //if we do not have enough water in the tank
    }
    if (floatSwitchStatus) {
        //if we passed the 10 minutes marker we can switch on the well
        if ((millis() - wellLastONTime) > 600000) {
          //Serial.println("The tank is not full and we are not in the wait windows. Switching Well ON.");
          executeCommand(40);
          wellLastONTime = millis();
        }
    } else {
          //Serial.print("The tank is full, floatSwitchStatus is:");
          //Serial.println(floatSwitchStatus);
    }
    oldTime = millis();        
  }
  setLedState();
}
