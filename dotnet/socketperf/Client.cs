using System;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace socketperf
{
    class Client
    {
        internal Client(string server)
        {
            mServer = server;
        }

        internal void Download()
        {
            IPHostEntry ipHostInfo = Dns.Resolve(mServer);
            IPAddress ipAddress = ipHostInfo.AddressList[0];
            IPEndPoint remoteEP = new IPEndPoint(ipAddress, Constants.PORT);

            Socket socket = new Socket(
                AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

            socket.Connect(remoteEP);
            try
            {
                byte[] buffer = new byte[Constants.BUFFER_SIZE];

                using (NetworkStream ns = new NetworkStream(socket))
                {
                    int ini = Environment.TickCount;

                    uint length = LengthSerializer.Deserialize(ns);

                    uint remaining = length;

                    int read = 0;
                    while (remaining > 0)
                    {
                        int readSize = buffer.Length;
                        if (readSize > remaining)
                            readSize = (int)remaining;

                        read = ns.Read(buffer, 0, readSize);
                        remaining -= (uint)read;
                    }

                    LogPrinter.LogTime("received", length, ini);
                }
            }
            finally
            {
                socket.Shutdown(SocketShutdown.Both);
                socket.Close();
            }
        }

        string mServer;
    }
}
