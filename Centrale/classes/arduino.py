import os
import shutil
import configparser
import serial
import threading
import time
import datetime
import math

#class for communicating with the arduino
# http://www.instructables.com/id/Arduino-Python-Communication-via-USB/
# https://www.python-course.eu/threads.php
class Arduino(threading.Thread):
	def __init__(self):
		threading.Thread.__init__(self)

		self.com = None
		self.data = [-1] * 10

		global __a_running
		global __a_temperature
		global __a_light
		global __a_blinds_light
		global __a_blinds_temperature

		global __a_temperature_list
		global __a_light_list

		global __a_temperature_connected
		global __a_light_connected

		global __a_send_bytes

		__a_running = True
		__a_temperature = 0
		__a_light = 0
		__a_blinds_light = False
		__a_blinds_temperature = False

		__a_temperature_list = []
		__a_light_list = []

		__a_temperature_connected = False
		__a_light_connected = False

		__a_send_bytes = [-1]

		self.start()
	def run(self):
		self.config = configparser.ConfigParser()
		if not os.path.isfile('config.ini'):
			shutil.copyfile('config.example.ini', 'config.ini')
		self.config.read('config.ini')

		comport = self.config['Communication']['comport']
		baudrate = self.config['Communication']['baudrate']

		# Init the comport to communicate with the arduino
		self.com = serial.Serial(comport , baudrate, timeout=.001)

		while __a_running:
			if self.com != None:
				byte = self.com.read()
				if byte:
					self.add_byte(ord(byte))
					self.parse_data()

				global __a_send_bytes
				if send_bytes[0] != -1:
					com.write(send_bytes)
					send_bytes = [-1]

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
					global __a_blinds_light
					if p2 == 0: # blinds are closed
						__a_blinds_light = False
					else:
						__a_blinds_light = True
					self.reset_data()
				elif p1 == 1: # blinds from temperature unit
					global __a_blinds_temperature
					if p2 == 0: # blinds are closed
						__a_blinds_temperature = False
					else:
						__a_blinds_temperature = True
					self.reset_data()
				else: # no valid units to report from so reset the data
					self.reset_data()

		else: # no valid packet id, so reset the data
			self.reset_data()

	# Stop the thread
	def stop(self):
		print("Stopping thread")
		global __a_running
		__a_running = False

	# Get the last reported temperature from the temperature unit
	def get_temperature(self):
		global __a_temperature
		return __a_temperature

	# Get the last reported light value from the light unit
	def get_light(self):
		global __a_light
		return __a_light

	# Get a list of all the temperature values
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

	# Get a list of all the light values
	# List structure:
	#	{
	#		(datetime, light),
	#		(datetime, light),
	#		(datetime, light),
	#		etc...
	#	}
	def get_light_list(self):
		global __a_light_list
		return __a_light_list

	# Get the status of the blinds of the light unit
	# False = closed, True = open
	def get_blinds_light(self):
		global __a_blinds_light
		return __a_blinds_light

	# Get the status of the blinds of the temperature unit
	# False = closed, True = open
	def get_blinds_temperature(self):
		global __a_blinds_temperature
		return __a_blinds_temperature

	# Get if the temperature unit is connected
	def temperature_connected(self):
		global __a_temperature_connected
		return __a_temperature_connected

	# Get if the light unit is connected
	def light_connected(self):
		global __a_light_connected
		return __a_light_connected

	# Open the blinds of the light unit
	def open_blinds_light(self):
		global __a_send_bytes
		__a_send_bytes = [
			10,
			0
		]

	# Open the blinsd of the temperature unit
	def open_blinds_temp(self):
		global __a_send_bytes
		__a_send_bytes = [
			10,
			1
		]

	# Close the blinds of the light unit
	def close_blinds_light(self):
		global __a_send_bytes
		__a_send_bytes = [
			11,
			0
		]

	# Close the blinds of the temperature unit
	def close_blinds_temp(self):
		global __a_send_bytes
		__a_send_bytes = [
			11,
			1
		]

	# Set the opening distace of the blinds of the light unit
	def set_open_distance_light(self, centimeters):
		global __a_send_bytes
		val1 = 0
		val2 = 0

		if centimeters < 0:
			val1 = 0
			val2 = 0
		elif centimeters > 32767:
			val1 = 127
			val2 = 255
		else:
			val1 = math.floor(centimeters / 256)
			val2 = centimeters % 256
		__a_send_bytes = [
			20,
			0,
			val1,
			val2
		]

	# Set the opening distance of the blinds of the temperature unit
	def set_open_distance_temperature(self, centimeters):
		global __a_send_bytes
		val1 = 0
		val2 = 0

		if centimeters < 0:
			val1 = 0
			val2 = 0
		elif centimeters > 32767:
			val1 = 127
			val2 = 255
		else:
			val1 = math.floor(centimeters / 256)
			val2 = centimeters % 256
		__a_send_bytes = [
			20,
			1,
			val1,
			val2
		]

	# Set the closing distance of the blinds of the light unit
	def set_closed_distance_light(self, centimeters):
		global __a_send_bytes
		val1 = 0
		val2 = 0

		if centimeters < 0:
			val1 = 0
			val2 = 0
		elif centimeters > 32767:
			val1 = 127
			val2 = 255
		else:
			val1 = math.floor(centimeters / 256)
			val2 = centimeters % 256
		__a_send_bytes = [
			21,
			0,
			val1,
			val2
		]

	# Set the closing distance of the blinds of the temperature unit
	def set_closed_distance_temperature(self, centimeters):
		global __a_send_bytes
		val1 = 0
		val2 = 0

		if centimeters < 0:
			val1 = 0
			val2 = 0
		elif centimeters > 32767:
			val1 = 127
			val2 = 255
		else:
			val1 = math.floor(centimeters / 256)
			val2 = centimeters % 256
		__a_send_bytes = [
			21,
			1,
			val1,
			val2
		]

	def set_max_value_light(self, light):
		print('do stuff')
	def set_min_value_light(self, light):
		print('do stuff')
	def set_max_value_temperature(self, temperature):
		print('do stuff')
	def set_min_value_temperatuer(self, temperatuer):
		print('do stuff')
