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
		self.com = serial.Serial(comport , baudrate, timeout=.1)
		#self.com = None
		self.data = [-1] * 10
		self.running = True
		self.start()
	def run(self):
		while self.running:
			if self.com != None:
				byte = self.com.read()
				if byte:
					self.add_byte(ord(byte))
			
			
	def reset_data(self):
		self.data = [-1] * 10
	
	def add_byte(self, byte):
		i = 0
		while i < len(self.data):
			if self.data[i] == -1:
				self.data[i] = byte
				i = len(self.data)
			i += 1
		if i == len(self.data):
			self.reset_data()