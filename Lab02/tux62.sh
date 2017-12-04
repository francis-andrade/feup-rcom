#! /bin/bash
# Script

ifconfig eth0 up
ifconfig eth0 172.16.61.1/24
route add default gw 172.16.61.254
route add -net 172.16.60.0/24 gw 172.16.61.253
