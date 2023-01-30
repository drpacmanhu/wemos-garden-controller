#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <UrlEncode.h>

String relay8 = "OFF";
String relay7 = "OFF";
String relay6 = "OFF";
String relay5 = "OFF";
String irrigationStatus = "OFF";
String lastReceivedMessage = "None";
byte currentTankLevel = 0;
bool manualOverride = false;
//error indicator led
const byte errorIndicator = D7;
bool doWeHaveAnError = true;
//wifi settings
const char* ssid     = "<YOUR WIFI NAME>";
const char* password = "<YOUR WIFI PASSWORD>";
// network less mode
bool doWeHaveNetwork = false;
//the server address where we upload the data
const char* cloudServerAddress = "<YOUR CLOUD SERVER>";
//when true the unit sends data to the cloud server as well
bool sendDataToCloud = false;
// loopCounterForReportingToCloud we would like to report the status in every 10 minutes
int loopCounterForReportingToCloud = 10;
//wifi signal strength
int wifiSignalStrength = 0;
//version number
const String versionNumber = "2023-01-25";
//this is the OTA enable variables
ESP8266WebServer server;
//set this value by default to false as the OTA timeout is 5 minutes
bool ota_flag = false;
// flashActiveLed used to flash the led on the board to show activity
bool activeLedState = false;
// items for ntp update
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 3600;
NTPClient timeClient(ntpUDP, "hu.pool.ntp.org", utcOffsetInSeconds);
String lastRebootWas = "Unknown";
unsigned long oldTime;
unsigned long ledLampTimer;
/*
   Start OTA.
*/
void startOTA() {
  ArduinoOTA.setPassword("<YOUR WEMOS PASSWORD>");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.println("Error[%u]:");
    Serial.println(error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void sendToSerial(int command) {
  String message = "command:" + String(command);
  Serial.println(message);
}

//sends the command to the arduino using serial
//these are related to Pin numbers
void sendCommand(String command) {
  if (command == "relay8ON" || command == "relay8OFF") {
    //same as Off as this is an impulse switch
    sendToSerial(40);
  } else if(command == "relay7ON") {
    sendToSerial(50);
  } else if(command == "relay7OFF") {
    sendToSerial(55);
  } else if(command == "relay6ON") {
    sendToSerial(60);
  } else if(command == "relay6OFF") {
    sendToSerial(65);
  } else if(command == "relay5ON") {
    sendToSerial(70);
  } else if(command == "relay5OFF") {
    sendToSerial(75);
  } else if(command == "irrigationON") {
    sendToSerial(100);
  } else if(command == "irrigationOFF") {
    sendToSerial(105);
  }
}

//Incoming message will look like:
//tank status 1-4 25-50-75-100%
//relay 8-5
//relay 4-3 is empty - not included into message
//relay 2-1 is for ball valve
void setLocalVariablesBasedOnIncomingData(String value, byte index) {
  if (index == 0) {
    //this is the tank current level
    if (value == "1") {
      currentTankLevel = 25;
    } else if (value == "2") {
      currentTankLevel = 50;
    } else if (value == "3") {
      currentTankLevel = 75;
    } else if (value == "4") {
      currentTankLevel = 100;
    } else if (value == "-1") {
      //this is an error flow.
      currentTankLevel = -1;
    }
  } else if (index == 1) {
    if (value == "1") {
      irrigationStatus = "OFF";
    } else {
      irrigationStatus = "ON";
    }
  } else if (index == 2) {
    if (value == "1") {
      relay5 = "OFF";
    } else {
      relay5 = "ON";
    }    
  } else if (index == 3) {
    if (value == "1") {
      relay6 = "OFF";
    } else {
      relay6 = "ON";
    }
  } else if (index == 4) {
    if (value == "1") {
      relay7 = "OFF";
    } else {
      relay7 = "ON";
    }
  } else if (index == 5) {
    if (value == "1") {
      relay8 = "OFF";
    } else {
      relay8 = "ON";
    }
  } 
}

void parseMessage(String paramMessage) {
  lastReceivedMessage = "";
  byte processedValue = 0;
  if (paramMessage.indexOf(";") >= 1 && paramMessage.length() > 8) {
    while (paramMessage.indexOf(";") >= 1)
    {
      lastReceivedMessage = lastReceivedMessage + paramMessage.substring(0,paramMessage.indexOf(";"));
      setLocalVariablesBasedOnIncomingData(paramMessage.substring(0,paramMessage.indexOf(";")), processedValue);
      paramMessage = paramMessage.substring(paramMessage.indexOf(";")+1);
      processedValue++;
    }
  } else {
      //TODO send a warning message to the cloud to notify the user
  }
}

//send the getStatus command to Arduino then process it
void requestDataFromArduino() {
  //clearing the serial buffer
  if (Serial.available()) {
      Serial.readString();
  }
  sendToSerial(200);
  int loopCounter = 0;
  while (loopCounter <= 50) {
    if (Serial.available()) {
      String incomingMessage = Serial.readString();
      if (incomingMessage.length() > 5) {
        parseMessage(incomingMessage);
      }
      //existing loop
      loopCounter = 100;
    }
    delay(250);
    ESP.wdtFeed();
    loopCounter++;
  }
}
/*
   We use a webserver to set runtime action if needed.
*/
void initWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", getWebPage());
    delay(1000);
  });

  server.on("/relay8", []() {
    if (relay8 == "OFF") {
      sendCommand("relay8ON");
      relay8 = "ON";
    } else {
      sendCommand("relay8OFF");
      relay8 = "OFF";
    }
    server.send(200, "text/html", getRedirectWebPage());
  });

  server.on("/relay7", []() {
    if (relay7 == "OFF") {
      sendCommand("relay7ON");
      relay7 = "ON";
    } else {
      sendCommand("relay7OFF");
      relay7 = "OFF";
    }
    server.send(200, "text/html", getRedirectWebPage());
  });

  server.on("/relay6", []() {
    if (relay6 == "OFF") {
      sendCommand("relay6ON");
      relay6 = "ON";
      ledLampTimer = millis();
    } else {
      sendCommand("relay6OFF");
      relay6 = "OFF";
    }
    server.send(200, "text/html", getRedirectWebPage());
  });

  server.on("/relay5", []() {
    if (relay5 == "OFF") {
      sendCommand("relay5ON");
      relay5 = "ON";
    } else {
      sendCommand("relay5OFF");
      relay5 = "OFF";
    }
    server.send(200, "text/html", getRedirectWebPage());
  });  

  server.on("/irrigationButton", []() {
    if (irrigationStatus == "OFF") {
      sendCommand("irrigationON");
      irrigationStatus = "ON";
    } else {
      sendCommand("irrigationOFF");
      irrigationStatus = "OFF";
    }
    server.send(200, "text/html", getRedirectWebPage());
  });

  server.on("/getData", []() {
    requestDataFromArduino();
    server.send(200, "text/html", getRedirectWebPage());
  });  

  server.on("/manualOverride", []() {
    if (manualOverride) {
      manualOverride = false;
    } else {
      manualOverride = true;
    }
    server.send(200, "text/html", getRedirectWebPage());
  });

  server.on("/setflag", []() {
    server.send(200, "text/plain", "Setting flag...");
    ota_flag = true;
  });

  server.on("/restart", []() {
    server.send(200, "text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
  });

  server.on("/sendDataToCloud", []() {
    if (sendDataToCloud) {
      sendDataToCloud = false;
    } else {
      sendDataToCloud = true;
    }
    server.send(200, "text/html", getRedirectWebPage());
  });

  server.on("/help", []() {
    server.send(200, "text/plain", "Commands are: manualOverride / setflag / restart / receiveData / summerMode / sendDataToCloud");
  });

  server.begin();
}

String getWebPage() {
  String webPage = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  webPage = webPage + "<style>table, th, td {border: 1px solid black;}</style></head>";
  webPage = webPage + "<header><h1>Garden Controller</h1></header>";
  webPage = webPage + "<body>";
  webPage = webPage + "<table><tr><th>Action</th><th>Button</th></tr>";

  webPage = webPage + "<tr><td>Well - " + relay8 + "</td><td>";
  webPage = webPage + "<form method=\"post\" action=/relay8><input id=\"relay8\" type=\"submit\" value=\"submit\" style=\"width:100%\"></form>";
  webPage = webPage + "</td></tr>";

  webPage = webPage + "<tr><td>Pump - " + relay7 + "</td><td>";
  webPage = webPage + "<form method=\"post\" action=/relay7><input id=\"relay7\" type=\"submit\" value=\"submit\" style=\"width:100%\"></form>";
  webPage = webPage + "</td></tr>";

  webPage = webPage + "<tr><td>Led Lamp - " + relay6 + "</td><td>";
  webPage = webPage + "<form method=\"post\" action=/relay6><input id=\"relay6\" type=\"submit\" value=\"submit\" style=\"width:100%\"></form>";
  webPage = webPage + "</td></tr>";

  webPage = webPage + "<tr><td>Empty - " + relay5 + "</td><td>";
  webPage = webPage + "<form method=\"post\" action=/relay5><input id=\"relay5\" type=\"submit\" value=\"submit\" style=\"width:100%\"></form>";
  webPage = webPage + "</td></tr>";

  webPage = webPage + "<tr><td>Irrigation - " + irrigationStatus + "</td><td>";
  webPage = webPage + "<form method=\"post\" action=/irrigationButton><input id=\"irrigationButton\" type=\"submit\" value=\"submit\" style=\"width:100%\"></form>";
  webPage = webPage + "</td></tr>";

  webPage = webPage + "<tr><td>GetData</td><td>";
  webPage = webPage + "<form method=\"post\" action=/getData><input id=\"getData\" type=\"submit\" value=\"submit\" style=\"width:100%\"></form>";
  webPage = webPage + "</td></tr>";

  webPage = webPage + "<tr><td>Last Rec. Msg.</td><td>";
  webPage = webPage + lastReceivedMessage;
  webPage = webPage + "</td></tr>";

  webPage = webPage + "<tr><td>Tank status</td><td>" + String(currentTankLevel) + "</td></tr>";

  webPage = webPage + "<tr><td>Send data to cloud</td><td>";
  if (sendDataToCloud) {
    webPage = webPage + "ON*";
  } else {
    webPage = webPage + "OFF*";
  }
  webPage = webPage + "</td></tr>";

  webPage = webPage + "<tr><td>Wifi Signal : </td><td>" + getWifiSignalStrenght() + "</td></tr>";
  webPage = webPage + "<tr><td>Version Number:</td><td>" + versionNumber + "</td></tr>";
  webPage = webPage + "<tr><td>Manual override:</td><td>" + manualOverride + "</td></tr>";
  webPage = webPage + "</table>";
  webPage = webPage + "Reset reason was : " + ESP.getResetReason() + "</br>";
  webPage = webPage + "Last reboot was at: " + lastRebootWas;
  webPage = webPage + "</br>* to change this value use the direct link";
  webPage = webPage + "</body></html>";
  
  return webPage;
}

String getWifiSignalStrenght() {
  String signal = "None";
  if (wifiSignalStrength <= 65 ) {
    signal = "| | | |";
  } else if (wifiSignalStrength > 65 and wifiSignalStrength <= 70) {
    signal = "| | |";
  } else if (wifiSignalStrength > 70 and wifiSignalStrength <= 75) {
    signal = "| |";
  } else {
    signal = "|";
  }
  signal = signal + " : " + wifiSignalStrength;
  return signal;
}

String getRedirectWebPage() {
  String webPage = "<html><head><meta http-equiv = \"refresh\" content=\"2; url=http://" + WiFi.localIP().toString() + "/\"></head>";
  webPage = webPage + "<body><p style=\"font-size:80px\"/>Working...</p></body></html>";
  return webPage;
}

void connectToWifi() {
  doWeHaveNetwork = false;
  wifiSignalStrength = -99;
  WiFi.hostname("WemosPro-GardenController");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int connectionLoop = 0;
  //Serial.println("Trying to connect to wifi...");
  //if we cannot connect to wifi within watchdog period restart the board
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    //Serial.println("Connecting to WIFI...");
    connectionLoop ++;
    if (connectionLoop > 30) {
      Serial.println("Unable to connect to WIFI network...");
      doWeHaveAnError = true;
      break;
      //ESP.restart("Wifi signal was too low or unavailable!");
    }
    flashActiveLed();
  }
  if (WiFi.status() == WL_CONNECTED) {
    doWeHaveNetwork = true;
    wifiSignalStrength = WiFi.RSSI();
    doWeHaveAnError = false;
  }
}

void uploadDataToServer() {
  //if we do not have network we quit from this method
  if (doWeHaveNetwork == false) {
    return;
  }
  String Link = "/wemos_garden_controller/reportStatus.php";
  const int httpsPort = 443;
  //Declare object of class WiFiClient
  WiFiClientSecure httpsClient;

  httpsClient.setInsecure();
  //Serial.print("Trying to connect to : ");
  //Serial.println(cloudServerAddress);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);
  
  //Serial.println("Establishing connection...");
  int r=0; //retry counter
  while((!httpsClient.connect(cloudServerAddress, httpsPort)) && (r < 30)){
      //Serial.print(".");
      r++;
      ESP.wdtFeed();
      delay(100);
      flashActiveLed();
  }
  if(r==30) {
    //push the error to the variable to user can see it.
    lastReceivedMessage = "Connection failed! Unable to send data...";
    //Serial.print("Connection failed! Unable to send data...");
    return;
  }
  //Serial.print("requesting URL: ");
  //Serial.println(cloudServerAddress);

  //this value here is a secret key leave it as it is
  String messageContent = "{\"currentTankLevel\":\"" + String(currentTankLevel) + "\"";
  messageContent = messageContent + "\"irrigationStatus\":\"" + irrigationStatus + "\"";
  messageContent = messageContent + "\"relay5\":\"" + relay5 + "\"";
  messageContent = messageContent + "\"relay6\":\"" + relay6 + "\"";
  messageContent = messageContent + "\"relay7\":\"" + relay7 + "\"";
  messageContent = messageContent + "\"relay8\":\"" + relay8 + "\"";  
  messageContent = messageContent + "\"lastReceivedMessage\":\"" + lastReceivedMessage + "\"}";  

  String postMessageContent = "message=" + urlEncode(messageContent);
  //Serial.println("Postmessage will be : " + postMessageContent);
  //Serial.print("postMessage length : ");
  //Serial.println(postMessageContent.length());

  httpsClient.println("POST " + String(Link) + " HTTP/1.1");
  httpsClient.println("Host: " + String(cloudServerAddress));
  httpsClient.println("Content-Type: application/x-www-form-urlencoded");
  httpsClient.println("Content-Length: " + String(postMessageContent.length()));
  httpsClient.println("sendDataToServer: true");
  httpsClient.println("");
  httpsClient.println(postMessageContent);
  httpsClient.flush();

  //Serial.println("waiting for reply");
  while (httpsClient.connected()) {  
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r") {
      //Serial.println("headers received");
      break;
    }
    //Serial.print(".");
    ESP.wdtFeed();
    delay(100);
    flashActiveLed();
  }

  //Serial.println("reply was:");
  //Serial.println("==========");
  String line;
  while(httpsClient.available()){        
    line = httpsClient.readStringUntil('\n');  //Read Line by Line
    //Serial.println(line); //Print response
  }
  //Serial.println("==========");
  //Serial.println("closing connection");
  httpsClient.stop();
}

void flashActiveLed() {
  if (activeLedState) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  if (doWeHaveAnError) {
    digitalWrite(errorIndicator, LOW);
  } else {
    digitalWrite(errorIndicator, HIGH);
  }
  activeLedState = !activeLedState;
  ESP.wdtFeed();
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  //Serial.println("Setup started...");

  ESP.wdtDisable();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(errorIndicator, OUTPUT);
  //Set the relays to default state
  //put these delays here to minimize the electronic noise from the relay board
  //experienced several power spikes which could cause serious issues
  ESP.wdtFeed();

  flashActiveLed();
  //start wifi connection
  //Serial.println("Checking for wifi network...");
  connectToWifi();
  if (doWeHaveNetwork) {
    //Serial.println("Ready");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    //Serial.println("MAC address: ");
    //Serial.println(WiFi.macAddress());
    //Serial.println("Starting OTA listener...");
    startOTA();
    //Serial.println("Starting webserver...");
    initWebServer();
    //Serial.println("Getting restart time...");

    timeClient.begin();
    timeClient.update();
    long milliseconds = timeClient.getEpochTime();
    //Serial.print("Received milliseconds from NTP server : ");
    //Serial.println(milliseconds);
    int ntpLoopCounter = 0;
    while (ntpLoopCounter < 11 && milliseconds < 10000) {
      timeClient.update();
      long milliseconds = timeClient.getEpochTime();
      //Serial.println("Waiting for new time value...");
      ESP.wdtFeed();
      delay(1000);
      ntpLoopCounter++;
    }
    setTime(milliseconds);
    /*milliseconds = NULL;*/
    timeClient.end();
    
    lastRebootWas = String(year()) + "-" + String(month()) + "-" + String(day()) + ":" + String(hour()) +":"+ String(minute()) +":" + String(second());
    //Serial.println("lastRebootWas: " + lastRebootWas);
  } else {
    Serial.println("We are in network less mode...");
  }
}

/*
   The main loop collects the data, send it to the server then set the relay based on the given value.
*/
void loop() {
  //if we are not connected trying to reconnect.
  if (WiFi.status() != WL_CONNECTED) {
      connectToWifi();
  }

  //we are initiated a FOTA update
  if (ota_flag) {
    ota_flag = false;
    int loopCounter = 0;
    Serial.println("Waiting for OTA connection for 120 seconds...");
    while (loopCounter < 600)
    {
      loopCounter++;
      ArduinoOTA.handle();
      delay(200);
    }
  }  

  for (int waitInMainLoop = 0; waitInMainLoop <= 4; waitInMainLoop++) {
    if (doWeHaveNetwork) {
      server.handleClient();
      ESP.wdtFeed();
      delay(500);
    }
  }

  if (doWeHaveNetwork && sendDataToCloud) {
    //we only deal with the counter if this is true
    //making sure that the counter will not overflow
    if (loopCounterForReportingToCloud >= 10) {
      loopCounterForReportingToCloud = 0;
      uploadDataToServer();
    } else {
      //Serial.print("loopCounterForReportingToCloud is : ");
      //Serial.println(loopCounterForReportingToCloud);
      loopCounterForReportingToCloud++;
    }
  }

  //we check for automation events in every two minutes.
  if((millis() - oldTime) > 120000) {
    //this means we have an error we shold switch the well off
    if (currentTankLevel == -1) {
      sendCommand("relay8OFF");
    }
    //switch on / off the submersed pump
    //we switch on the pump between 1 and 2 AM
    if (hour() > 1 && hour() < 2 ) {
      sendCommand("relay7ON");
    } else {
      sendCommand("relay7OFF");
    }
    //if led lamp is on for 5 minutes switch it off
    if (ledLampTimer > 300000) {
      sendCommand("relay6OFF");
    }
    oldTime = millis();   
  }

  if (Serial.available()) {
    String inputFromSerial = Serial.readString();
    parseMessage(inputFromSerial);
  }

  flashActiveLed();
}
