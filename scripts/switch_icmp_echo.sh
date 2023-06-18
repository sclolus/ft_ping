#!/bin/bash

cat /proc/sys/net/ipv4/icmp_echo_ignore_all |  sed -e '{s/1/0/;t;s/0/1/}'  > /proc/sys/net/ipv4/icmp_echo_ignore_all
