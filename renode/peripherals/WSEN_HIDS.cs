using System;
using System.Collections.Generic;
using Antmicro.Renode.Core;
using Antmicro.Renode.Core.Structure.Registers;
using Antmicro.Renode.Peripherals.I2C;
using Antmicro.Renode.Peripherals.Sensor;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Utilities;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class WSEN_HIDS : II2CPeripheral, IProvidesRegisterCollection<ByteRegisterCollection>, ITemperatureSensor
    {
        public WSEN_HIDS()
        {
            RegistersCollection = new ByteRegisterCollection(this);
            DefineRegisters();
        }

        public void Reset()
        {
            RegistersCollection.Reset();
        }

        public void Write(byte[] data)
        {
            if(data.Length == 0)
            {
                return;
            }

            registerAddress = data[0];
            if(data.Length > 1)
            {
                for(int i = 1; i < data.Length; i++)
                {
                    RegistersCollection.Write(registerAddress, data[i]);
                    registerAddress++;
                }
            }
        }

        public byte[] Read(int count)
        {
            var result = new byte[count];
            for(int i = 0; i < count; i++)
            {
                result[i] = RegistersCollection.Read(registerAddress);
                registerAddress++;
            }
            return result;
        }

        public void FinishTransmission()
        {
        }

        public decimal Temperature { get; set; }
        public decimal Humidity { get; set; }

        public ByteRegisterCollection RegistersCollection { get; }

        private void DefineRegisters()
        {
            Registers.DeviceID.Define(RegistersCollection, 0xBC) // WHO_AM_I for WSEN-HIDS
                .WithTag("DEVICE_ID", 0, 8);

            Registers.CtrlReg1.Define(RegistersCollection, 0x00)
                .WithValueField(0, 8, name: "CTRL_REG1");

            Registers.CtrlReg2.Define(RegistersCollection, 0x00)
                .WithValueField(0, 8, name: "CTRL_REG2");

            Registers.StatusReg.Define(RegistersCollection, 0x03)
                .WithFlag(0, FieldMode.Read, name: "T_DA", valueProviderCallback: _ => true)
                .WithFlag(1, FieldMode.Read, name: "H_DA", valueProviderCallback: _ => true);

            Registers.TempOutL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "TEMP_OUT_L", valueProviderCallback: _ => 0x00);
            Registers.TempOutH.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "TEMP_OUT_H", valueProviderCallback: _ => 0x20);

            Registers.HumOutL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "HUM_OUT_L", valueProviderCallback: _ => 0x00);
            Registers.HumOutH.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "HUM_OUT_H", valueProviderCallback: _ => 0x40);
        }

        private byte registerAddress;

        private enum Registers : byte
        {
            DeviceID = 0x0F,
            CtrlReg1 = 0x20,
            CtrlReg2 = 0x21,
            StatusReg = 0x27,
            TempOutL = 0x28,
            TempOutH = 0x29,
            HumOutL = 0x2A,
            HumOutH = 0x2B,
        }
    }
}
