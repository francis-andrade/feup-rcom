#! /bin/bash
# Script

ifconfig eth0 up
ifconfig eth0 172.16.60.254/24
ifconfig eth1 172.16.61.253/24
route add default gw 172.16.61.254