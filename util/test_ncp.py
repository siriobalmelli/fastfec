#!/usr/bin/env python3

#   test_ncp.py
# test 'ncp' utility for correctness

import subprocess
import sys
import os

ncp = 'util/ncp' 
test_file = 'copy.bin'

def create_random_file():
	try:
		subprocess.run(['dd', 'if=/dev/urandom', 'of='+test_file, 'bs=1024', 'count=1024'],
				stdout=subprocess.PIPE, stderr=subprocess.PIPE,
				shell=False, check=True);
	except subprocess.CalledProcessError as err:
		print(err.cmd)
		exit(1)

def ncp_copy(cmds, ncp): 
	'''copy a file using cmds"'''
	try:
		cmds.insert(0, ncp)
		subprocess.run(cmds,
				stdout=subprocess.PIPE, stderr=subprocess.PIPE,
				shell=False, check=True);
	except subprocess.CalledProcessError as err:
		print(err.cmd)
		print(err.stdout.decode('ascii'))
		print(err.stderr.decode('ascii'))
		exit(1)

def check_output_files():
	if os.path.isfile('no_flags.copy') and os.path.isfile('force.copy') \
		and os.path.isfile('verbose.copy') and os.path.isfile('verbose_force.copy'):
			os.remove('no_flags.copy')
			os.remove('verbose.copy')
			os.remove('force.copy')
			os.remove('verbose_force.copy')
			os.remove(test_file)
			exit(0)
	else:
		exit(1)

cmds = { 'clean'	: [ test_file, 'no_flags.copy'],
		 'force'	: [ '-f', test_file, 'force.copy'],
		 'verbose'	: [ '-v', test_file, 'verbose.copy'],
		 'force_verbose' : [ '-v', '-f', test_file, 'verbose_force.copy']
		 }

#   main()
if __name__ == "__main__":
	if sys.argv[1] is not None:
		ncp = sys.argv[1]

	create_random_file()

	for k,v in cmds.items():
		ncp_copy(v, ncp)

	check_output_files()
