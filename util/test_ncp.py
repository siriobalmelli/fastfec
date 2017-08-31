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

def ncp_copy(cmds): 
	'''copy a file using cmds"'''
	try:
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

#   main()
if __name__ == "__main__":
	if sys.argv[1] is not None:
		ncp = sys.argv[1]

	create_random_file()
	cmds_clean = [ncp, test_file, 'no_flags.copy']
	cmds_force = [ncp, '-f', test_file, 'force.copy']
	cmds_verbose = [ncp, '-v', test_file, 'verbose.copy'] 
	cmds_verbose_copy = [ncp, '-v', '-f', test_file, 'verbose_force.copy']

	ncp_copy(cmds_clean)
	ncp_copy(cmds_force)
	ncp_copy(cmds_verbose)
	ncp_copy(cmds_verbose_copy)

	check_output_files()
