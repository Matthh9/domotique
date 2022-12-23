# domotique

WORK IN PROGRESS

repo contenant les scripts permettant de faire tourner le projet domotique home made
base :
- rpi avec :
        - serveur mqtt mosquito pour la communication
        - Homebridge permettant la communication avec google home
        - domoticz permettant la connexion entre le serveur mqtt et homebridge
        - script python transformé en service comportant l'intéligence pour le lancement des actions en fonction des commandes de google home
- des EPS32 : 
        - connecté au wifi pour permettre la communication grace au serveur mqtt puis l'exécution d'actions notamment grace à des relays (simulation de pression de bouton sur des équipements physiques)
