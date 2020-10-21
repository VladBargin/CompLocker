import time
import RPi.GPIO as GPIO
from keypad import keypad

GPIO.setwarnings(False)

kp = keypad()

while True:
	digit = None
	while digit == None:
		digit = kp.getKey()

	print(digit)
	if digit == '#':
		break

	while kp.getKey() != None:
		continue

	time.sleep(0.2)

GPIO.cleanup()
