import clr
clr.AddReference("Renode")
from Antmicro.Renode.Core import EmulationManager
import System
import random
from System.Threading import Thread, ThreadStart

machine = EmulationManager.Instance.CurrentEmulation.Machines[0]
baro2 = machine["sysbus.i2c2.baro2"]

def feed_data():
    while True:
        if not machine.IsPaused:
            try:
                # Generate random pressure and temperature
                # Normal pressure at sea level is ~101325 Pa.
                pressure = random.uniform(101000.0, 102000.0)
                # Temperature in Celsius
                temperature = random.uniform(20.0, 25.0)
                
                # set properties in C#
                baro2.Pressure = System.Convert.ToDecimal(pressure)
                baro2.Temperature = System.Convert.ToDecimal(temperature)
                
                print("Baro2 updated -> Pressure: %.2f Pa, Temperature: %.2f C" % (pressure, temperature))
            except Exception as e:
                print("Baro2 test error: %s" % str(e))
        
        System.Threading.Thread.Sleep(1000)

t = Thread(ThreadStart(feed_data))
t.IsBackground = True
t.Start()
print("Baro2 random data simulation started")
