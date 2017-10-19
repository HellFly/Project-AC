import os
import serial
import threading
import time

#class for communicating with the arduino
# http://www.instructables.com/id/Arduino-Python-Communication-via-USB/
# https://www.python-course.eu/threads.php
class Arduino(threading.Thread):
	def __init__(self, comport, baudrate):
		threading.Thread.__init__(self)
		self.com = None
		self.counter = 0
		#self.com = serial.Serial(comport , baudrate, timeout=.1)
		self.running = True
	def run(self):
		while self.running:
			data = None
			if self.com != None:
				data = self.com.readline()

			self.counter += 1
			if self.counter > 3:
				self.running = False

			print("Counter is " + str(self.counter))
			time.sleep(1)

running = True
com = Arduino('COM1', 96000)
com.start()

while running:
	#Run the program
	time.sleep(1)

com.join()
