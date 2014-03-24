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
                Client client = new Client(args[1]);
                client.Download();
                return 0;
            }

            if (args[0] != "-s" || args.Length != 3)
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

            Server server = new Server(data);
            server.Run(mode);
            return 0;
        }

        static void Usage()
        {
            Console.WriteLine("usage:");
            Console.WriteLine("    socketperf.exe -c server_address");
            Console.WriteLine("    socketperf.exe -s [memory size_in_mb | readsend file_path | transmitfile file_path]");
            Console.WriteLine();
            Console.WriteLine("server modes:");
            Console.WriteLine("    memory -> the server will send the specified amount of MB directly from memory (max value 4095MB). ");
            Console.WriteLine("    readsend -> the server will read and send the specified file (max file size 4GB).");
            Console.WriteLine("    transmitfile -> the server will send the specified file using the TransmitFile API call (max file size 2GB).");
        }
    }
}
