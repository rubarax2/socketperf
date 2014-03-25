using System;
using System.IO;

namespace socketperf
{
    class Constants
    {
        internal const int PORT = 8087;
        internal const int BUFFER_SIZE = 1024 * 1024;
    }

    class LengthSerializer
    {
        internal static void Serialize(Stream stream, uint length)
        {
            byte[] buffer = Serialize(length);
            stream.Write(buffer, 0, buffer.Length);
        }

        internal static byte[] Serialize(uint length)
        {
            return BitConverter.GetBytes(length);
        }

        internal static uint Deserialize(Stream stream)
        {
            byte[] buffer = new byte[4];
            int read = stream.Read(buffer, 0, buffer.Length);

            if (read != 4)
                throw new Exception("The length header wasn't correctly read");

            return BitConverter.ToUInt32(buffer, 0);
        }
    }

    class LogPrinter
    {
        internal static void LogTime(string header, uint length, int ini)
        {
            float totalMB = (float)length / (1024 * 1024);
            int time = Environment.TickCount - ini;
            float timeSecs = (float)time / 1000;
            float speed = totalMB / timeSecs;

            Console.WriteLine("{0} {1:0.00} MB in {2:0.00} secs -> {3:0.00} Mbps",
                header, totalMB, timeSecs, speed * 8);
        }
    }
}
