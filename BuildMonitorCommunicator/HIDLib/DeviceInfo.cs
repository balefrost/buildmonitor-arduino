using System;

namespace HIDLib
{
    public class DeviceInfo
    {
        public readonly int flags;
        public readonly Guid interfaceClassGuid;
        public readonly string path;

        public DeviceInfo(int flags, Guid interfaceClassGuid, string path)
        {
            this.flags = flags;
            this.interfaceClassGuid = interfaceClassGuid;
            this.path = path;
        }

        public Device GetDevice()
        {
            return new Device(path);
        }
    }
}