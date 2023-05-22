#!/usr/bin/python3
import sys, os
import fcntl, errno, curses
import serial
import time
import base64
import struct
from collections import namedtuple
from functools import reduce
import itertools
import subprocess

def avrdude(port, *args):
	# http://www.nongnu.org/avrdude/user-manual/avrdude_4.html
	return subprocess.call(
		(
			r"avrdude",
			#r"-CC:\SysGCC\avr\avrdude.conf", "-PCOM"+str(portnum+1),
			"-pm328p", "-carduino",
			#"-b115200",
			#"-b57600",
			"-P"+port
		) + args,
		stdin=sys.stdin,
		stdout=sys.stdout,
		stderr=sys.stderr
	)

def ioloop(win, port):
	while True:
		c = win.getch()
		if c>=0:
			if c==0x0D: c=0x0A # Translate return to newline
			#win.addstr('(%02X)'%c)
			#win.addch(c)
			try: port.write(bytes((c,)))
			except serial.serialutil.SerialException: pass
		rx = port.read(1)
		if rx:
			c = rx[0]
			if c==0x04: break;
			if c==0x7F:
				y,x=win.getyx()
				win.move(y,x-1)
				win.delch()
			else: win.addch(c)
			#win.addstr('(%02X)'%c)

def console(port):
	# Make stdin nonblocking
	#infd = sys.stdin.fileno()
	#fcntl.fcntl(infd, fcntl.F_SETFL, fcntl.fcntl(infd, fcntl.F_GETFL) | os.O_NONBLOCK)
	win = curses.initscr()
	try:
		curses.cbreak()
		curses.nonl()
		curses.noecho()
		win.scrollok(True)
		win.nodelay(True)
		with serial.Serial(port=port, baudrate=115200, bytesize=8, parity=serial.PARITY_NONE, stopbits=1, xonxoff=0, rtscts=0, timeout=0) as port:
			ioloop(win, port)
	except KeyboardInterrupt:
		pass
	finally:
		curses.echo()
		curses.nl()
		curses.nocbreak()
		curses.endwin()

def bootload(port, baud):
	#avrdude(port, "-q",  "-Ulfuse:r:-:h", "-Uhfuse:r:-:h", "-Uefuse:r:-:h", "-Ulock:r:-:h", "-Usignature:r:-:h")
	#exit()
	if True:
		if avrdude(port, "-b%i"%baud, "-v", "-D", "-Uflash:w:__builddir__/app.hex:i"):
			print("Download fail!")
			return
	time.sleep(.5)
	console(port)
	

def main():
	bootload(sys.argv[1], 57600)
	print("Done")

if __name__ == "__main__": main()

#
# C:\SysGCC\avr\avrdude -CC:\SysGCC\avr\avrdude.conf -patmega328p -carduino -PCOM5 -b115200 -D -
# C:\SysGCC\avr\avrdude -CC:\SysGCC\avr\avrdude.conf -v -v -v -v -patmega328p -carduino -PCOM5 -b115200 -D -Uflash:w:__builddir__/app.hex:i

#/cygdrive/c/SysGCC/avr/avrdude -C/cygdrive/c/SysGCC/avr/avrdude.conf -v -v -v -v -patmega328p -carduino -PCOM5 -b115200 -D -Uflash:w:__builddir__/app.hex:i

#

