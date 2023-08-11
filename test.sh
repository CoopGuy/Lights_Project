#!/bin/bash
alias run="sudo ./IRDecode"
alias runc="gcc -Wall -pthread -o IRDecode IRDecode.c -I/home/pi/Led-Testing/pigpio-master -lpigpio -lrt -lm && run"
alias comp="gcc -Wall -pthread -o IRDecode IRDecode.c -I/home/pi/Led-Testing/pigpio-master -lpigpio -lrt -lm" 
