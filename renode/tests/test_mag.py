import clr
clr.AddReference("Renode")
from Antmicro.Renode.Core import EmulationManager
import System
import random
from System.Threading import Thread, ThreadStart

machine = EmulationManager.Instance.CurrentEmulation.Machines[0]
mag = machine["sysbus.i2c1.mag"]

def feed_data():
    while True:
        if not machine.IsPaused:
            try:
                mag.MagneticFieldX = x = random.randint(-5000, 5000)
                mag.MagneticFieldY = y = random.randint(-5000, 5000)
                mag.MagneticFieldZ = z = random.randint(-5000, 5000)
                print("MAG sent -> X: %d, Y: %d, Z: %d" % (x, y, z))
            except Exception as e:
                pass
        System.Threading.Thread.Sleep(100)

t = Thread(ThreadStart(feed_data))
t.IsBackground = True
t.Start()
print("MAG random data simulation started")
