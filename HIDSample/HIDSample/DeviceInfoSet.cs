using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace HIDSample
{
    public class DeviceInfoSet : IDisposable, IEnumerable<DeviceInfo>
    {
        private readonly Guid classGuid;
        private IntPtr handle;
        private bool disposed;

        private DeviceInfoSet(IntPtr handle, Guid classGuid)
        {
            this.handle = handle;
            this.classGuid = classGuid;
        }


        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        public IEnumerator<DeviceInfo> GetEnumerator()
        {
            return new DeviceInfoEnumerator(this);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        ~DeviceInfoSet()
        {
            Dispose(false);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposed)
                return;

            Win32Usb.SetupDiDestroyDeviceInfoList(handle);
            handle = IntPtr.Zero;

            disposed = true;
        }

        public static DeviceInfoSet GetByClass(Guid classGuid)
        {
            IntPtr handle = Win32Usb.SetupDiGetClassDevs(ref classGuid, null, IntPtr.Zero,
                                                         Win32Usb.DIGCF_DEVICEINTERFACE | Win32Usb.DIGCF_PRESENT);
            if (handle != IntPtr.Zero)
            {
                return new DeviceInfoSet(handle, classGuid);
            }
            throw new Exception("Win32 Error");
        }

        private class DeviceInfoEnumerator : IEnumerator<DeviceInfo>
        {
            private readonly DeviceInfoSet deviceInfoSet;
            private DeviceInfo current;
            private uint nextIndex;

            public DeviceInfoEnumerator(DeviceInfoSet deviceInfoSet)
            {
                this.deviceInfoSet = deviceInfoSet;
            }

            public void Dispose()
            {
            }

            public bool MoveNext()
            {
                Guid classGuid = deviceInfoSet.classGuid;
                var deviceInterfaceData = new DeviceInterfaceData();
                deviceInterfaceData.Size = Marshal.SizeOf(deviceInterfaceData);
                if (Win32Usb.SetupDiEnumDeviceInterfaces(deviceInfoSet.handle, 0, ref classGuid, nextIndex,
                                                         ref deviceInterfaceData))
                {
                    string path = GetDevicePath(deviceInfoSet.handle, ref deviceInterfaceData);
                    current = new DeviceInfo(deviceInterfaceData.Flags, deviceInterfaceData.InterfaceClassGuid, path);
                    ++nextIndex;
                    return true;
                }

                return false;
            }

            public void Reset()
            {
                nextIndex = 0;
            }

            object IEnumerator.Current
            {
                get { return Current; }
            }

            public DeviceInfo Current
            {
                get { return current; }
            }

            private static string GetDevicePath(IntPtr deviceInfoSetHandle, ref DeviceInterfaceData deviceInterfaceData)
            {
                uint nRequiredSize = 0;
                Win32Usb.SetupDiGetDeviceInterfaceDetail(deviceInfoSetHandle, ref deviceInterfaceData, IntPtr.Zero, 0,
                                                         ref nRequiredSize, IntPtr.Zero);

                var oDetail = new DeviceInterfaceDetailData {Size = 5};

                if (Win32Usb.SetupDiGetDeviceInterfaceDetail(deviceInfoSetHandle, ref deviceInterfaceData, ref oDetail,
                                                             nRequiredSize, ref nRequiredSize, IntPtr.Zero))
                {
                    return oDetail.DevicePath;
                }

                throw new Exception("Win32 Error");
            }
        }
    }
}