/*
 * this firmware receives an URL that returns data in json 
 * format then decode it and print it
 * 
 * made by Yeffri J. Salazar, Najt Labs Guatemala 2016
 * https://github.com/yeffrimic/handling_json_through_mqtt_and_http_ESP8266
*/
//-------- Libraries required --------//
#include <ESP8266WiFi.h>// this library is for ESP8266
#include <ArduinoJson.h>// to handle the Json
#include <ESP8266HTTPClient.h>// To connect throught HTTP
#include <PubSubClient.h>// to use MQTT protocol

/*
   you can edit all this variables and constants with
   your values, feel free to change it.
*/

//-------- mqtt setup --------//
const char * MQTT_broker = "Broker";//name of the mqtt broker
const char * MQTT_user = "username";//username to access to the broker
const char * MQTT_password = "password";//password to access to the broker
const char * MQTT_topicSub = "topic to subscribe";//Topic to susbcribe
const char * MQTT_topicAlive = "topic alive";//Topic to publish alive messages
const uint16_t MQTT_port = 1234; // port  to access to the broker
//those two are for the functionality of the mqtt
WiFiClient espClient; 
PubSubClient client(espClient);

//-------- wifi setup --------//
const char* ssid = "SSID";//SSID of the wifi that you will use
const char* password = "password"; //password of the wifi that you will use


/********* Setup wifi ***************************
   setup wifi connect to wifi with the constants
   defined up
   while does not connect print a "."
   if connect then print the local ip
************************************************/

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print(F("Connecting to ")) ;
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/******** void callback***********************
   this function receive the info from mqtt
   suscriptionand print the infro that comes
   through
**********************************************/

void callback(char* topic, uint8_t* payload, uint16_t length) {
  String URLin;
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));
  for (uint16_t i = 0; i < length; i++) {
    URLin += (char)payload[i];
  }
  Serial.println(URLin);
  delay(1000);
 httpRequest(URLin);
}

/******** void httpRequest***********************
    this function receives the url and make the
    request and receive a json and send it to
    parseJson function
    if doesnt receive an Url publish the error
    to RequestError topic
 *                                              *
 ***********************************************/


void httpRequest(String urlRequest) {
  HTTPClient http;
  Serial.print(F("[HTTP] begin...\n"));
  http.begin(urlRequest); //HTTP
  Serial.print(F("[HTTP] GET...\n"));
  uint16_t httpCode = http.GET();
  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
      publisher("http ok", "StatusRequest");
      parseJson(payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    publisher("http request fail", "StatusRequest");
  }

  http.end();
}

/******** void publisher***********************
    function that publish to the broker
    receives the message and topic
 *                                              *
 ***********************************************/
void publisher(char* topublish, char * topic) {
  if (client.connect("ESP8266Client", MQTT_user, MQTT_password)) {
    Serial.println("connected");
    // Once connected, publish an announcement...
    client.publish(topic, topublish);
  } else {
    Serial.print(F("failed, rc="));
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
    reconnect();
  }
}


/******** void ParseJson***********************
  this funcition receives json data from
   http request and decode and print it
 ***********************************************/

void parseJson(String json) {
  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    publisher("this is not a Json","JsonStatus");
    return;
  }
    publisher("Json ok ","JsonStatus");
  for (JsonObject::iterator it = root.begin(); it != root.end(); ++it)
  {
    Serial.println(it->key);
    Serial.println(it->value.asString());
  }
}

/*
   when the mqtt client is disconnected,
   then try to reconnect to mqtt broker.
   and suscribe the mqtt suscription topic
   else try to reconnect each 5 seconds
*/

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(F("Attempting MQTT connection..."));
    // Attempt to connect
    if (client.connect("ESP8266Client", MQTT_user, MQTT_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(MQTT_topicAlive, "alive");
      // ... and resubscribe
      client.subscribe(MQTT_topicSub);
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);//begin the Serial comunnication
  setup_wifi();// connect to wifi
  client.setServer(MQTT_broker, MQTT_port);
  client.setCallback(callback);
}


void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

}
