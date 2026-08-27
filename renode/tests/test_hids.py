import clr
clr.AddReference("Renode")
from Antmicro.Renode.Core import EmulationManager
import System
import random
from System.Threading import Thread, ThreadStart

machine = EmulationManager.Instance.CurrentEmulation.Machines[0]
hids = machine["sysbus.i2c1.hids"]

def feed_data():
    while True:
        if not machine.IsPaused:
            try:
                # Generate random humidity and temperature
                # Humidity from 0 to 100%
                humidity = random.uniform(30.0, 60.0)
                # Temperature in Celsius
                temperature = random.uniform(20.0, 25.0)
                
                # set properties in C#
                hids.Humidity = System.Convert.ToDecimal(humidity)
                hids.Temperature = System.Convert.ToDecimal(temperature)
                
                print("HIDS updated -> Humidity: %.2f %%, Temperature: %.2f C" % (humidity, temperature))
            except Exception as e:
                print("HIDS test error: %s" % str(e))
        
        System.Threading.Thread.Sleep(1000)

t = Thread(ThreadStart(feed_data))
t.IsBackground = True
t.Start()
print("HIDS random data simulation started")
