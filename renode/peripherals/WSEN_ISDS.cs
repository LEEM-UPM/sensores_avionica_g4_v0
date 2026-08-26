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
    public class WSEN_ISDS : II2CPeripheral, IProvidesRegisterCollection<ByteRegisterCollection>, ITemperatureSensor
    {
        public WSEN_ISDS()
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
        public short AccelerationX { get; set; }
        public short AccelerationY { get; set; }
        public short AccelerationZ { get; set; }

        public ByteRegisterCollection RegistersCollection { get; }

        private void DefineRegisters()
        {
            Registers.DeviceID.Define(RegistersCollection, 0x6A).WithTag("DEVICE_ID", 0, 8);
            Registers.Ctrl1Xl.Define(RegistersCollection, 0x00).WithValueField(0, 8, name: "CTRL1_XL");
            Registers.Ctrl2G.Define(RegistersCollection, 0x00).WithValueField(0, 8, name: "CTRL2_G");

            Registers.OutX_L_XL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTX_L_XL", valueProviderCallback: _ => (byte)(AccelerationX & 0xFF));
            Registers.OutX_H_XL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTX_H_XL", valueProviderCallback: _ => (byte)((AccelerationX >> 8) & 0xFF));
            
            Registers.OutY_L_XL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTY_L_XL", valueProviderCallback: _ => (byte)(AccelerationY & 0xFF));
            Registers.OutY_H_XL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTY_H_XL", valueProviderCallback: _ => (byte)((AccelerationY >> 8) & 0xFF));

            Registers.OutZ_L_XL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTZ_L_XL", valueProviderCallback: _ => (byte)(AccelerationZ & 0xFF));
            Registers.OutZ_H_XL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "OUTZ_H_XL", valueProviderCallback: _ => (byte)((AccelerationZ >> 8) & 0xFF));
        }

        private byte registerAddress;

        private enum Registers : byte
        {
            DeviceID = 0x0F,
            Ctrl1Xl = 0x10,
            Ctrl2G = 0x11,
            OutX_L_XL = 0x28, OutX_H_XL = 0x29,
            OutY_L_XL = 0x2A, OutY_H_XL = 0x2B,
            OutZ_L_XL = 0x2C, OutZ_H_XL = 0x2D,
        }
    }
}
