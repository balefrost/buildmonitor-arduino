using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace HIDSample
{
    internal class Program
    {
        private const uint VendorId = 0x03EB;
        private const uint ProductId = 0x204F;

        private static string GetDevicePath(IntPtr hInfoSet, ref DeviceInterfaceData oInterface)
        {
            uint nRequiredSize = 0;
            // Get the device interface details
            if (!Win32Usb.SetupDiGetDeviceInterfaceDetail(hInfoSet, ref oInterface, IntPtr.Zero, 0, ref nRequiredSize, IntPtr.Zero))
            {
                DeviceInterfaceDetailData oDetail = new DeviceInterfaceDetailData();
                oDetail.Size = 5;
                if (Win32Usb.SetupDiGetDeviceInterfaceDetail(hInfoSet, ref oInterface, ref oDetail, nRequiredSize, ref nRequiredSize, IntPtr.Zero))
                {
                    return oDetail.DevicePath;
                }
            }
            return null;
        }

        private static void Main(string[] args)
        {
            string strSearch = string.Format("vid_{0:x4}&pid_{1:x4}", VendorId, ProductId);
            Guid hidGuid = Win32Usb.HIDGuid;
            Console.WriteLine(hidGuid);
            IntPtr hInfoSet = Win32Usb.SetupDiGetClassDevs(ref hidGuid, null, IntPtr.Zero, Win32Usb.DIGCF_DEVICEINTERFACE | Win32Usb.DIGCF_PRESENT);
            Console.WriteLine(hInfoSet);

            try
            {
                DeviceInterfaceData oInterface = new DeviceInterfaceData();
                oInterface.Size = Marshal.SizeOf(oInterface);
                uint nIndex = 0;
                while (Win32Usb.SetupDiEnumDeviceInterfaces(hInfoSet, 0, ref hidGuid, nIndex, ref oInterface))
                {
                    string devicePath = GetDevicePath(hInfoSet, ref oInterface);
                    Console.WriteLine("Considering " + devicePath);
                    if (devicePath.IndexOf(strSearch) >= 0)
                    {
                        SafeFileHandle deviceHandle = Win32Usb.CreateFile(devicePath, Win32Usb.GENERIC_READ | Win32Usb.GENERIC_WRITE, 0, IntPtr.Zero, Win32Usb.OPEN_EXISTING, 0 /*Win32Usb.FILE_FLAG_OVERLAPPED*/, IntPtr.Zero);
                        if (deviceHandle.IsInvalid)
                        {
                            throw new Exception("Invalid file handle");
                        }

                        IntPtr dataBuffer;
                        if (!Win32Usb.HidD_GetPreparsedData(deviceHandle, out dataBuffer))
                        {
                            throw new Exception("Could not read from device");
                        }

                        HidCaps hidCaps;
                        Win32Usb.HidP_GetCaps(dataBuffer, out hidCaps);	// extract the device capabilities from the internal buffer
                        short inputReportLength = hidCaps.InputReportByteLength;
                        short outputReportLength = hidCaps.OutputReportByteLength;

                        Console.WriteLine("input: " + inputReportLength + " output: " + outputReportLength);

                        using (
                            var f = new FileStream(deviceHandle, FileAccess.Read | FileAccess.Write, outputReportLength,
                                                   false /*true*/))
                        {
                            var toWrite = new byte[outputReportLength];
                            toWrite[0] = 0x00;
                            toWrite[1] = 0x00;
                            toWrite[2] = 0xff;
                            toWrite[3] = 0xff;
                            f.Write(toWrite, 0, toWrite.Length);
                        }
                        break;
                    }
                    nIndex++;
                }
            }
            finally
            {
                Win32Usb.SetupDiDestroyDeviceInfoList(hInfoSet);
            }
            Console.WriteLine("Press any key to continue...");
            Console.ReadKey();
        }
    }
}