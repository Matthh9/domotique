#include <C:/Users/matth/Documents/GitHub/domotique/libraries/variable_conf_esp/variable_conf_esp.h>

#include <IRremote.h>
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


#include "FastLED.h"

#define DATA_PIN    14
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    23
int BRIGHTNESS=255;

CRGB leds[NUM_LEDS];

#define port_sonnette 32
#define port_piezo 26
#define projo_power 12
#define led_power 27

bool lumiere_on_off = false;
bool sonnette_off = true;

/**********************************************************************************
 * SECTION DES FONCTIONS
 *********************************************************************************/
void sonnette(void * parameter){
  client.publish("commande", "sonnette");
  // Change this depending on where you put your piezo buzzer
  //const int TONE_OUTPUT_PIN = port_piezo;
  
  // The ESP32 has 16 channels which can generate 16 independent waveforms
  // We'll just choose PWM channel 1 here
  const int TONE_PWM_CHANNEL = 1; 
  
  // change this to make the song slower or faster
  int tempo = 144;

  note_t notemusique [] =  {
    NOTE_D, NOTE_G, NOTE_Bb, NOTE_A,
    NOTE_G, NOTE_D, NOTE_C, NOTE_A,
  };
  
  int octave [] = { //uint8_t
    4, 4, 4, 4, 
    4, 5, 5, 4,
  };
  
  int temp[] = {
    4, -4, 8, 4, 
    2, 4, -2, -2,
  };

  int notes = sizeof(notemusique) / sizeof(notemusique[0]);
  // this calculates the duration of a whole note in ms (60s/tempo)*4 beats
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;

  ledcAttachPin(port_piezo, TONE_PWM_CHANNEL);

  for (int thisNote = 0; thisNote < notes; thisNote = thisNote + 1) {
    // calculates the duration of each note
    divider = temp[thisNote];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }
    // we only play the note for 90% of the duration, leaving 10% as a pause
    ledcWriteNote(TONE_PWM_CHANNEL, notemusique[thisNote], octave[thisNote]);

    // Wait for the specief duration before playing the next note.
    vTaskDelay(noteDuration*0.9 / portTICK_PERIOD_MS);
    ledcWriteTone(TONE_PWM_CHANNEL, 0);
    vTaskDelay(noteDuration*0.1 / portTICK_PERIOD_MS);
    
  }

  ledcDetachPin(port_piezo);

  
  digitalWrite(port_sonnette, LOW);
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  
  sonnette_off=true;
  
  // When you're done, call vTaskDelete. Don't forget this!
  vTaskDelete(NULL);
}


void demarrage_projo(void * parameter){
  Serial.print("projo task");
//  if defined(ESP32)
//  #define IR_RECEIVE_PIN          15  // D15
//  #define IR_SEND_PIN              4  // D4
//  #define tone(a,b,c) void()      // no tone() available on ESP32
//  #define noTone(a) void()
//  #define TONE_PIN                42 // Dummy for examples using it
//  #define APPLICATION_PIN         16 // RX2 pin

  digitalWrite(projo_power, HIGH);
  vTaskDelay(10000 / portTICK_PERIOD_MS);

  // Implement your custom logic here
  IrSender.begin(IR_SEND_PIN, false);// Specify send pin and enable feedback LED at default feedback LED pin
  
  uint16_t sAddress = 0x0102;
  uint8_t sCommand = 0x34;
  uint8_t sRepeats = 5;
  
  // Results for the first loop to: Protocol=NEC Address=0x102 Command=0x34 Raw-Data=0xCB340102 (32 bits)
  //IrSender.sendNEC(sAddress, sCommand, sRepeats);
  IrSender.sendNECRaw(0x57A8FF00, sRepeats); // commande allumage
  vTaskDelay(15000 / portTICK_PERIOD_MS);
  IrSender.sendNECRaw(0x738CFF00, sRepeats); //son up pour forcer l'activation bug du projo
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  IrSender.sendNECRaw(0x639CFF00, sRepeats); //son down pour refaire l'équilibre

// When you're done, call vTaskDelete. Don't forget this!
  vTaskDelete(NULL);
}


void extinction_projo(void * parameter){
  Serial.print("projo task");
//  if defined(ESP32)
//  #define IR_RECEIVE_PIN          15  // D15
//  #define IR_SEND_PIN              4  // D4
//  #define tone(a,b,c) void()      // no tone() available on ESP32
//  #define noTone(a) void()
//  #define TONE_PIN                42 // Dummy for examples using it
//  #define APPLICATION_PIN         16 // RX2 pin

  // Implement your custom logic here
  IrSender.begin(IR_SEND_PIN, false);// Specify send pin and enable feedback LED at default feedback LED pin
  
  uint16_t sAddress = 0x0102;
  uint8_t sCommand = 0x34;
  uint8_t sRepeats = 2;
  
  // Results for the first loop to: Protocol=NEC Address=0x102 Command=0x34 Raw-Data=0xCB340102 (32 bits)
  //IrSender.sendNEC(sAddress, sCommand, sRepeats);
  IrSender.sendNECRaw(0x57A8FF00, sRepeats);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  IrSender.sendNECRaw(0x57A8FF00, sRepeats);
  vTaskDelay(7500 / portTICK_PERIOD_MS);
  digitalWrite(projo_power, LOW);
  
// When you're done, call vTaskDelete. Don't forget this!
  vTaskDelete(NULL);
}


//fonction de base permettant la connection de l'esp au wifi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
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
}


//fonction callback modifiée pour porter les traitement à réaliser en fonction des messages mqtt reçus
void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();

  if (String(topic).equals("commande") == 1) {

    //LED strip
    if (message.indexOf("{\"idx\" : 5") != -1 ) {
      Serial.println("LED");
      if (message.indexOf("\"nvalue\" : 1") != -1) {
        digitalWrite(led_power, HIGH);
        lumiere_on_off=true;

        int debut_level = message.indexOf("\"level\" :");
        int luminosite_message = message.substring(debut_level+10,message.length()-1).toInt();
        FastLED.setBrightness(map(luminosite_message, 1, 100, 1, 255));
        
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        digitalWrite(led_power, LOW);
        lumiere_on_off=false;
      }
    }
    
    //LED strip en violet
    if (message.indexOf("{\"idx\" : 12") != -1 ) {
      Serial.println("Projo");
      if (message.indexOf("\"nvalue\" : 1") != -1) {
        digitalWrite(led_power, HIGH);
        lumiere_on_off=true;
        client.publish("commande", "{\"idx\" : 12, \"nvalue\" : 0}");
      }
    }
    
    //projo
    else if (message.indexOf("{\"idx\" : 8") != -1 ) {
      if (message.indexOf("\"nvalue\" : 1") != -1) {
        xTaskCreatePinnedToCore(
          demarrage_projo,    // Function that should be called
          "demarrage_projo",   // Name of the task (for debugging)
          1000,            // Stack size (bytes)
          NULL,            // Parameter to pass
          2,               // Task priority
          NULL,             // Task handle
          1          // Core you want to run the task on (0 or 1)
        );
          
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        xTaskCreatePinnedToCore(
          extinction_projo,    // Function that should be called
          "extinction_projo",   // Name of the task (for debugging)
          1000,            // Stack size (bytes)
          NULL,            // Parameter to pass
          2,               // Task priority
          NULL,             // Task handle
          1          // Core you want to run the task on (0 or 1)
        );
      }
    }
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(WiFi.localIP());
    // Attempt to connect
    if (client.connect(clientId.c_str(), USER, PASSWORD)) {
      Serial.println("connected");
      // ... and resubscribe
      for (int i = 0; i < taileTopics; i++) {
        client.subscribe(topics[i], 1);
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


// fonction d'animation des leds
void pride()
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, bri8);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( leds[pixelnumber], newcolor, 64);
  }

  FastLED.show();
}

void reconnect();

void setup() {
//  pinMode(port_sonnette, OUTPUT);
//  digitalWrite(port_sonnette, LOW);
//  pinMode(port_sonnette, INPUT);

  pinMode(projo_power , OUTPUT);
  digitalWrite(projo_power, LOW);

  pinMode(led_power , OUTPUT);
  digitalWrite(led_power, LOW);


  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS)
    .setCorrection(TypicalLEDStrip)
    .setDither(BRIGHTNESS < 255);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
}

void loop() {
  //put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  if(lumiere_on_off) {
    pride();
  }
  
//  if(digitalRead(port_sonnette)==HIGH and sonnette_off) {
//    sonnette_off=false;
//    xTaskCreatePinnedToCore(
//        sonnette,    // Function that should be called
//        "sonette",   // Name of the task (for debugging)
//        10000,            // Stack size (bytes)
//        NULL,            // Parameter to pass
//        1,               // Task priority
//        NULL,             // Task handle
//        1          // Core you want to run the task on (0 or 1)
//      );
//  }
  
}
