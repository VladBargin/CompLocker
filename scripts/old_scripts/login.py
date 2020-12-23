import sys
import time
from datetime import datetime
from picamera import PiCamera

camera = PiCamera()
#camera.start_preview()

code = 1
logins = {'12345': '12345', '123': '123', '1': '1'};
with open("../logs/logs.txt", "a") as f:
	match = False
	if len(sys.argv) != 3:
		f.write("Incorrect format, got arguments: " + str(sys.argv[1:]) + '\n')
	else:
		login = sys.argv[1]
		passwd = sys.argv[2]
		for l, p in logins.items():
			if l == login and passwd == p:
				match = True
				break

		if match:
			f.write('Got match with [' + login + ', ' + passwd + ']\n')
		else:
			f.write('Did not get match with [' + login + ', ' +  passwd + ']\n')
	stime = str(datetime.now())
	time.sleep(2)
	f.write(stime + '\n\n')
	camera.capture('../logs/' + stime + '.jpg')

	if match:
		code = 0

#camera.stop_preview()
sys.exit(code)
