#include <C:/Users/matth/Documents/GitHub/domotique/libraries/variable_conf_esp/variable_conf_esp.h>

#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (5000)
char msg[MSG_BUFFER_SIZE];
int value = 0;

#define FOUR 36
#define PLAQUE 39
#define lave_linge_power 26
#define lave_linge_start 32
#define lave_vaisselle_power 27
#define lave_vaisselle_select 33
#define lave_vaisselle_demi_charge 25

#define volet_temps_ouverture 30000 //ouverture à 100% du volet

struct Volet_velux {
  int volet_down;
  int volet_up;
  int volet_power;
  int volet_position;
  int volet_new_position;
};

Volet_velux volet = {12, 13, 14, 0, 0};


/**********************************************************************************
 * SECTION DES FONCTIONS
 *********************************************************************************/

/**
 * fonction permettant de simuler l'appuie sur un bouton en activant pendant 600 ms une sortie du contrôleur.
 * la sortie doit être raccordée sur un relais qui permet de ferme le circuit du bouton à commander
 * input : int port : correspond au numéro du port arduino à activer
           (optionnel, defaut 500 ms) int temps : ms a temporiser l'activation du port si pas de 
 * output : none
**/
void bouton(int port, int temps=250) {
  digitalWrite(port, HIGH);
  vTaskDelay(temps / portTICK_PERIOD_MS);
  digitalWrite(port, LOW);
}


/**
 * tache permettant de démarrer le lave vaisselle, appelé lors de la reception du bon message mqtt
 * la fonction envoie un message mqtt de confirmation
 * input : none
 * output : none
**/
void demarrage_lave_vaisselle(void * parameter){
  client.publish("commande", "demarrage lave vaiselle");
  //on allume la prise du lave vaisselle
  digitalWrite(lave_vaisselle_power, HIGH);
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  
  //séquence de lancement du lave vaisselle select et demi charge
  bouton(lave_vaisselle_select, 500);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  bouton(lave_vaisselle_demi_charge, 1000);
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  // When you're done, call vTaskDelete. Don't forget this!
  vTaskDelete(NULL);
}


/**
 * tache permettant de démarrer la machine à laver, appelé lors de la reception du bon message mqtt
 * la fonction envoie un message mqtt de confirmation
 * input : none
 * output : none
**/
void demarrage_machine(void * parameter){
  client.publish("commande", "demarrage machine");

  //on allume la prise de la machine
  digitalWrite(lave_linge_power, HIGH);
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  //on simule l'appuie sur le bouton start
  bouton(lave_linge_start);
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  // When you're done, call vTaskDelete. Don't forget this!
  vTaskDelete(NULL);
}


/**
 * tache permettant d'ouvrir ou de fermer les volets des velux, appelé lors de la reception du bon message mqtt
 * input : structure de type Volet_velux permettant d'avoir toutes les infos sur le volet
 * output : none
**/
void velux_commande_2(void * parameter){
  //client.publish("commande", "demarrage machine");

  Volet_velux* packet = (Volet_velux*)parameter;

  //calcul de l'action à faire
  int temps;
  if (packet->volet_new_position == 100){
    temps = -volet_temps_ouverture+2000;
  } else if (packet->volet_new_position == 0){
    temps = volet_temps_ouverture+2000;
  } else {
    temps = (packet->volet_position-packet->volet_new_position)*volet_temps_ouverture/100;
  }
  Serial.println( temps );

  int port;
  if (temps==abs(temps)){ 
    port = packet->volet_down ;
  } else { port = packet->volet_up ; }
  Serial.println( port );
  
  
  //on allume l'alim
  digitalWrite(packet->volet_power, HIGH);
  vTaskDelay(2500 / portTICK_PERIOD_MS);
  
  //on active le port necessaire pour lancer la commande d'ouverture ou de fermeture
  digitalWrite(port, HIGH);
  vTaskDelay(abs(temps) / portTICK_PERIOD_MS);
  digitalWrite(port, LOW);
  vTaskDelay(2500 / portTICK_PERIOD_MS);
  
  //on éteint l'alim
  digitalWrite(packet->volet_power, LOW);
  

  //on positionne la nouvelle position comme la position courante
  packet->volet_position=packet->volet_new_position;
  
  // When you're done, call vTaskDelete. Don't forget this!
  vTaskDelete(NULL);
}



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
    if (message.indexOf("{\"idx\" : 1,") != -1 ) {
      if (message.indexOf("\"nvalue\" : 3") != -1) {
        xTaskCreatePinnedToCore(
          demarrage_machine,    // Function that should be called
          "demarrage_machine",   // Name of the task (for debugging)
          5000,            // Stack size (bytes)
          NULL,            // Parameter to pass
          1,               // Task priority
          NULL,             // Task handle
          1          // Core you want to run the task on (0 or 1)
        );
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        //on éteint la prise de la machine
        digitalWrite(lave_linge_power, LOW);
        client.publish("commande", "extinction machine");
      }
    }
    
    //lave vaisselle
    else if (message.indexOf("{\"idx\" : 2,") != -1 ) {
      if (message.indexOf("\"nvalue\" : 3") != -1) {
        xTaskCreatePinnedToCore(
          demarrage_lave_vaisselle,    // Function that should be called
          "demarrage_lave_vaisselle",   // Name of the task (for debugging)
          5000,            // Stack size (bytes)
          NULL,            // Parameter to pass
          1,               // Task priority
          NULL,             // Task handle
          1          // Core you want to run the task on (0 or 1)
        );

      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        //on éteint la prise du lave vaisselle
        digitalWrite(lave_vaisselle_power, LOW);
        client.publish("commande", "extinction lave vaiselle");
      }
    }
    
    //Four
    else if (message.indexOf("{\"idx\" : 3,") != -1 ) {
      if (message.indexOf("\"nvalue\" : 1") != -1) {
        Serial.print("four on");
        digitalWrite(PLAQUE, HIGH);
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        Serial.print("four off");
        digitalWrite(PLAQUE, LOW);
      }
    }
    
    //Plaque
    else if (message.indexOf("{\"idx\" : 4,") != -1 ) {
      if (message.indexOf("\"nvalue\" : 1") != -1) {
        digitalWrite(FOUR, HIGH);
      } else if (message.indexOf("\"nvalue\":0") != -1) {
        digitalWrite(FOUR, LOW);
      }
    }

    //Volet velux
    else if (message.indexOf("{\"idx\" : 16,") != -1 ) {
      if (message.indexOf("\"nvalue\" : 1") != -1) {
        volet.volet_new_position = 100;
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        volet.volet_new_position = 0;
      } else {
        int volet_level = message.indexOf("\"level\" :");
        int volet_new_position = message.substring(volet_level+10,message.length()-1).toInt();
        volet.volet_new_position = volet_new_position;
      }
      xTaskCreatePinnedToCore(
        velux_commande_2,    // Function that should be called
        "velux_commande",   // Name of the task (for debugging)
        5000,            // Stack size (bytes)
        &volet,            // Parameter to pass
        1,               // Task priority
        NULL,             // Task handle
        1          // Core you want to run the task on (0 or 1)
      );     
    }
  }
}


void setup() {
  pinMode(FOUR, OUTPUT);
  digitalWrite(FOUR, LOW);
  pinMode(PLAQUE, OUTPUT);
  digitalWrite(PLAQUE, LOW);

  pinMode(lave_linge_power, OUTPUT);
  digitalWrite(lave_linge_power, LOW);
  pinMode(lave_linge_start, OUTPUT);
  digitalWrite(lave_linge_start, LOW);

  pinMode(lave_vaisselle_select, OUTPUT);
  digitalWrite(lave_vaisselle_select, LOW);
  pinMode(lave_vaisselle_demi_charge, OUTPUT);
  digitalWrite(lave_vaisselle_demi_charge, LOW);
  pinMode(lave_vaisselle_power, OUTPUT);
  digitalWrite(lave_vaisselle_power, LOW);

  pinMode(volet.volet_power, OUTPUT);
  digitalWrite(volet.volet_power, LOW);
  pinMode(volet.volet_up, OUTPUT);
  digitalWrite(volet.volet_up, LOW);
  pinMode(volet.volet_down, OUTPUT);
  digitalWrite(volet.volet_down, LOW);
  

  //  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  //  digitalWrite(BUILTIN_LED, HIGH);
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  //Serial.println(!client.connected());
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
