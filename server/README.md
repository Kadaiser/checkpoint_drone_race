# Contenido
1. [Servidor DHCP para Raspberry Pi model 3](#part1)
2. [Instalacción de mosquitto en Raspberry Pi model 3](#part2)
2. [Configuracion para el inicio de una carrera](#part3)

## **Servidor DHCP para Raspberry Pi model 3 <a name="part1"></a>**
### Actualizar los paquetes e instalar hostapd y dnsmasq
```
sudo apt-get update
sudo apt-get -y install hostapd dnsmasq
```

### Actualizar el fichero dhcp.conf

```
sudo nano /etc/dhcpcd.conf
```
Escribir al final del fichero

```
denyinterfaces wlan0
```
### Actualizar el fichero interfaces
```
sudo nano /etc/network/interfaces
```
Escribir al final del fichero

```
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp

allow-hotplug wlan0
iface wlan0 inet static
    address 192.168.4.1
    netmask 255.255.255.0
    network 192.168.4.0
    broadcast 192.168.4.255
```

### Actualizar o crear el fichero hostapd.conf
```
sudo nano /etc/hostapd/hostapd.conf
```
Escribir al final del fichero
```
interface=wlan0
driver=nl80211
ssid=dronrace
hw_mode=g
channel=6
ieee80211n=1
wmm_enabled=1
ht_capab=[HT40][SHORT-GI-20][DSSS_CCK-40]auth_algs=1
ignore_broadcast_ssid=0
wpa=2
macaddr_acl=0

wpa_key_mgmt=WPA-PSK
wpa_passphrase=4thewin!
rsn_pairwise=CCMP
```

### Actualizar o crear el  /etc/default/hostapd

```
sudo nano /etc/default/hostapd
```
Buscar en el fichero la siguiente cadena "DAEMON_CONF" y remplazar por
```
DAEMON_CONF="/etc/hostapd/hostapd.conf"
```
### Reiniciar el servidor

```
sudo reboot
```

## **Instalacción de mosquitto en Raspberry Pi model 3 <a name="part2"></a>**

```
sudo apt update
sudo apt install -y mosquitto mosquitto-clients
sudo systemctl enable mosquitto.service
```

## **Configuracion para el inicio de una carrera <a name="part3"></a>**
### Requisitos
Debe estar instalado python en el servidor (raspberrypi) y las siguientes liberías
```
pip install inquirer
pip install paho-mqtt
```
Para iniciar la configuracion de la carrera, es decir, definir los corredores y las balizas debemos ejecutar el siguiente comando
```
python server.py
```
Al inicar el servicio nos informará si se ha conseguido conectar al broker de MQTT. En caso de ser asi nos pemitirá realizar 3 operaciones:
1. **Registrar un corredor**: definir la direccion MAC del Sensortag que llevará el dron
2. **Registrar un checkpoint**: definir la posicion del checkpoint o baliza
2. **Iniciar carrera**: A partir de este punto se empezará a leer todos los mensajes generados por las balizas (cada vez que un corredor es dectado por una baliza)
