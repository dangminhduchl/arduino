#include <ESP8266WiFi.h>
#include <Redis.h>

#define MAX_BACKOFF 300000


WiFiClient espClient;
WiFiClient pubSubWiFiClient;

Redis redis(espClient);
Redis pub_redis(pubSubWiFiClient);

const char* ssid = "Duc";
const char* password = "1234567789";
const char* redisServer = "3.27.65.231";
const int redisPort = 6379;

const int magnet_switch = 5;  // Magnet switch
const int magnetLED = 14;
const int lock = 4;

long currentTime = 0;
int door_status = 0;
int lock_status = 0;

void setup() {
  Serial.begin(19200);
  pinMode(magnet_switch, INPUT_PULLUP);
  pinMode(magnetLED, OUTPUT);
  pinMode(lock, OUTPUT);
  digitalWrite(magnetLED, LOW);
  digitalWrite(lock, LOW);

  setupWiFi();
  setupRedis();

  // Set initial lock and door status in Redis
  redis.set("status:lock", String(lock_status).c_str());
  redis.set("status:door", String(door_status).c_str());

  auto backoffCounter = -1;
  auto resetBackoffCounter = [&]() {
    backoffCounter = 0;
  };

  while (subscribeLoop(resetBackoffCounter)) {
    auto curDelay = min((1000 * (int)pow(2, backoffCounter)), MAX_BACKOFF);

    if (curDelay != MAX_BACKOFF) {
      ++backoffCounter;
    }

    Serial.printf("Waiting %ds to reconnect...\n", curDelay / 1000);
    delay(curDelay);
  }
}

bool subscribeLoop(std::function<void(void)> resetCounter) {
  Serial.println("Listening...");
  resetCounter();
  pub_redis.subscribe("control");
  auto subRv = pub_redis.startSubscribingNonBlocking(redisSubCallback, loop);
  return subRv == RedisSubscribeServerDisconnected;
}

void setupWiFi() {
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
}

void setupRedis() {
  espClient.connect(redisServer, redisPort);
  if (espClient.connected()) {
    Serial.print("connected redis");
  }

  pubSubWiFiClient.connect(redisServer, redisPort);
  if (pubSubWiFiClient.connected()) {
    Serial.print("connected redis");
  }
}

void redisSubCallback(Redis* redisInstance, String channel, String msg) {
  Serial.println(channel);
  Serial.println(msg);
  if (channel == "control") {
    if (msg == "0")
    {
      digitalWrite(lock, LOW);
    }
    if (msg == "1")
    {
      digitalWrite(lock, HIGH);
    }

  }
}


void loop() {
  currentTime = millis();
  if (!espClient.connected()) {
    Serial.println("Failed to connect to Redis server");
    espClient.connect(redisServer, redisPort);
  }
  //
  int new_door_status = !digitalRead(magnet_switch);
  int new_lock_status = digitalRead(lock);

  if (new_door_status != door_status || new_lock_status != lock_status) {
    door_status = new_door_status;
    lock_status = new_lock_status;
    Serial.printf("lock data: %d\n", lock_status);
    String publishPayload = "{\"lock\": " + String(lock_status) + ", \"door\": " + String(new_door_status) + "}";
    Serial.println(publishPayload);
    // Publish message vào "status" trong Redis
    // const char* dit ="DITME";
    // const char* key = "status";
    if (redis.set("status", publishPayload.c_str())) {
      Serial.println("Success");
    } else {
      Serial.println("Failed");
    }
  }

  // digitalWrite(magnetLED, LOW);
  delay(1000);
}
