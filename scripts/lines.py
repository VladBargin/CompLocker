from RPi import GPIO
from time import sleep

GPIO.setmode(GPIO.BCM)

left_sensor = 11
right_sensor = 9

GPIO.setup(left_sensor, GPIO.IN)
GPIO.setup(right_sensor, GPIO.IN)

try:
	while True:
		print("Left sensor active:", bool(GPIO.input(left_sensor)), "  Right sensor active:", bool(GPIO.input(right_sensor)))
		sleep(0.5)
except:
	GPIO.cleanup()
