import os
import shutil
import configparser
import serial
import threading
import time

__a_running = True

__a_temperature = 0
__a_light = 0
__a_blinds_status = False

__a_temperature_connected = False
__a_light_connected = False

#class for communicating with the arduino
# http://www.instructables.com/id/Arduino-Python-Communication-via-USB/
# https://www.python-course.eu/threads.php
class Arduino(threading.Thread):
	def __init__(self):
		threading.Thread.__init__(self)

		self.com = None
		self.data = [-1] * 10

		self.send_bytes = [-1]

		global __a_running
		__a_running = True

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

		while __a_running:
			if self.com != None:
				byte = self.com.read()
				if byte:
					self.add_byte(ord(byte))
					self.parse_data()

				if self.send_bytes[0] != -1:
					com.write(self.send_bytes)
					self.send_bytes = [-1]

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
				global __a_light
				__a_light = p1*256 + p2
				self.reset_data()
				print("Light: " + str(__a_light))
		elif c == 2: # report temperature sensor
			if p1 != -1:
				global __a_temperature
				__a_temperature = p1-128
				self.reset_data()
				print("Temperature: " + str(__a_temperature))
		elif c == 3: # report status of blinds
			if p1 != -1 and p2 != -1:
				if p1 == 0: # blinds from light unit
					if p2 == 0: # blinds are closed
						__a_blinds_status = False
					else:
						__a_blinds_status = True
					self.reset_data()
				elif p1 == 1: # blinds from temperature unit
					if p2 == 0: # blinds are closed
						__a_blinds_status = False
					else:
						__a_blinds_status = True
					self.reset_data()
				else: # no valid units to report from so reset the data
					self.reset_data()

		else: # no valid packet id, so reset the data
			self.reset_data()


	def stop(self):
		print("Stopping thread")
		global __a_running
		__a_running = False

	def get_temperature():
		global __a_temperature
		return __a_temperature
	def get_light():
		global __a_light
		return __a_light
	def get_blinds(): # False = closed, True = open
		global __a_blinds_status
		return __a_blinds_status
	def temperature_connected():
		global __a_temperature_connected
		return __a_temperature_connected
	def light_connected():
		global __a_light_connected
		return __a_light_connected
