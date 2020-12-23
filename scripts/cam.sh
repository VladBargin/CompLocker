#!/bin/bash

DATE=$(date +"%Y-%m-%d_%H%M")

raspistill -rot 270 -o /home/pi/Documents/CompLocker/logs/$DATE.jpg
