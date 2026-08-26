import clr
clr.AddReference("Renode")
from Antmicro.Renode.Core import EmulationManager
import System
import random
from System.Threading import Thread, ThreadStart

machine = EmulationManager.Instance.CurrentEmulation.Machines[0]
imu2 = machine["sysbus.i2c2.imu2"]

def feed_data():
    while True:
        if not machine.IsPaused:
            try:
                imu2.AccelerationX = x = random.randint(-32000, 32000)
                imu2.AccelerationY = y = random.randint(-32000, 32000)
                imu2.AccelerationZ = z = random.randint(-32000, 32000)
                print("IMU2 sent -> X: %d, Y: %d, Z: %d" % (x, y, z))
            except Exception as e:
                pass
        System.Threading.Thread.Sleep(100)

t = Thread(ThreadStart(feed_data))
t.IsBackground = True
t.Start()
print("IMU2 random data simulation started")
