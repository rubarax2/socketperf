using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace socketperf
{
    internal enum ServerMode { Memory, ReadSend, TransmitFile };

    internal class ServerData
    {
        internal uint SizeInMB;
        internal string FilePath;
    }
    class Server
    {
        internal Server(ServerData data)
        {
            mData = data;
        }

        internal void Run(ServerMode mode)
        {
            Socket socket = new Socket(
                AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

            socket.Bind(new IPEndPoint(IPAddress.Any, Constants.PORT));

            socket.Listen(10);

            while (true)
            {
                Socket client = socket.Accept();
                try
                {
                    switch (mode)
                    {
                        case ServerMode.Memory:
                            MemorySend(client);
                            break;

                        case ServerMode.ReadSend:
                            ReadSend(client);
                            break;

                        case ServerMode.TransmitFile:
                            TransmitFile(client);
                            break;
                    }
                }
                finally
                {
                    client.Shutdown(SocketShutdown.Both);
                    client.Close();
                }
            }
        }

        void MemorySend(Socket client)
        {
            byte[] buffer =  new byte[Constants.BUFFER_SIZE];
            (new Random()).NextBytes(buffer);

            uint totalLen = (uint)mData.SizeInMB * 1024 * 1024;

            int ini = Environment.TickCount;

            using (NetworkStream ns = new NetworkStream(client))
            {
                LengthSerializer.Serialize(ns, totalLen);

                uint toSent;
                uint totalSent = 0;
                do
                {
                    if ((totalLen - totalSent) < Constants.BUFFER_SIZE)
                        toSent = totalLen - totalSent;
                    else
                        toSent = Constants.BUFFER_SIZE;

                    ns.Write(buffer, 0, (int)toSent);

                    totalSent += toSent;
                }
                while (totalSent < totalLen);
            }

            LogPrinter.LogTime("memory: sent", totalLen, ini);
        }

        void ReadSend(Socket client)
        {
            uint length = (uint)new FileInfo(mData.FilePath).Length;

            byte[] buffer = new byte[Constants.BUFFER_SIZE];

            int ini = Environment.TickCount;

            using (NetworkStream ns = new NetworkStream(client))
            {
                LengthSerializer.Serialize(ns, length);

                using (FileStream fs = File.OpenRead(mData.FilePath))
                {
                    int read = fs.Read(buffer, 0, buffer.Length);
                    while (read > 0)
                    {
                        ns.Write(buffer, 0, read);
                        read = fs.Read(buffer, 0, buffer.Length);
                    }
                }
            }

            LogPrinter.LogTime("readsend: sent", length, ini);
        }

        void TransmitFile(Socket client)
        {
            uint length = (uint)new FileInfo(mData.FilePath).Length;

            int ini = Environment.TickCount;

            using (NetworkStream ns = new NetworkStream(client))
            {
                byte[] lengthBytes = LengthSerializer.Serialize(length);

                // send the header
                client.SendFile(mData.FilePath, lengthBytes, null, TransmitFileOptions.UseKernelApc);
            }

            LogPrinter.LogTime("transmitfile: sent", length, ini);
        }

        ServerData mData;
    }
}
