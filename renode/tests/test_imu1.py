import clr
clr.AddReference("Renode")
from Antmicro.Renode.Core import EmulationManager
import System
import random
from System.Threading import Thread, ThreadStart

machine = EmulationManager.Instance.CurrentEmulation.Machines[0]
imu1 = machine["sysbus.i2c1.imu1"]

def feed_data():
    while True:
        if not machine.IsPaused:
            try:
                imu1.AccelerationX = x = random.randint(-32000, 32000)
                imu1.AccelerationY = y = random.randint(-32000, 32000)
                imu1.AccelerationZ = z = random.randint(-32000, 32000)
                print("IMU1 sent -> X: %d, Y: %d, Z: %d" % (x, y, z))
            except Exception as e:
                pass
        System.Threading.Thread.Sleep(100)

t = Thread(ThreadStart(feed_data))
t.IsBackground = True
t.Start()
print("IMU1 random data simulation started")
