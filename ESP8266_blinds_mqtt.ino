#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Stepper2.h>

// Update these with values suitable for your network.
const char* ssid = "xxxx;
const char* password = "xxxx";
const char* mqtt_server = "xxxxx";

const int rpm = 15; // max rpm on 28BYJ-48 is ~15
int pinOut[4] = { 14, 12, 13, 15 };
Stepper2 myStepper(pinOut);

WiFiClient espClient;
PubSubClient client(espClient);

int maxStep = 4;
int goToStep;
int curLevel;
int tempPos;
int curStep;
int mqttLevel;
char msg[3];

void setup_wifi() {
   delay(100);
  // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Command from MQTT broker is : [");
  Serial.print(topic);
  Serial.println("]");
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  String mytopic(topic);
  if (mytopic == "blinds/level/state") {
    mqttLevel = message.toInt();
    curStep = map (mqttLevel, 0, 100, 0, maxStep);
    Serial.println(curStep);
  } else if (mytopic == "blinds/cmd" && message.equalsIgnoreCase("on")) {
    Serial.print("Uzuolaidas atidarom!" );
    Serial.println("curStep:"+curStep);
    myStepper.setDirection(0); // clock-wise
    for (int i=curStep; i < maxStep; i++){
         myStepper.turn(); // one full turn
         curStep++;
    }
    myStepper.stop();
    client.publish("blinds/cmd/state", "on", true);
    client.publish("blinds/level/state", "0", true);
    Serial.println("Uzuolaidos atidarytos!" );
  } else if (mytopic == "blinds/cmd" && message.equalsIgnoreCase("off")) {
    Serial.print("Uzuolaidas uzdarom!" );
    myStepper.setDirection(1); // clock-wise
    for (int k=curStep; k < maxStep; k++){
         myStepper.turn(); // one full turn
         curStep--;
    }
    myStepper.stop();
    client.publish("blinds/cmd/state", "off", true);
    client.publish("blinds/level/state", "100", true);
    Serial.println("Uzuolaidos uzdarytos!" );
  }else if(mytopic == "blinds/level"){
    if(message.toInt() >= 0 && message.toInt() <= 100){
      Serial.println("into loop");
      mqttLevel = message.toInt();
      goToStep = map (mqttLevel, 0, 100, 0, maxStep);
      Serial.println("goToStep "+String(goToStep)+", curStep " +String(curStep));
      if(goToStep >= curStep ) {
        myStepper.setDirection(1); // clock-wise
        for (tempPos = curStep; tempPos < goToStep; tempPos++) {
          myStepper.turn(); // one full turn
          curStep++;
        }
      }else{
        myStepper.setDirection(0);
        for (tempPos = curStep; tempPos > goToStep; tempPos--) {
          myStepper.turn(); // one full turn
          curStep--;
        }
      }
    }
    mqttLevel = map (curStep, 0, maxStep, 0, 100);
    sprintf(msg, "%ld", mqttLevel);
    client.publish("blinds/level/state", msg, true);
  }
  
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Uzuolaidos-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
      client.subscribe("blinds/level/state");
      client.subscribe("blinds/cmd/state");
      client.subscribe("blinds/level");
      client.subscribe("blinds/cmd");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(6000);
    }
  }
} //end reconnect()

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  myStepper.setSpeed(rpm);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
