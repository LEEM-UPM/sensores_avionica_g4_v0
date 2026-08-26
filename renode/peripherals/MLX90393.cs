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
    public class MLX90393 : II2CPeripheral, IProvidesRegisterCollection<ByteRegisterCollection>, ITemperatureSensor
    {
        public MLX90393()
        {
            IRQ = new GPIO();
            RegistersCollection = new ByteRegisterCollection(this);
            DefineRegisters();
        }

        public GPIO IRQ { get; private set; }

        public void Reset()
        {
            RegistersCollection.Reset();
        }

        public void Write(byte[] data)
        {
            if(data.Length == 0) return;
            registerAddress = data[0];
            for(int i = 1; i < data.Length; i++)
            {
                RegistersCollection.Write(registerAddress, data[i]);
                registerAddress++;
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

        public void FinishTransmission() {}

        public decimal Temperature { get; set; }
        public short MagneticFieldX { get; set; }
        public short MagneticFieldY { get; set; }
        public short MagneticFieldZ { get; set; }

        public ByteRegisterCollection RegistersCollection { get; }

        private void DefineRegisters()
        {
            Registers.Status.Define(RegistersCollection, 0x00).WithValueField(0, 8, name: "STATUS");

            Registers.OutX_L.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTX_L", valueProviderCallback: _ => (byte)(MagneticFieldX & 0xFF));
            Registers.OutX_H.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTX_H", valueProviderCallback: _ => (byte)((MagneticFieldX >> 8) & 0xFF));
            
            Registers.OutY_L.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTY_L", valueProviderCallback: _ => (byte)(MagneticFieldY & 0xFF));
            Registers.OutY_H.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTY_H", valueProviderCallback: _ => (byte)((MagneticFieldY >> 8) & 0xFF));

            Registers.OutZ_L.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTZ_L", valueProviderCallback: _ => (byte)(MagneticFieldZ & 0xFF));
            Registers.OutZ_H.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTZ_H", valueProviderCallback: _ => (byte)((MagneticFieldZ >> 8) & 0xFF));
        }

        private byte registerAddress;

        private enum Registers : byte
        {
            Status = 0x00,
            OutX_L = 0x01, OutX_H = 0x02,
            OutY_L = 0x03, OutY_H = 0x04,
            OutZ_L = 0x05, OutZ_H = 0x06,
        }
    }
}
