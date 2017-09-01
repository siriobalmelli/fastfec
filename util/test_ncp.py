#!/usr/bin/env python3

#   test_ncp.py
# test 'ncp' utility for correctness

import subprocess
import sys
import os
import filecmp
import collections as col

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
		sub = subprocess.run(cmds,
				stdout=subprocess.PIPE, stderr=subprocess.PIPE,
				shell=False, check=True);
	
		return sub.stdout.decode('ascii'), sub.stderr.decode('ascii')
	except subprocess.CalledProcessError as err:
		print(err.cmd)
		print(err.stdout.decode('ascii'))
		print(err.stderr.decode('ascii'))
		exit(1)

output_files = [ 'no_flags.copy', 'verbose.copy' ]
# No output is valid, so we add it to the list.
verbose_output = [ '', '\'copy.bin\' -> \'verbose.copy\'' ]

def check_output_files():
	for out_file in output_files:
		if not os.path.isfile(out_file):
			exit(1)
		else:
			if not filecmp.cmp(test_file, out_file):
				exit(1)
			else:
				os.remove(out_file)

	os.remove(test_file)

# Use OrderedDict such that iterations are always in the order of elements inserted.
# To make sure that 'force' tests always succeed overwriting previously created file.
cmds = col.OrderedDict([
		 ('clean', [ test_file, output_files[0]]),
		 ('force', [ '-f', test_file, output_files[0]]),
		 ('verbose', [ '-v', test_file, output_files[1]]),
		 ('force_verbose', [ '-v', '-f', test_file, output_files[1]])
		 ])

#   main()
if __name__ == "__main__":
	if sys.argv[1] is not None:
		ncp = sys.argv[1]
	
	create_random_file()

	for k,v in cmds.items():
		stdout, stderr = ncp_copy(v, ncp)
		# Check output, if any
		if stdout.rstrip() not in verbose_output:
			exit(1)
		elif stderr:
			exit(1)

	check_output_files()
