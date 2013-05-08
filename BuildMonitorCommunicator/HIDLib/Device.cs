using System;
using System.IO;
using HIDLib.Win32USB;
using Microsoft.Win32.SafeHandles;

namespace HIDLib
{
    public class Device : IDisposable
    {
        public readonly FileStream InputStream;
        public readonly FileStream OutputStream;
        private readonly SafeFileHandle deviceHandle;
        private bool disposed;

        public Device(string path)
        {
            bool useOverlappedIo = false;
            deviceHandle = USB.CreateFile(path,
                                          USB.GENERIC_READ | USB.GENERIC_WRITE,
                                          0, IntPtr.Zero, USB.OPEN_EXISTING,
                                          useOverlappedIo ? USB.FILE_FLAG_OVERLAPPED : 0,
                                          IntPtr.Zero);
            if (deviceHandle.IsInvalid)
            {
                throw new Exception("Invalid file handle");
            }

            IntPtr preparsedData;
            if (!USB.HidD_GetPreparsedData(deviceHandle, out preparsedData))
            {
                throw new Exception("Could not read from device");
            }

            HidCaps hidCaps;
            USB.HidP_GetCaps(preparsedData, out hidCaps);

            // extract the device capabilities from the internal buffer
            short inputReportLength = hidCaps.InputReportByteLength;
            short outputReportLength = hidCaps.OutputReportByteLength;

            Console.WriteLine("input: " + inputReportLength + " output: " + outputReportLength);

            if (inputReportLength > 0)
            {
                InputStream = new FileStream(deviceHandle, FileAccess.Read, inputReportLength,
                                             useOverlappedIo);
            }
            if (outputReportLength > 0)
            {
                OutputStream = new FileStream(deviceHandle, FileAccess.Write, outputReportLength,
                                              useOverlappedIo);
            }
        }


        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~Device()
        {
            Dispose(false);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposed)
                return;

            if (OutputStream != null)
            {
                OutputStream.Dispose();
            }
            if (InputStream != null)
            {
                InputStream.Dispose();
            }
            deviceHandle.Dispose();

            disposed = true;
        }
    }
}