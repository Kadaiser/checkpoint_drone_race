import paho.mqtt.client as mqtt
import time
import inquirer
import json

MQTT_SERVER = "127.0.0.1";
MQTT_PORT = 1883

receivedMessages = []
racers = []
checkpoints = 0


only_listen = False
connecto_broker = False


TOPIC_REGISTRAR_CORREDOR = 'dronrace/regRunner'
TOPIC_REGISTRAR_CHECK_POINT = 'dronrace/esp32_0B/setCheckpoint'
TOPIC_RECONNECTIONS = 'dronrace/reconnnections'
TOPIC_CHECKPOINT_VALUE = 'dronrace/checkpointValue'
TOPIC_RUNNER_DETECTED = 'dronrace/runnerDetected'

questions = [
  inquirer.List('size',
                message='Operacion desea realizar',
                choices=['1 - Registrar un corredor', '2 - Registrar un checkpoint', '3 - Iniciar carrera'],
            ),
]



# callback function on broker connect
def connect_msg(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        global connecto_broker
        connecto_broker = True                #Use global variable
    else:
        print("Connection failed")


# callback function to subscribe callback messsage event

def on_message(client, userdata, message):
    print("********** Message received ******************")
    print(str(message.payload))
    print("**********************************************")


def on_publish(client, userdata, mid):
  receivedMessages.append(mid)

# publish topic message
def publish(topic, message, waitForAck = False):
  mid = client.publish(topic, message, 2)[1]
  if (waitForAck):
    while mid not in receivedMessages:
      time.sleep(0.25)

def registrarCorredor():
    mac = raw_input('MAC Sensortag: ')
    print('Registrado el corredor con identificador: ' + mac)
    msg = json.dumps({'runner': mac})
    publish(TOPIC_REGISTRAR_CORREDOR, msg, True)
    print(TOPIC_REGISTRAR_CORREDOR, msg, True)

def registrarCheckPoint():
    postion = int(input('Posicion del checkpoint o baliza: '))
    print('Position Baliza: ' + str(postion))
    msg = json.dumps({'value': postion})
    publish(TOPIC_REGISTRAR_CHECK_POINT, msg, True)
    print(TOPIC_REGISTRAR_CHECK_POINT, msg, True)

def inicarSubscripcionCheckPoints():
    client.subscribe(TOPIC_RUNNER_DETECTED)
    client.subscribe(TOPIC_CHECKPOINT_VALUE)
    client.subscribe(TOPIC_RECONNECTIONS)
    print("El servidor se encuentra escuchando los siguientes TOPICS: ", TOPIC_RUNNER_DETECTED, TOPIC_CHECKPOINT_VALUE, TOPIC_RECONNECTIONS)
    global only_listen
    only_listen = True

def toPublish(operation):
    if operation == 'Registrar un corredor':
        registrarCorredor()
    elif operation == 'Registrar un checkpoint':
        registrarCheckPoint()
    elif operation == 'Iniciar carrera':
        inicarSubscripcionCheckPoints()
    else:
        print('Esta implementada esta operacion')

client = mqtt.Client(client_id='publisher-pi')

# Connecting callback functions
client.on_connect = connect_msg
# Connect to broker
client.on_message = on_message
client.on_publish = on_publish
client.connect(MQTT_SERVER,MQTT_PORT)

client.loop_start()        #start the loop

while connecto_broker != True:    #Wait for connection
    time.sleep(0.1)

try:
    while True:
        if only_listen:
            time.sleep(1)
        else:
            answers = inquirer.prompt(questions)
            toPublish(answers['size'])
except KeyboardInterrupt:
    print 'exiting'
