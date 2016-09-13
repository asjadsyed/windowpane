# windowpane

Compiling:
```
$ make
```

Installing:
```
$ make install
```

`cowpatty2windowpane` Usage:
```
$ ./cowpatty2windowpane
error! usage: ./cowpatty2windowpane <input.cwpa> <output.wndp>
$ ./cowpatty2windowpane input.cwpa output.wndp
$ du -sh input.cwpa output.wndp
393M	input.cwpa
294M	output.wndp
$ 
```

`windowpane` Usage:
```
$ convert packet capture to simpler format
$ aircrack-ng -J packetcapture packetcapture.pcap
Opening packetcapture.pcap
Read 499 packets.

   #  BSSID              ESSID                     Encryption

   1  00:0B:86:C2:A4:85  linksys                   WPA (1 handshake)

Choosing first network as target.

Opening packetcapture.pcap
Reading packets, please wait...

Building Hashcat (1.00) file...

[*] ESSID (length: 7): linksys
[*] Key version: 2
[*] BSSID: 00:0B:86:C2:A4:85
[*] STA: 00:13:CE:55:98:EF
[*] anonce:
    1A 9B DF 0C C8 9E 5E 32 20 F7 1A A7 4F E3 2D F6 
    5B B8 C1 C5 B8 66 4B 9D 98 AE F7 09 B9 64 4D 29 
[*] snonce:
    E8 DF A1 6B 87 69 95 7D 82 49 A4 EC 68 D2 B7 64 
    1D 37 82 16 2E F0 DC 37 B0 14 CC 48 34 3E 8D D4 
[*] Key MIC:
    0E 71 A6 25 FA AD E7 CE 9C 82 21 F7 B1 DB CE 46
[*] eapol:
    01 03 00 75 02 01 0A 00 00 00 00 00 00 00 00 00 
    05 E8 DF A1 6B 87 69 95 7D 82 49 A4 EC 68 D2 B7 
    64 1D 37 82 16 2E F0 DC 37 B0 14 CC 48 34 3E 8D 
    D4 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
    00 00 16 30 14 01 00 00 0F AC 04 01 00 00 0F AC 
    04 01 00 00 0F AC 02 28 00 

Successfully written to packetcapture.hccap


Quitting aircrack-ng...
$ 
$ # optional: use pw-inspector to trim wordlist to occupy less space
$ # on disk by resticting each guess to wpa2 specs specifications
$ pw-inspector -m 8 -M 63 -i rockyou.txt -o wpa-rockyou.txt
$ du -sh rockyou.txt wpa-rockyou.txt
134M	rockyou.txt
100M	wpa-rockyou.txt
$ ./windowpane
[FIX me later]
error! usage: ./windowpane <wordlist.txt> <hashes.wndp> <handshake.hccap>
$ ./windowpane rockyou.txt output.wndp
success! 
ssid is: [ dictionary ]
```
