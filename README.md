# DTN for Zeppelin.

Download source from:
http://prdownloads.sourceforge.net/project/ion-dtn/ion-3.6.0.tar.gz

Porting ion to ARM (ZC706@Xilinx, RaspberryPi, Zynq, Pynq, all tested)

LTP configuration file (ltp.rc) using port 9988 (not privileged port,
therefore sudo not necessary).

====================================================
## begin ionadmin
1 1 bench.ionconfig
s

a contact +0 +86400 1 4 1250000
a contact +0 +86400 4 1 1250000
a range +0 +86400 1 4 1
m production 1000000
m consumption 1000000
m horizon +0
## end ionadmin

## begin ionsecadmin
1
e 1
## end ionsecadmin

## begin ltpadmin
1 50 24000000
a span 4 50 240000 50 240000 1400 240000 1 'udplso 192.168.200.172:9988'
s 'udplsi 192.168.2.112:9988'
m screening n
w 1
## end ltpadmin

## begin bpadmin
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.1 q
a protocol ltp 1400 100
a induct ltp 1 ltpcli
a outduct ltp 4 ltpclo

# Start the daemons
s
## end bpadmin

## begin ipnadmin
a plan 4 ltp/4
#a group 3 3 2
## end ipnadmin

====================================================
udplsi -- local IP and PORT, with endpoint ipn:1.1
udplso -- remote IP and PORT, with endpoint ipn:4.1

Too large wmSize and heapWords may not work in bench.ionconfig:

wmSize 5000000
wmAddress 0
configFlags 1
heapWords 2000000
pathName /tmp

