socketperf
==========
Tool to measure the network performance through TCP sockets. Include versions in C & C#.

usage:
* socketperf.exe -c server_address
* socketperf.exe -s [memory size_in_mb | readsend file_path | transmitfile file_path]


server modes:
* memory -> the server will send the specified amount of MB directly from memory (max value 4095MB).
* readsend -> the server will read and send the specified file (max file size 2GB).
* transmitfile -> the server will send the specified file using the TransmitFile API call (max file size 2GB).

examples:
* socketperf.exe -s memory 2048
* socketperf.exe -s readsend c:\data\server.iso
* socketperf.exe -s transmitfile c:\data\server.iso
* socketperf.exe -c 192.168.1.55
