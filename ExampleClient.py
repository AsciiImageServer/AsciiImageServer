"""
Sample code for talking to the image server.
"""
import socket
import struct
import time

class AsciiImageServerClient():
	def __init__(self):
		self.connect()

	def connect(self):
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.connect(("localhost", 5555))
		print self.recv_response()

	def disconnect(self):
		print self.send_command_string('q')

	def recv_response(self):
		time.sleep(0.25)
		return self.sock.recv(10000)

	def send_command_string(self, command):
		cmd_len = len(command)
		pkt = struct.pack('I', cmd_len) + command
		self.sock.send(pkt)
		return self.recv_response()

	def get_image_count(self):
		return self.send_command_string('c')

	def get_image_at_index(self, index):
		return self.send_command_string('g%d\0' % index)

	def display_image_at_index(self, index):
		print self.get_image_at_index(index)

	def add_image(self, requires_login, caption, ascii_image, hash_bytes):
		return self.send_command_string('a%s%s\0%s\0%s' % (chr(requires_login), caption, ascii_image, hash_bytes))

	def attempt_login(self, password):
		return self.send_command_string('l%s' % password)


if __name__ == "__main__":
	c = AsciiImageServerClient()
	print c.get_image_count()
	c.display_image_at_index(1)
	c.disconnect()