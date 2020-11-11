import RPi.GPIO as GPIO
import time
from math import *
from random import *


GPIO.setmode(GPIO.BOARD)
GPIO.setup(23, GPIO.OUT)
GPIO.output(23, 0)
GPIO.cleanup()
