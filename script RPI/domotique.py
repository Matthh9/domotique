import random

from paho.mqtt import client as mqtt_client


broker = '192.168.1.10'
port = 1883
topics = ["commande", "serveur"]
# generate client ID with pub prefix randomly
client_id = f'python-mqtt-{random.randint(0, 100)}'
username = '' #mqtt user
password = '' #pwd de l'user mqtt

import subprocess


def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def publish(client, topic, msg):
    result = client.publish(topic, msg)
    # result: [0, 1]
    status = result[0]
    if status == 0:
        print(f"Send `{msg}` to topic `{topic}`")
    else:
        print(f"Failed to send message to topic {topic}")
     
     
def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")
            
        if "commande" in msg.topic:
            #machine a laver
            if '"idx" : 1' in msg.payload.decode() and '"nvalue" : 1' in msg.payload.decode() :
                process = subprocess.run(["sh", "/home/pi/domotique/send_mqtt.sh", "commande", "{\"idx\" : 1, \"nvalue\" : 3}", "now+1min"])# "3:47"])
                publish(client, "commande", '{"idx" : 1, "nvalue" : 2}')
                process = subprocess.run(["sh", "/home/pi/domotique/send_mqtt.sh", "commande", "{\"idx\" : 1, \"nvalue\" : 0}", "now+2min"])#"7:04"])
            
            #lave vaisselle
            elif '"idx" : 2' in msg.payload.decode() and '"nvalue" : 1' in msg.payload.decode() :
                process = subprocess.run(["sh", "/home/pi/domotique/send_mqtt.sh", "commande", "{\"idx\" : 2, \"nvalue\" : 3}", "0:20"])
                publish(client, "commande", '{"idx" : 2, "nvalue" : 2}')
                process = subprocess.run(["sh", "/home/pi/domotique/send_mqtt.sh", "commande", "{\"idx\" : 2, \"nvalue\" : 0}", "2:00"])
            
            #coupure du wifi pour une box orange
            elif '"idx" : 6' in msg.payload.decode() :
                if '"nvalue" : 1' in msg.payload.decode() :
                    process = subprocess.run(["sudo", "/usr/local/bin/sysbus", "-url http://192.168.1.1/", "-wifion"])
                    print("wifi on")
                elif '"nvalue" : 0' in msg.payload.decode() :
                    process = subprocess.run(["sudo", "/usr/local/bin/sysbus", "-url http://192.168.1.1/", "-wifioff"])
                    #print("wifi off")
            
            
            # traduction pour les requêtes non connues
            elif '"Battery"' in msg.payload.decode() and '"LastUpdate"' in msg.payload.decode():
                publish(client, "commande", "{\"idx\""+msg.payload.decode().split('"idx"',1)[1].split(',',1)[0]+", \"nvalue\""+msg.payload.decode().split('"nvalue"',1)[1].split(',',1)[0]+"}")
                    
                    
        if (msg.topic=="serveur"):
            if (msg.payload.decode()=='dlna'):
                process = subprocess.run(["sudo", "service", "minidlna", "restart"])
            elif (msg.payload.decode()=='reboot'):
                process = subprocess.run(["sudo", "reboot"])
                    

    for topic in topics:
        client.subscribe(topic)
    
    client.on_message = on_message


def run():
    client = connect_mqtt()
    subscribe(client)
    client.loop_forever()


if __name__ == '__main__':
    run()
