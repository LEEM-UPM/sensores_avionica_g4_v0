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
    public class WSEN_PADS : II2CPeripheral, IProvidesRegisterCollection<ByteRegisterCollection>, ITemperatureSensor
    {
        public WSEN_PADS()
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
                this.Log(LogLevel.Warning, "Unexpected write with no data");
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
        public decimal Pressure { get; set; }

        public ByteRegisterCollection RegistersCollection { get; }

        private void DefineRegisters()
        {
            Registers.DeviceID.Define(RegistersCollection, 0xB3) // WHO_AM_I for WSEN-PADS
                .WithTag("DEVICE_ID", 0, 8);

            Registers.CtrlReg1.Define(RegistersCollection, 0x00)
                .WithValueField(0, 8, name: "CTRL_REG1");

            Registers.CtrlReg2.Define(RegistersCollection, 0x10)
                .WithValueField(0, 8, name: "CTRL_REG2");

            Registers.StatusReg.Define(RegistersCollection, 0x03)
                .WithFlag(0, FieldMode.Read, name: "P_DA", valueProviderCallback: _ => true)
                .WithFlag(1, FieldMode.Read, name: "T_DA", valueProviderCallback: _ => true);

            Registers.PressOutXL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "PRESS_OUT_XL", valueProviderCallback: _ => (byte)(CalculateRawPressure() & 0xFF));
            Registers.PressOutL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "PRESS_OUT_L", valueProviderCallback: _ => (byte)((CalculateRawPressure() >> 8) & 0xFF));
            Registers.PressOutH.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "PRESS_OUT_H", valueProviderCallback: _ => (byte)((CalculateRawPressure() >> 16) & 0xFF));

            Registers.TempOutL.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "TEMP_OUT_L", valueProviderCallback: _ => (byte)(CalculateRawTemperature() & 0xFF));
            Registers.TempOutH.Define(RegistersCollection, 0x00).WithValueField(0, 8, FieldMode.Read, name: "TEMP_OUT_H", valueProviderCallback: _ => (byte)((CalculateRawTemperature() >> 8) & 0xFF));
        }

        private uint CalculateRawPressure()
        {
            return (uint)(Pressure * 4096m / 100m);
        }

        private ushort CalculateRawTemperature()
        {
            return (ushort)(Temperature * 100m);
        }

        private byte registerAddress;

        private enum Registers : byte
        {
            DeviceID = 0x0F,
            CtrlReg1 = 0x10,
            CtrlReg2 = 0x11,
            StatusReg = 0x27,
            PressOutXL = 0x28,
            PressOutL = 0x29,
            PressOutH = 0x2A,
            TempOutL = 0x2B,
            TempOutH = 0x2C,
        }
    }
}
