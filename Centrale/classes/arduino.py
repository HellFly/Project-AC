import os
import shutil
import configparser
import serial
import threading
import time
import datetime
import random

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

		global __a_temperature
		__a_temperature = 0
		global __a_light
		__a_light = 0
		global __a_blinds_status
		__a_blinds_status = False

		global __a_temperature_list
		__a_temperature_list = []
		global __a_light_list
		__a_light_list = []

		global __a_temperature_connected
		__a_temperature_connected = False
		global __a_light_connected
		__a_light_connected = False

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
				global __a_light_list
				__a_light = p1*256 + p2
				__a_light_list.append([datetime.datetime.now(), __a_light])
				self.reset_data()
				print("Light: " + str(__a_light))
		elif c == 2: # report temperature sensor
			if p1 != -1:
				global __a_temperature
				global __a_temperature_list
				__a_temperature = p1-128
				__a_temperature_list.append([datetime.datetime.now(), __a_temperature])
				self.reset_data()
				print("Temperature: " + str(__a_temperature))
		elif c == 3: # report status of blinds
			if p1 != -1 and p2 != -1:
				if p1 == 0: # blinds from light unit
					global __a_blinds_status
					if p2 == 0: # blinds are closed
						__a_blinds_status = False
					else:
						__a_blinds_status = True
					self.reset_data()
				elif p1 == 1: # blinds from temperature unit
					#global __a_blinds_status
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

	def get_temperature(self):
		global __a_temperature
		__a_temperature = random.randint(16, 30)
		return __a_temperature
	def get_light(self):
		global __a_light
		return __a_light
	# List structure:
	#	{
	#		(datetime, temperature),
	#		(datetime, temperature),
	#		(datetime, temperature),
	#		etc...
	#	}
	def get_temperature_list(self):
		global __a_temperature_list
		return __a_temperature_list
	# List structure:
	#	{
	#		(datetime, light),
	#		(datetime, light),
	#		(datetime, light),
	#		etc...
	#	}
	def get_light_list(sef):
		global __a_light_list
		return __a_light_list
	def get_blinds(self): # False = closed, True = open
		global __a_blinds_status
		return __a_blinds_status
	def temperature_connected(self):
		global __a_temperature_connected
		return __a_temperature_connected
	def light_connected(self):
		global __a_light_connected
		return __a_light_connected
