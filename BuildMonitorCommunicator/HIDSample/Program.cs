using System;
using HIDLib;
using HIDLib.Win32USB;

namespace HIDSample
{
    internal class Program
    {
        private const uint VendorId = 0x03EB;
        private const uint ProductId = 0x204F;

        private static void Main(string[] args)
        {
            string strSearch = string.Format("vid_{0:x4}&pid_{1:x4}", VendorId, ProductId);
            Guid hidGuid = USB.HIDGuid;
            Console.WriteLine(hidGuid);

            using (DeviceInfoSet infoSet = DeviceInfoSet.GetByClass(hidGuid))
            {
                Console.WriteLine(infoSet);

                foreach (DeviceInfo info in infoSet)
                {
                    Console.WriteLine("Considering " + info.path);
                    if (info.path.IndexOf(strSearch) >= 0)
                    {
                        using (Device d = info.GetDevice())
                        {
                            var toWrite = new byte[4];
                            toWrite[0] = 0x00;
                            toWrite[1] = 0x00;
                            toWrite[2] = 0xff;
                            toWrite[3] = 0xff;
                            d.OutputStream.Write(toWrite, 0, toWrite.Length);
                        }
                        break;
                    }
                }
            }

            Console.WriteLine("Press any key to continue...");
            Console.ReadKey();
        }
    }
}