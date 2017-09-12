socketperf
==========
Tool to measure the networxk performance through TCP sockets. Include versions in C & C#.

usage:
* socketperf.exe -c server_address
* socketperf.exe -s [memory size_in_mb | readsend file_path | transmitfile file_path]


server modes:
* memory -> the server will send the specified amount of MB directly from memory (max value 4095MB).
* readsend -> the server will read and send the specified file (max file size 2GB).
* transmitfile -> the server will send the specified file using the TransmitFile API call (max file size 2GB).

sample: test file transfer using regular sockets:
```
  $ socketperf.exe -s readsend c:\data\server.iso
  readsend: sent 1895.63 MB in 20.11 secs -> 754.14 Mbps/s
  
  $ socketperf.exe -c 192.168.1.55
  received 1895.63 MB in 20.11 secs -> 754.14 Mbps/s
```
sample: test transfer with memory generated data (not reading from disk)
```
  $ socketperf.exe -s memory 4095
  memory: sent 4095.00 MB in 37.03 secs -> 884.69 Mbps/s
  
  $ socketperf.exe -c 192.168.1.55
  received 4095.00 MB in 37.03 secs -> 884.69 Mbps/s
```
sample: test TransmitFile performance
```
  $ socketperf.exe -s transmitfile c:\data\server.iso
  transmitfile: sent 1680.31 MB in 21.03 secs -> 639.18 Mbps/s
  
  $ socketperf.exe -c 192.168.1.55
  received 1680.31 MB in 21.03 secs -> 639.18 Mbps/s
```

//change

// new change

//change


//change
//change 
// change
// change
//change  

//change

//cghange

//change
// change
// change
//change
