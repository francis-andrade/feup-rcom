#! /bin/bash
# Script

ifconfig eth0 up
ifconfig eth0 172.16.60.1/24
route add default gw 172.16.60.254
