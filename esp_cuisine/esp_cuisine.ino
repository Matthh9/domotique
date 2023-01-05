#include <C:/Users/matth/Documents/GitHub/domotique/libraries/variable_conf_esp/variable_conf_esp.h>

#include <WiFi.h>
#include <PubSubClient.h>

#define FOUR 12
#define PLAQUE 13
#define lave_linge_power 26
#define lave_linge_start 32
#define lave_vaisselle_power 27
#define lave_vaisselle_select 33
#define lave_vaisselle_demi_charge 25


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


//fonction permettant de simuler l'appuie sur un bouton en activant pendant 600 ms une sortie du contrôleur.
//la sortie doit être raccordée sur un relais qui permet de ferme le circuit du bouton à commander
// input : int port : correspond au numéro du port arduino à activer
// output : none
void bouton(int port, int temps=500) {
  digitalWrite(port, HIGH);
  delay(temps);
  digitalWrite(port, LOW);
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
        digitalWrite(lave_linge_power, HIGH);
        delay(5000);
        bouton(lave_linge_start);
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        //on éteint la prise de la machine
        digitalWrite(lave_linge_power, LOW);
      }
    }
    //lave vaisselle
    else if (message.indexOf("{\"idx\" : 2") != -1 ) {
      if (message.indexOf("\"nvalue\" : 3") != -1) {
        //on allume la prise du lave vaisselle
        digitalWrite(lave_vaisselle_power, HIGH);
        delay(5000);

        //réinitialisation de lave vaisselle si l'ancien programme ne s'est pas fini
        bouton(lave_vaisselle_select, 5000);
        delay(1000);
        
        //séquence de lancement du lave vaisselle select et demi charge
        bouton(lave_vaisselle_select);
        delay(500);
        bouton(lave_vaisselle_demi_charge);
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        //on éteint la prise du lave vaisselle
        digitalWrite(lave_vaisselle_power, LOW);
      }
    }
    //Four
    else if (message.indexOf("{\"idx\" : 3") != -1 ) {
      if (message.indexOf("\"nvalue\" : 1") != -1) {
        Serial.print("four on");
        digitalWrite(PLAQUE, HIGH);
      } else if (message.indexOf("\"nvalue\" : 0") != -1) {
        Serial.print("four off");
        digitalWrite(PLAQUE, LOW);
      }
    }
    //Plaque
    else if (message.indexOf("{\"idx\" : 4") != -1 ) {
      if (message.indexOf("\"nvalue\" : 1") != -1) {
        digitalWrite(FOUR, HIGH);
      } else if (message.indexOf("\"nvalue\":0") != -1) {
        digitalWrite(FOUR, LOW);
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
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect("1", USER, PASSWORD)) {
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


  //  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  //  digitalWrite(BUILTIN_LED, HIGH);
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //voir comment faire pour boucler et attendre un peu évitant au microcontrolleur de tourner à fond dans le vide vérifiant juste si la connection est fonctionnelle
  // par exemple une vérification toutes les 10 secondes sans pour autant mettre un delay qui bloque le microcontroleur pendant ce temps
  unsigned long now = millis();
  //  if (now - lastMsg > 200000) {
  //    Serial.println("boucle");
  //    lastMsg = now;
  //    ++value;
  //    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
  //    Serial.print("Publish message: ");
  //    Serial.println(msg);
  //    client.publish("outTopic", msg);
  //  }
}
