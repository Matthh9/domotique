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
#define NUM_LEDS    50
int BRIGHTNESS=255;

CRGB leds[NUM_LEDS];


/**********************************************************************************
 * SECTION DES FONCTIONS
 *********************************************************************************/
void sonnette(void * parameter){
    // Change this depending on where you put your piezo buzzer
    const int TONE_OUTPUT_PIN = 26;
    
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

    ledcAttachPin(TONE_OUTPUT_PIN, TONE_PWM_CHANNEL);

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

    ledcDetachPin(TONE_OUTPUT_PIN);
    
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

  // Implement your custom logic here
  IrSender.begin(IR_SEND_PIN, false);// Specify send pin and enable feedback LED at default feedback LED pin
  
  uint16_t sAddress = 0x0102;
  uint8_t sCommand = 0x34;
  uint8_t sRepeats = 0;
  
  // Results for the first loop to: Protocol=NEC Address=0x102 Command=0x34 Raw-Data=0xCB340102 (32 bits)
  IrSender.sendNEC(sAddress, sCommand, sRepeats);
//  IrSender.sendNECRaw(0xCB340102, sRepeats);
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
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
    //lave linge
    if (message.indexOf("{\"idx\" : 1") != -1 ) {
      Serial.println("Lave linge");
      if (message.indexOf("\"nvalue\" : 3") != -1) {
        //on allume la prise de la machine
        //digitalWrite(lave_linge_power, HIGH);
        delay(5000);
        //bouton(lave_linge_start);
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        //on éteint la prise de la machine
        //digitalWrite(lave_linge_power, LOW);
      }
    }

    if (message.indexOf("{\"idx\" : 3") != -1 ) {
      Serial.println("sonette");
      if (message.indexOf("\"nvalue\" : 1") != -1) {
        //on allume la prise de la machine
        //digitalWrite(lave_linge_power, HIGH);

        
        xTaskCreatePinnedToCore(
            sonnette,    // Function that should be called
            "sonette",   // Name of the task (for debugging)
            1000,            // Stack size (bytes)
            NULL,            // Parameter to pass
            1,               // Task priority
            NULL,             // Task handle
            1          // Core you want to run the task on (0 or 1)
          );
        
        //delay(5000);
        //bouton(lave_linge_start);
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        //on éteint la prise de la machine
        //digitalWrite(lave_linge_power, LOW);
      }
    }

    if (message.indexOf("{\"idx\" : 4") != -1 ) {
      Serial.println("Projo");
      if (message.indexOf("\"nvalue\" : 1") != -1) {
        //on allume la prise de la machine
        //digitalWrite(lave_linge_power, HIGH);

        
        xTaskCreatePinnedToCore(
            demarrage_projo,    // Function that should be called
            "demarrage_projo",   // Name of the task (for debugging)
            1000,            // Stack size (bytes)
            NULL,            // Parameter to pass
            2,               // Task priority
            NULL,             // Task handle
            1          // Core you want to run the task on (0 or 1)
          );
        
        //delay(5000);
        //bouton(lave_linge_start);
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        //on éteint la prise de la machine
        //digitalWrite(lave_linge_power, LOW);
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


#define led1 32
#define led2 27

// task to be delayed
void vATask1( void * pvParameters )
{
  for(;;){ // infinite loop

    // Turn the LED on
    digitalWrite(led1, HIGH);

    // Pause the task for 500ms
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //delay(1000);
    
    // Turn the LED off
    digitalWrite(led1, LOW);

    // Pause the task again for 500ms
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //delay(1000);
  }
}


// task to be delayed
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
  
}


void setup() {
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);


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



  xTaskCreatePinnedToCore(
      vATask1,    // Function that should be called
      "Toggle LED",   // Name of the task (for debugging)
      1000,            // Stack size (bytes)
      NULL,            // Parameter to pass
      2,               // Task priority
      NULL,             // Task handle
      0          // Core you want to run the task on (0 or 1)
    );
    

}

void loop() {
   //put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  pride();
  FastLED.show();
}
