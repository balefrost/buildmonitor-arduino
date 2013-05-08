using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using HIDLib.Win32USB;

namespace HIDLib
{
    public class DeviceInfoSet : IDisposable, IEnumerable<DeviceInfo>
    {
        private readonly Guid classGuid;
        private bool disposed;
        private IntPtr handle;

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

            USB.SetupDiDestroyDeviceInfoList(handle);
            handle = IntPtr.Zero;

            disposed = true;
        }

        public static DeviceInfoSet GetByClass(Guid classGuid)
        {
            IntPtr handle = USB.SetupDiGetClassDevs(ref classGuid, null, IntPtr.Zero,
                                                    USB.DIGCF_DEVICEINTERFACE | USB.DIGCF_PRESENT);
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
                if (USB.SetupDiEnumDeviceInterfaces(deviceInfoSet.handle, 0, ref classGuid, nextIndex,
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
                USB.SetupDiGetDeviceInterfaceDetail(deviceInfoSetHandle, ref deviceInterfaceData, IntPtr.Zero, 0,
                                                    ref nRequiredSize, IntPtr.Zero);

                var oDetail = new DeviceInterfaceDetailData {Size = 5};

                if (USB.SetupDiGetDeviceInterfaceDetail(deviceInfoSetHandle, ref deviceInterfaceData, ref oDetail,
                                                        nRequiredSize, ref nRequiredSize, IntPtr.Zero))
                {
                    return oDetail.DevicePath;
                }

                throw new Exception("Win32 Error");
            }
        }
    }
}