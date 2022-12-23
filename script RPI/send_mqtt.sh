#!/bin/bash

echo "mosquitto_pub -h localhost -t "$1" -m '"$2"' -u "user" -P "user pwd" >> test.log" | at $3
