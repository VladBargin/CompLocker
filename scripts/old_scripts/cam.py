from picamera import PiCamera
from time import sleep

camera = PiCamera()

camera.start_preview()
camera.start_recording("vid0.h264")
sleep(30)
camera.stop_recording()
camera.stop_preview()

