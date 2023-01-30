//well
byte relay8 = 4;
//submersible pump
byte relay7 = 5;
//led reflector
byte relay6 = 6;
//empty
byte relay5 = 7;
//empty
byte relay4 = 8;
//empty
byte relay3 = 9;
//12V ball vale - bidirectional movement
byte relay2 = 10;
//12V DC to DC converter
byte relay1 = 11;

byte quarterTankLevel = A1;
byte halfTankLevel = A2;
byte quaterToFullTankLevel = A3;
byte fullTankLevel = A4;

// to detect the signal from the flow detector
byte sensorInterrupt = 0;
byte sensorPin       = 2                                                              ;

bool isWellIdle = true;
bool isWellActive = false;
bool irrigationActive = false;
volatile byte pulseCount;  
unsigned long oldTime;

void setup()
{
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);
     
  pinMode(sensorInterrupt, INPUT);
  digitalWrite(sensorInterrupt, HIGH);

  pinMode(quarterTankLevel, INPUT_PULLUP);
  pinMode(halfTankLevel, INPUT_PULLUP);
  pinMode(quaterToFullTankLevel, INPUT_PULLUP);
  pinMode(fullTankLevel, INPUT_PULLUP);

  pinMode(relay1, OUTPUT);
  digitalWrite(relay1, HIGH);
  delay(250);
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

int readWaterTankLevel() {
  //We check two levels at the same time to make sure there is no malfunction
  //The output pins were pulled in the setup section. They will be pulled up via a 10k
  //resistor so when the water allows electricity to flow their value will be 1.
  //We need to check the system after it went live due to limescale can ruine the detection.
  if (analogRead(fullTankLevel) <= 500 && analogRead(quaterToFullTankLevel) <= 500) {
    //we are full
    return 4;
  } else if (analogRead(quaterToFullTankLevel) <= 500 && analogRead(halfTankLevel) <= 500) {
    return 3;
  } else if (analogRead(halfTankLevel) <= 500 && analogRead(quarterTankLevel) <= 500) {
    return 2;
  } else if (analogRead(quarterTankLevel) <= 500) {
    return 1;
  } else {
    //this should not happen                                                                                                                                                                                                                                                                
    return -1;
  }  
}
//Message will look like:
//tank status 1-4 25-50-75-100%
//relay 8-5
//relay 4-3 is empty - not included into message
//relay 2-1 is for ball valve
void sendStatusUpdate()
{
  String message = "response:";
  message = message + String(readWaterTankLevel()) + ";"; 
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
    message = message + "1;";
  } else {
    message = message + "0;";
  }
  Serial.println(message);
}

/**
 * Main program loop
 */
void loop()
{
  if (Serial.available()){
    String incoming = Serial.readString();
    //Serial.print("Serial incoming was : ");
    //Serial.print();
    if (incoming.indexOf("command:") > -1) {
      String commandNumber = incoming.substring(incoming.indexOf(":") +1 );
      executeCommand(commandNumber.toInt());
    } else {
      Serial.println("not valid command!");
    }
  }

  //count the pulses for two minutes making sure that the well is in idle mode
  if((millis() - oldTime) > 120000) {
    detachInterrupt(sensorInterrupt);
    if (pulseCount == 0) {
      //Serial.println("well IS idle ...");
      isWellIdle = true;
    } else {
      //Serial.println("well IS NOT idle ...");
      isWellIdle = false;
      isWellActive = true;
    }
    oldTime = millis();   

    //the resevoir is full and the well is idle we need to switch it off.
    if (readWaterTankLevel() == 4 && isWellIdle && isWellActive) {
      //this means we need to switch off the well
      executeCommand(40);
    }
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
  //Serial.println("pulseCount : " + String(pulseCount));
  delay(1000);
}

/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
