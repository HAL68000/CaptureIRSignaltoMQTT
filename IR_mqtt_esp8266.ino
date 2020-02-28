/*
  Important note before compile

  Modify the PubSubClient.h library and raise the maximum buffer size from 128 bytes to an higher value otherwise it will not send anything

  E.g.
  #ifndef MQTT_MAX_PACKET_SIZE
  #define MQTT_MAX_PACKET_SIZE 2048
  #endif


*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>

// Update these with values suitable for your network.

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_SERVER";
bool msgsent = false;
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
char msglong[4096];
int value = 0;
// ==================== start of TUNEABLE PARAMETERS ====================

// The GPIO an IR detector/demodulator is connected to. Recommended: 14 (D5)
// Note: GPIO 16 won't work on the ESP8266 as it does not have interrupts.
const uint16_t kRecvPin = 5;

// GPIO to use to control the IR LED circuit. Recommended: 4 (D2).
const uint16_t kIrLedPin = 14;

// The Serial connection baud rate.
// NOTE: Make sure you set your Serial Monitor to the same speed.
const uint32_t kBaudRate = 115200;

// As this program is a special purpose capture/resender, let's use a larger
// than expected buffer so we can handle very large IR messages.
const uint16_t kCaptureBufferSize = 1024;  // 1024 == ~511 bits

// kTimeout is the Nr. of milli-Seconds of no-more-data before we consider a
// message ended.
const uint8_t kTimeout = 50;  // Milli-Seconds

// kFrequency is the modulation frequency all UNKNOWN messages will be sent at.
const uint16_t kFrequency = 38000;  // in Hz. e.g. 38kHz.


// ==================== end of TUNEABLE PARAMETERS ====================

// The IR transmitter.
IRsend irsend(kIrLedPin);
// The IR receiver.
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, false);
// Somewhere to store the captured message.
decode_results results;
void fanspeed() {
  Serial.println("Sent");

  uint16_t fanonoff[] = {8970, 4454, 648, 1568, 648, 462, 648, 458, 648, 462, 648, 462, 622, 484, 648, 462, 648, 462, 624, 1594, 648, 1568, 648, 462, 648, 1568, 648, 1570, 648, 1568, 648, 1572, 648, 458, 648, 462, 648, 462, 648, 462, 648, 458, 648, 462, 648, 464, 622, 486, 644, 462, 648, 1572, 644, 1572, 622, 1594, 648, 1568, 648, 1574, 618, 1598, 648, 1568, 648, 1570, 648, 462, 648, 458, 648, 462, 648, 462, 644, 462, 648, 462, 624, 482, 648, 462, 648, 1568, 648, 1572, 644, 1574, 648, 1568, 648, 1568, 650, 1572, 618, 1600, 648, 1568, 648, 14336, 8998, 2140, 648}; // output
  uint16_t *raw_array = fanonoff;

  irsend.sendRaw(raw_array, 104, kFrequency );
}
void turnonoff() {
  Serial.println("Sent");
  uint16_t  rawData[103] = {8982, 4450, 648, 1572, 644, 462, 648, 462, 648, 462, 648, 462, 644, 462, 648, 462, 648, 462, 648, 1568, 648, 1574, 644, 462, 648, 1572, 648, 1568, 648, 1568, 648, 1572, 648, 462, 644, 1572, 648, 462, 644, 1572, 648, 1568, 648, 462, 648, 462, 644, 466, 644, 462, 648, 462, 648, 1568, 648, 462, 644, 466, 644, 1572, 648, 1568, 648, 1574, 644, 1572, 648, 1568, 648, 462, 648, 1568, 648, 1572, 644, 610, 648, 610, 648, 610, 644, 462, 648, 462, 644, 1572, 624, 488, 644, 462, 648, 1568, 648, 1568, 648, 1568, 650, 1572, 644, 14022, 8996, 2138, 644}; //  DC7BBB41
  uint16_t *raw_array = rawData;

  irsend.sendRaw(raw_array, 104, kFrequency );
}
void turntvon() {
  irsend.sendMitsubishi2(0x28A8);
}
void tvinput() {

  irsend.sendMitsubishi2(0x283C);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  irrecv.enableIRIn();  // Start up the IR receiver.
  irsend.begin();       // Start up the IR sender.

  Serial.begin(kBaudRate, SERIAL_8N1);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();

  Serial.print("SmartIRRepeater is now running and waiting for IR input "
               "on Pin ");
  Serial.println(kRecvPin);
  Serial.print("and will retransmit it on Pin ");
  Serial.println(kIrLedPin);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //     client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();


  {
    // parse command
    char ch = Serial.read();
    // Serial.print(ch);
    switch (ch)
    {

      case 'o': turnonoff(); break;
      case 'f': fanspeed(); break;
      case 't': turntvon(); break;
      case 'i': tvinput(); break;
      default: break;
    }
  }
  delay(500);

  if (msgsent != false) {
    Serial.println("Sent");

    client.publish("IR", msglong);
    msgsent = false;
  }
  // Check if an IR message has been received.
  if (irrecv.decode(&results)) {  // We have captured something.
    // The capture has stopped at this point.

    decode_type_t protocol = results.decode_type;
    dumpCode(&results, protocol);
    uint16_t size = results.bits;
    bool success = true;

    irrecv.resume();

    // Display a crude timestamp & notification.

  }
  yield();  // Or delay(milliseconds); This ensures the ESP doesn't WDT reset.
}
void dumpCode(decode_results *results, decode_type_t protocol) {

  char sizechar[20];
  // Start declaration
  strcpy(msglong, "uint16_t_rawData[");

  sprintf(sizechar, "%d", (results->rawlen - 1));
  strcat(msglong, sizechar);
  //Serial.print(msg);
  // Start declaration
  Serial.print("uint16_t  ");              // variable type
  Serial.print("rawData[");                // array name
  Serial.print(results->rawlen - 1, DEC);  // array size
  Serial.print("] = {");                   // Start declaration
  strcat(msglong, "]={");

  // Dump data
  for (uint16_t i = 1; i < results->rawlen; i++) {
    sprintf(sizechar, "%d", (results->rawbuf[i] * kRawTick));
    strcat(msglong, sizechar);
    strcat(msglong, " ");
    Serial.print(results->rawbuf[i] * kRawTick, HEX);
    if (i < results->rawlen - 1)
      Serial.print(",");  // ',' not needed on last one
    if (!(i & 1)) Serial.print(" ");
  }
  strcat(msglong, "}; ");

  // End declaration
  Serial.print("};");  //

  // Comment
  Serial.print("  // ");
  //Serial.print(encoding(results));
  Serial.print(" ");
  serialPrintUint64(results->value, 16);

  // Newline
  Serial.println("");

  // Now dump "known" codes
  if (results->decode_type != UNKNOWN) {
    // Some protocols have an address &/or command.
    // NOTE: It will ignore the atypical case when a message has been decoded
    // but the address & the command are both 0.
    if (results->address > 0 || results->command > 0) {
      strcat(msglong, "uint32_t_address_=_0x");
      Serial.print("uint32_t  address = 0x");
      Serial.print(results->address, HEX);
      sprintf(sizechar, "%0X", (results->address));
      strcat(msglong, sizechar);
      Serial.println(";");
      Serial.print("uint32_t  command = 0x");
      sprintf(sizechar, " uint32_t_command_=_0x%0X ", results->command);
      strcat(msglong, sizechar);
      Serial.print(results->command, HEX);
      Serial.println(";");
    }


  }
  // All protocols have data
  Serial.print("uint64_t  data = 0x");
  sprintf(sizechar, "uint64_t_data_=_0x%0X ", (results->value));
  strcat(msglong, sizechar);
  strcat(msglong,  typeToString(protocol).c_str());
  Serial.println(msglong);


  serialPrintUint64(results->value, 16);
  Serial.println(";");
  msgsent = true;

}
