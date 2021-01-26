#!/usr/bin/env python3
# **********************************************************************************
# This script will handle the transmission of a compiled sketch in the
# form of an INTEL HEX flash image to an attached gateway/master Moteino node,
# for further wireless transmission to a target Moteino node that will receive it de-HEXified and
# store it in external memory. Once received by the target (which is also loaded with a custom bootloader
# capable of reading back that image) it will reset and reprogram itself with the new sketch
#
# EXAMPLE command line: python WirelessProgramming.py -f PathToFile.hex -s COM100 -t 123
# where -t is the target ID of the Moteino you are programming
# and -s is the serial port of the programmer Moteino (on linux/osx it is something like ttyAMA0)
# To get the .hex file path go to Arduino>file>preferences and check the verbosity for compilation
#   then you will get the path in the debug status area once the sketch compiles
# **********************************************************************************
# Copyright Felix Rusu, LowPowerLab.com
# Library and code by Felix Rusu - lowpowerlab.com/contact
# **********************************************************************************
# License
# **********************************************************************************
# This program is free software; you can redistribute it
# and/or modify it under the terms of the GNU General
# Public License as published by the Free Software
# Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE. See the GNU General Public
# License for more details.
#
# You should have received a copy of the GNU General
# Public License along with this program.
# If not, see <http://www.gnu.org/licenses/>.
#
# Licence can be viewed at
# http://www.gnu.org/licenses/gpl-3.0.txt
#
# Please maintain this license information along with authorship
# and copyright notices in any redistribution of this code
# **********************************************************************************
import time
import sys
import collections
import re
import paho.mqtt.client as mqtt
import os
import struct

from sys import exit

### GENERAL SETTINGS ###
MQTT_HOST = "localhost"  # the default mqtt server connected to
MQTT_PWD = ''            # password of MQTT
MQTT_USER = ''           # user of MQTT
HEX = "flash.hex"        # the HEX file containing the new program for the MQTT_USER
LINESPERPACKET = 8      # HEX lines to send per MQTT packet (every line need 16byte of memory)
TIMEOUT = 5              # Seconds of timeout
retries = 2

# Read command line arguments
if (sys.argv and len(sys.argv) > 1):
    if len(sys.argv) == 2 and (sys.argv[1] == "-h" or sys.argv[1] == "-help" or sys.argv[1] == "?"):
        print(" -f or -file                HEX file to upload (Default: ", HEX, ")")
        print(
            " -l or -lines {lines}       HEX lines per MQTT packet (Default: 8)")
        print(
            " -m or -mqtt {server}       Specify the mqtt server  (Default: ", MQTT_HOST, ")")
        print(
            " -u or -user {user}         Specify the user to connect to MQTT")
        print(" -p or -password {password} Specify the MQTT password")
        print(
            " -t or -timeout {secs}      Specify the OTA ACK timeout (Default 5s)")
        print(" -h or -help or ?           Print this message")
        exit(0)

    for i in range(len(sys.argv)):
        if (sys.argv[i] == "-m" or sys.argv[i] == "-mqtt") and len(sys.argv) >= i+2:
            MQTT_HOST = sys.argv[i+1]
        if (sys.argv[i] == "-p" or sys.argv[i] == "-password") and len(sys.argv) >= i+2:
            MQTT_PWD = sys.argv[i+1]
        if (sys.argv[i] == "-f" or sys.argv[i] == "-file") and len(sys.argv) >= i+2:
            HEX = sys.argv[i+1].strip()
        if (sys.argv[i] == "-u" or sys.argv[i] == "-user") and len(sys.argv) >= i+2:
            MQTT_USER = sys.argv[i+1]
        if (sys.argv[i] == "-l" or sys.argv[i] == "-lines") and len(sys.argv) >= i+2:
            if sys.argv[i+1].isdigit() and int(sys.argv[i+1]) >= 1:
                LINESPERPACKET = int(sys.argv[i+1])
            else:
                print("LINESPERPACKET invalid  (",
                      sys.argv[i+1], "), must be >1.")
                exit(1)
        if (sys.argv[i] == "-t" or sys.argv[i] == "-timeout") and len(sys.argv) >= i+2:
            if sys.argv[i+1].isdigit() and int(sys.argv[i+1]) > 1:
                TIMEOUT = int(sys.argv[i+1])
            else:
                print("TIMEOUR invalid  (", sys.argv[i+1], "), must be > 1.")
                exit(1)

class UpdateProgress(object):
    progress = 0
    symbol = {0:' ',1:'|',2:'/',3:'-',4:'\\'}
    current = 0
    def __call__(self, progress = -1 ):
        size = 40
        if(progress == -1):
            progress = self.progress
        else:
            self.progress = progress
        if(self.progress == progress):
            self.current = (self.current+1) % len(self.symbol)
        else:
            self.current = 0    
        sys.stdout.write('\u001b[1000DUploading [{0}{1}{2}] {3}%'.format('#'*int(progress/(100/size)),self.symbol[self.current],' '*(size-int(progress/(100/size))-1), progress))
        sys.stdout.flush()

update_progress = UpdateProgress()

def LOG(message):
    sys.stdout.write(message)
    sys.stdout.flush()


def LOGln(message):
    sys.stdout.write('\n' + message)


def millis():
    return int(round(time.time() * 1000))


HANDSHAKE_OK = 0
HANDSHAKE_FAIL = 1
HANDSHAKE_FAIL_TIMEOUT = 2
HANDSHAKE_ERROR = 3
HANDSHAKE_FAIL_ERROR = 4


def on_connect(client, userdata, flags, rc):
    if rc != 0:
        # 0: Connection successful
        # 1: Connection refused - incorrect protocol version
        # 2: Connection refused - invalid client identifier
        # 3: Connection refused - server unavailable
        # 4: Connection refused - bad username or password
        # 5: Connection refused - not authorised
        # 6-255: Currently unused.
        LOGln("Error MQTT connect:"+rc)
        exit(1)
    client.subscribe("clerct/mn/otak")


ack = 10


def on_message(client, userdata, msg):
    global ack
#    print(ack, msg.topic+" "+str(msg.payload))
    if msg.topic == "clerct/mn/otak":
      if msg.payload == b'K':
        ack = -1
      else:
        ack = -2


start = millis()


def waitAck():
    global ack
    global start
    ack = millis()
    while(ack > 0 and ack-start < TIMEOUT*1000):
        update_progress()
        ack = millis()
        time.sleep(1)
    return ack


class TimeoutError(Exception):
    pass


def sendWithAck(topic, message):
    global ack
    global start
    start = millis()
    rty = retries
    ack = 0
    while(rty > 0 and ack >= 0):    
        client.publish(topic, message+bytes(0))
        waitAck()
        rty -= 1
    if ack == -1:
        return True
    else:
        raise TimeoutError("Fatal:Timeout on sending message", topic, message.hex())


# MAIN()
# if __name__ == "__main__":

client = mqtt.Client()
client.username_pw_set(MQTT_USER, MQTT_PWD)
client.on_connect = on_connect
client.on_message = on_message
client.enable_logger(logger=None)
client.connect(MQTT_HOST, 1883, 60)
client.loop_start()

while client.is_connected() != True:
    time.sleep(1)

try:
    with open(HEX) as f:

        seq = 0
        packetCounter = 0
        content = f.readlines()
        LOGln("File found, passing to OTA Programmer... Blocks:"+str(len(content))+"\n")
#        LOGln("TX > B<"+str(len(content))+">")          
        sendWithAck("clerct/mn/ota", bytes('B',"ASCII") + len(content).to_bytes(2, 'little'))  # B<EpromSize>


        while seq < len(content):
            currentLine = content[seq].strip()
            hexDataToSend= currentLine[9:len(currentLine)-2]
            isEOF = (content[seq].strip() == ":00000001FF")
            bundledLines = 1
            while isEOF != True and bundledLines < LINESPERPACKET:
#                LOGln(str(seq)+"/"+str(len(content))+" "+str(bundledLines))
                isEOF = (content[seq+bundledLines].strip() == ":00000001FF")
                if isEOF != True:
                    currentLine = content[seq+bundledLines].strip()
                    hexDataToSend = hexDataToSend + currentLine[9:len(currentLine)-2]
                    bundledLines += 1
            
            tx = bytes('C',"ASCII")+(len(content)-seq).to_bytes(2,'little') + bytes.fromhex(hexDataToSend)
#            LOGln("TX > "+tx.hex()+" ["+str(int(len(hexDataToSend)/2))+"]")
            update_progress(int(seq*100/len(content)))
             
            if len(hexDataToSend) > 0: 
                sendWithAck("clerct/mn/ota", tx)
            
            seq += bundledLines
            packetCounter += 1

        sendWithAck("clerct/mn/ota", bytes('E',"ASCII")+packetCounter.to_bytes(2, 'little'))
        update_progress(100)
        LOGln("\nSuccess!")
except IOError:
    LOGln("File [" + HEX + "] not found, exiting...")
    exit(1)
except TimeoutError as timeout:
    print(timeout)
    exit(1)

finally:
    client.disconnect()
