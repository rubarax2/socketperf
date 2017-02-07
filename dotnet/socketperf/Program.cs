using System;

namespace socketperf
{
    class Program
    {
        static int Main(string[] args)
        {
            if (args.Length < 2)
            {
                Usage();
                return 1;
            }

            if (args[0] == "-c")
            {
                SetChunkSizeAndPortIfNeeded(args, 2);
                Client client = new Client(args[1]);
                client.Download();
                return 0;
            }

            if (args[0] != "-s" || args.Length < 3)
            {
                Usage();
                return 1;
            }

            ServerMode mode;
            switch(args[1])
            {
                case "memory":
                    mode = ServerMode.Memory;
                    break;

                case "readsend":
                    mode = ServerMode.ReadSend;
                    break;

                case "transmitfile":
                    mode = ServerMode.TransmitFile;
                    break;

                default:
                    Usage();
                    return 1;
            }

            ServerData data = new ServerData();
            if (mode == ServerMode.Memory)
                data.SizeInMB = uint.Parse(args[2]);
            else
                data.FilePath = args[2];

            SetChunkSizeAndPortIfNeeded(args, 3);

            Server server = new Server(data);
            server.Run(mode);
            return 0;
        }

        static void SetChunkSizeAndPortIfNeeded(string[] args, int index)
        {
            uint chunkSize;
            if (index < args.Length && uint.TryParse(args[index], out chunkSize))
            {
                Constants.CHUNK_IN_KB = chunkSize;
            }

            index++;

            uint port;
            if (index < args.Length && uint.TryParse(args[index], out port))
            {
                Constants.PORT = (int)port;
            }

            Console.WriteLine("Port number {0}. Chunk size set to {1} KBytes",
                Constants.PORT, Constants.CHUNK_IN_KB);
        }

        static void Usage()
        {
            Console.WriteLine("usage:");
            Console.WriteLine("    socketperf.exe -c server_address [chunk_size_in_kb] [port]");
            Console.WriteLine("    socketperf.exe -s [memory size_in_mb | readsend file_path | transmitfile file_path] [chunk_size_in_kb] [port]");
            Console.WriteLine();
            Console.WriteLine("server modes:");
            Console.WriteLine("    memory -> the server will send the specified amount of MB directly from memory (max value 4095MB). ");
            Console.WriteLine("    readsend -> the server will read and send the specified file (max file size 4GB).");
            Console.WriteLine("    transmitfile -> the server will send the specified file using the TransmitFile API call (max file size 2GB).");
            Console.WriteLine();
            Console.WriteLine("sample: test file transfer using regular sockets:");
            Console.WriteLine();
            Console.WriteLine("    socketperf.exe -s readsend c:\\data\\server.iso");
            Console.WriteLine("    readsend : sent 1895.63 MB in 20.11 secs -> 754.14 Mbps");
            Console.WriteLine();
            Console.WriteLine("    socketperf.exe -c 192.168.1.55");
            Console.WriteLine("    received 1895.63 MB in 20.11 secs -> 754.14 Mbps");
            Console.WriteLine();
            Console.WriteLine("sample: test transfer with memory generated data (not reading from disk)");
            Console.WriteLine();
            Console.WriteLine("    socketperf.exe -s memory 4095");
            Console.WriteLine("    memory: sent 4095.00 MB in 37.03 secs -> 884.69 Mbps");
            Console.WriteLine();
            Console.WriteLine("    socketperf.exe -c 192.168.1.55");
            Console.WriteLine("    received 4095.00 MB in 37.03 secs -> 884.69 Mbps");
            Console.WriteLine();
            Console.WriteLine("sample: test TransmitFile performance");
            Console.WriteLine();
            Console.WriteLine("    socketperf.exe -s transmitfile c:\\data\\server.iso");
            Console.WriteLine("    transmitfile : sent 1680.31 MB in 21.03 secs -> 639.18 Mbps");
            Console.WriteLine();
            Console.WriteLine("    socketperf.exe -c 192.168.1.55");
            Console.WriteLine("    received 1680.31 MB in 21.03 secs -> 639.18 Mbps");
        }
    }
}
