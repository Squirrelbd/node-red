
#include <Ethernet.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"
const int oneWireBus = 8;
float temp;
float tempSensor1;
float tempSensor2;
float humSensor1;
float humSensor2;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);
int numberOfDevices;
DeviceAddress tempDeviceAddress;

#define DHTPIN0 2     // Digital pin connected to the DHT sensor
#define DHTPIN1 3     // Digital pin connected to the DHT sensor

//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE0 DHT21   // DHT 22  (AM2302), AM2321
#define DHTTYPE1 DHT21   // DHT 22  (AM2302), AM2321

DHT dht0(DHTPIN0, DHTTYPE0);
DHT dht1(DHTPIN1, DHTTYPE1);
// This MAC addres can remain the same
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };

IPAddress ip(192, 168, 5, 110);
//IPAddress myDns(202,191,120, 2);
//IPAddress gateway(10, 30, 0, 1);
//IPAddress subnet(255, 0, 0, 0);

// Replace with your Raspberry Pi IP Address. In my case, the RPi IP Address is 192.168.1.76
IPAddress server(192, 168, 5, 101);
// Initializes the variables
EthernetClient ethClient_hvac;
PubSubClient client(ethClient_hvac);

char Hum[10];
char Tem[10];

char Hum1[10];
char Tem1[10];
char Hum2[10];
char Tem2[10];

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

// Change the function below if you want to subscribe to more topics with your Arduino
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient_HVAC_Meter")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  //Serial.begin(115200);
  client.setServer(server, 1883);
  client.setCallback(callback);
  sensors.begin();
  dht0.begin();
  dht1.begin();
  Ethernet.begin(mac, ip);
  numberOfDevices = sensors.getDeviceCount();
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  // Loop through each device, print out address
  for (int i = 0; i < numberOfDevices; i++) {
    // Search the wire for address
    if (sensors.getAddress(tempDeviceAddress, i)) {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  totalTemp();
  float tempA =  temp / numberOfDevices;
  Serial.println("==================");
  Serial.print("Temperature Avarage for 2 device: ");
  Serial.println(tempA);
  Serial.println("==================");

  float humAvg = totalHum() / 2;
  Serial.println("==================");
  Serial.print("Humidity Avarage for 2 device: ");
  Serial.println(humAvg);
  Serial.println("==================");

  dtostrf(tempA, 4, 0, Tem);
  dtostrf(humAvg, 4, 0, Hum);

  // Publishes a new Temperature value
  //client.publish("topic_temp", Tem);
  //client.publish("topic_hum", Hum);

  client.publish("topic_temp_1", Tem1);
  client.publish("topic_temp_2", Tem2);

  client.publish("topic_hum_1", Hum1);
  client.publish("topic_hum_2", Hum2);


  delay(5000);
}

void totalTemp() {

  sensors.requestTemperatures(); // Send the command to get temperatures
  delay(10);
  temp = 0;
  // Loop through each device, print out temperature data
  for (int i = 0; i < numberOfDevices; i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      // Output the device ID
      Serial.print("Temperature for device: ");
      Serial.println(i, DEC);
      // Print the data
      float tempC = sensors.getTempC(tempDeviceAddress);
      Serial.print("Temp C: ");
      Serial.println(tempC);
      if (i == 0) {
        tempSensor1 = tempC;
        dtostrf(tempSensor1, 4, 0, Tem1);

      } else {
        tempSensor2 = tempC;
        dtostrf(tempSensor2, 4, 0, Tem2);
      }
      temp = temp + tempC;
    }
  }
  if ( temp <= 20) {
    Serial.println(F("Not Proper Value from Temp sensor!"));
    return;
  }
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

float totalHum() {

  float totalHum = 0;
  float h0 = dht0.readHumidity();
  float t0 = dht0.readTemperature();
  float f0 = dht0.readTemperature(true);

  float h1 = dht1.readHumidity();
  float t1 = dht1.readTemperature();
  float f1 = dht1.readTemperature(true);


  //Check if any reads failed and exit early (to try again).
  if (isnan(h0) || isnan(t0) || isnan(f0)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  } else {
    Serial.print("Humidity 1: ");
    Serial.println(h0);
    humSensor1 = h0;
    dtostrf(humSensor1, 4, 0, Hum1);


  }
  float hif0 = dht0.computeHeatIndex(f0, h0);
  float hic0 = dht0.computeHeatIndex(t0, h0, false);

  //Check if any reads failed and exit early (to try again).
  if (isnan(h1) || isnan(t1) || isnan(f1)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  else {
    Serial.print("Humidity 2: ");
    Serial.println(h1);
    humSensor2 = h1;
    dtostrf(humSensor2, 4, 0, Hum2);

  }
  float hif1 = dht1.computeHeatIndex(f1, h1);
  float hic1 = dht1.computeHeatIndex(t1, h1, false);
  totalHum = h0 + h1;
  Serial.println();

  return totalHum;
}
