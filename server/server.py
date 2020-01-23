import paho.mqtt.client as mqtt
import time
import inquirer
import json
import operator

MQTT_SERVER = "192.168.4.1";
MQTT_PORT = 1883

receivedMessages = []
racersRute = {}
checkpoints = []
finalCheckPointsRace = []
numVueltas = 1

only_listen = False
connecto_broker = False

TOPIC_COMUN = 'dronrace/'

TOPIC_REGISTRAR_CORREDOR = TOPIC_COMUN + 'regRunner'
TOPIC_CHECK_POINT = 'setCheckpoint'
TOPIC_RECONNECTIONS = TOPIC_COMUN + 'reconnnections'
TOPIC_CHECKPOINT_VALUE = TOPIC_COMUN + 'checkpointValue'
TOPIC_RUNNER_DETECTED = TOPIC_COMUN + 'runnerDetected'


questions = [
  inquirer.List('size',
                message='Operacion desea realizar',
                choices=['1 - Registrar un corredor', '2 - Registrar un checkpoint', '3 - Numero de vueltas','4 - Iniciar carrera'],
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


def addToLogsRunnerDectected(strMsg):
    data = json.loads(strMsg)
    if data.runner in racersRute:
        value = racersRute.get(data.runner)
        racersRute[data.runner] = value.append(data.checkpoint)
    else:
        print("No se ha encontrado definido el corredor con IDENTIFICADOR: " + data.runner)


# callback function to subscribe callback messsage event

def on_message(client, userdata, message):
    print("********** Message received ******************")
    payload = str(message.payload)
    topic = message.topic
    print("Topic name: " + topic + "\nMensage: " + payload)
    print("**********************************************")
    if topic == TOPIC_RUNNER_DETECTED:
        addToLogsRunnerDectected(payload)

def on_publish(client, userdata, mid):
  receivedMessages.append(mid)

# publish topic message
def publish(topic, message, waitForAck = False):
  mid = client.publish(topic, message, 2)[1]
  if (waitForAck):
    while mid not in receivedMessages:
      time.sleep(0.25)

def registrarCorredor():
    global racersRute
    print("********** Corredores registrados ******************")
    print(racersRute)
    print("**********************************************")
    mac = raw_input('MAC Sensortag: ')
    print('Registrado el corredor con identificador: ' + mac)
    msg = json.dumps({'runner': mac})
    publish(TOPIC_REGISTRAR_CORREDOR, msg, True)
    print(TOPIC_REGISTRAR_CORREDOR, msg, True)
    racersRute[mac] = []

def registrarCheckPoint():
    global checkpoints
    print("********** Balizas registradas ******************")
    print(checkpoints)
    print("**********************************************")
    checkPointName = raw_input('Nombre de la baliza: ')
    postion = int(input('Posicion de la baliza: '))
    print('Registrada la baliza con nombre ' + checkPointName + '. Position: ' + str(postion))
    msg = json.dumps({'value': postion})
    topic = TOPIC_COMUN  + checkPointName + '/' + TOPIC_CHECK_POINT
    publish(topic, msg, True)
    print(topic, msg, True)
    tuple = (postion, checkPointName)
    if (tuple in checkpoints) == False:
        checkpoints = checkpoints + [tuple]

def registrarNumvueltas():
    global numVueltas
    numVueltas = int(input('Numero vueltas: '))
    print("********** Numero de vueltas al circuito ******************")
    print(' Vueltas: ' + str(numVueltas))
    print("**********************************************")

def printFinalRunners():
    global racersRute
    print("********** Corredores finales registrados ******************")
    print(racersRute)
    print("**********************************************")

def printFinalCheckPoints():
    global checkpoints
    checkpoints.sort(key = operator.itemgetter(1))
    print("********** Balizas finales registradas ******************")
    print(checkpoints)
    print("**********************************************")

def printFinalTrace():
    global checkpoints
    global numVueltas
    global finalCheckPointsRace
    checkpoints.sort(key = operator.itemgetter(1))
    recorrido = [ name for (pos, name) in checkpoints]
    finalCheckPointsRace = recorrido
    i = 1
    if numVueltas > 1:
        while i < numVueltas:
            finalCheckPointsRace = finalCheckPointsRace + recorrido
            i = i + 1
    print("********** Recorrido final del circuito por las balizas ******************")
    print(finalCheckPointsRace)
    print("**********************************************")

def inicarSubscripcionCheckPoints():
    printFinalRunners()
    printFinalCheckPoints()
    printFinalTrace()
    client.subscribe(TOPIC_RUNNER_DETECTED)
    client.subscribe(TOPIC_CHECKPOINT_VALUE)
    client.subscribe(TOPIC_RECONNECTIONS)
    print("El servidor se encuentra escuchando los siguientes TOPICS: ", TOPIC_RUNNER_DETECTED, TOPIC_CHECKPOINT_VALUE, TOPIC_RECONNECTIONS)
    global only_listen
    only_listen = True

def toPublish(operation):
    if operation == '1 - Registrar un corredor':
        registrarCorredor()
    elif operation == '2 - Registrar un checkpoint':
        registrarCheckPoint()
    elif operation == '3 - Numero de vueltas':
        registrarNumvueltas()
    elif operation == '4 - Iniciar carrera':
        inicarSubscripcionCheckPoints()
    else:
        print('No esta implementada esta operacion')

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
            time.sleep(2)
            print('Ctrl+C para finalizar la carrera: ')
        else:
            answers = inquirer.prompt(questions)
            toPublish(answers['size'])
except KeyboardInterrupt:

    print 'exiting'
