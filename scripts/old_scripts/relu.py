import RPi.GPIO as GPIO
import time
from math import *
from random import *

off = True
while True:
	level = int(input("Input: "))

	if level > 1 or level < 0:
		break
	if level == 1 and off:
		off = False
		GPIO.setmode(GPIO.BOARD)
		GPIO.setup(23, GPIO.OUT)
		GPIO.output(23, 0)
	elif level == 0 and not off:
		off = True
		GPIO.cleanup()
GPIO.cleanup()
