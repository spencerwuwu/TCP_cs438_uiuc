#/bin/sh

# [this delete any existing tc rules]
sudo tc qdisc del dev eth0 root 2>/dev/null
sudo tc qdisc add dev eth0 root handle 1:0 netem delay 20ms loss 25%
sudo tc qdisc add dev eth0 parent 1:1 handle 10: tbf rate 100Mbit burst 40mb latency 25ms
