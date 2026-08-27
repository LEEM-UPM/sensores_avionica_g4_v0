import clr
clr.AddReference("Renode")
from Antmicro.Renode.Core import EmulationManager
import System
import random
from System.Threading import Thread, ThreadStart

machine = EmulationManager.Instance.CurrentEmulation.Machines[0]
gps_uart = machine["sysbus.lpuart1"]

def checksum(sentence):
    calc_cksum = 0
    for char in sentence:
        calc_cksum ^= ord(char)
    return "%02X" % calc_cksum

def feed_data():
    while True:
        if not machine.IsPaused:
            try:
                # Generate random latitude and longitude slightly varying
                lat = 4807.000 + random.uniform(0.0, 0.1)
                lon = 1131.000 + random.uniform(0.0, 0.1)
                
                # Form NMEA sentence (without $ and checksum)
                nmea_body = "GPGGA,123519,{:.3f},N,{:.3f},E,1,08,0.9,545.4,M,46.9,M,,".format(lat, lon)
                
                # Calculate checksum
                cksum = checksum(nmea_body)
                
                # Form complete sentence
                nmea_sentence = "${}*{}\r\n".format(nmea_body, cksum)
                
                print("GPS sent -> %s" % nmea_sentence.strip())
                
                # Send character by character
                for char in nmea_sentence:
                    gps_uart.WriteChar(ord(char))
                    System.Threading.Thread.Sleep(1)
                    
            except Exception as e:
                print("GPS test error: %s" % str(e))
        
        System.Threading.Thread.Sleep(1000)

t = Thread(ThreadStart(feed_data))
t.IsBackground = True
t.Start()
print("GPS random data simulation started")
