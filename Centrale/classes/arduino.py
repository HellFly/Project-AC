import os
import shutil
import configparser
import serial
import threading
import time

#class for communicating with the arduino
# http://www.instructables.com/id/Arduino-Python-Communication-via-USB/
# https://www.python-course.eu/threads.php
class Arduino(threading.Thread):
	def __init__(self):
		threading.Thread.__init__(self)

		self.com = None
		self.data = [-1] * 10
		self.running = True

		self.temperature_connected = False
		self.light_connected = False
		self.temperature = 0
		self.light = 0
		self.open_status = 'Open'

		self.start()
	def run(self):
		self.config = configparser.ConfigParser()
		if not os.path.isfile('config.ini'):
			shutil.copyfile('config.example.ini', 'config.ini')
		self.config.read('config.ini')

		comport = self.config['Communication']['comport']
		baudrate = self.config['Communication']['baudrate']

		# Init the comport to communicate with the arduino
		self.com = serial.Serial(comport , baudrate, timeout=.1)

		while self.running:
			if self.com != None:
				byte = self.com.read()
				if byte:
					self.add_byte(ord(byte))
					self.parse_data()



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
	def parse_data(self):
		c = self.data[0]
		p1 = self.data[1]
		p2 = self.data[2]
		p3 = self.data[3]
		p4 = self.data[4]

		if c == 1: # report light sensor
			if p1 != -1 and p2 != -1:
				self.light = p1*256 + p2
				self.reset_data()
				print("Light: " + str(self.light))
		elif c == 2: # report temperature sensor
			if p1 != -1:
				self.temperature = p1-128
				self.reset_data()
				print("Temperature: " + str(self.temperature))


	def get_temperature(self):
		return self.temperature
	def get_light(self):
		return self.light
