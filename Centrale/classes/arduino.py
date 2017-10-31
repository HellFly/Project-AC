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
		# Init the comport to communicate with the arduino
		#self.com = serial.Serial(comport , baudrate, timeout=.1)
		self.com = None
		self.counter = 0
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

	def setLightValue(self, sensorvalue):
		self.lightvolume = sensorvalue

	def printLightVolume(self):
		setLightValue(sensorinput)

		if self.lightvolume < 40:
			print("Zeer donker")
		elif self.lightvolume > 40 and self.lightvolume < 80:
			print("Donker")
		elif self.lightvolume > 80 and self.lightvolume < 120:
			print("Schemerig")
		elif self.lightvolume > 120 and self.lightvolume < 160:
			print("Licht")
		elif self.lightvolume > 160 and self.lightvolume < 200:
			print("Zeer licht")
		else:
			print("Er is geen lichtvolume gemeten")
