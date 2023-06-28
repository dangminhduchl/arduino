#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Redis.h>

WiFiClient espClient;
WiFiClient redisConn;

Redis *gRedis = nullptr;
PubSubClient client(espClient);

const char* ssid = "Duc";
const char* password = "1234567789";
const char* mqttServer = "54.252.131.125";
const int mqttPort = 1883;
const char* mqttClientID = "mqttClient";

const int magnet_switch = 5; // Magnet switch
const int magnetLED = 14;
const int lock = 4;

long currentTime = 0;
int door_status = null;
int lock_status = null;

void setup() {
  Serial.begin(19200);
  pinMode(magnet_switch, INPUT_PULLUP);
  pinMode(magnetLED, OUTPUT);
  pinMode(lock, OUTPUT);
  digitalWrite(magnetLED, LOW);
  digitalWrite(lock, LOW);
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  String publishpayload = "{\"lock\" : 0, \"door\" : 0}";
  client.publish("status",(const uint8_t*)publishpayload.c_str(), publishpayload.length(), false);
  if (!redisConn.connect("127.0.0.1", "6379")) {
    return;
  }

  gRedis = Redis(redisConn);
  


}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (client.connect(mqttClientID)) {
      Serial.println("Connected to MQTT broker");
      // Subscribe to the control topic
      client.subscribe("control");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received message on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Handle control commands
  if (strcmp(topic, "control") == 0) {
    Serial.println("Received message from control topic");
    
    // Create a char array to store the message
    char message[length + 1]; // +1 for null-terminator
    strncpy(message, (char*)payload, length); // Copy payload to message array
    message[length] = '\0'; // Add null-terminator at the end
    
    String receivedMessage = String(message);
    Serial.println("MESSAGE:\n");
    Serial.println(receivedMessage);
    Serial.println("end:\n");
    
    if (receivedMessage == "1") {
      lock_status = 1;
      digitalWrite(lock, HIGH);
      Serial.println("Lock engaged");
    } else if (receivedMessage == "0") {
      lock_status = 0;
      digitalWrite(lock, LOW);
      Serial.println("Lock disengaged");
    }
  }
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();
  
  currentTime = millis();
  if (door_status == null && lock_status == null) {

  }
  
  int new_door_status = digitalRead(magnet_switch);
  int new_lock_status = digitalRead(lock);
  
  // Publish the lock and door status to the MQTT server
  if (new_door_status != door_status || new_lock_status != lock_status) {
    door_status = new_door_status;
    lock_status = new_lock_status;
    String publishpayload = "{ \"lock\": " + String(lock_status) + ", \"door\": " + String(door_status) + " }";
    client.publish("status",(const uint8_t*)publishpayload.c_str(), publishpayload.length(), false);
  }

  digitalWrite(magnetLED, HIGH);
  delay(1000);
}
