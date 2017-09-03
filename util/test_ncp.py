#!/usr/bin/env python3

#   test_ncp.py
# test 'ncp' utility for correctness

import subprocess
import sys
import os
import filecmp
import collections as col

ncp = 'util/ncp' 
test_dir = 'test_copy/'

input_files = [ 'copy.bin', 'copy_one.bin', 'copy_two.bin', 'copy_three.bin', 'copy_four.bin' ]
output_files = [ 'no_flags.copy', 'verbose.copy', 'test_copy/no_flags.copy', 
		'test_copy/verbose.copy', 'test_copy/copy_one.bin', 'test_copy/copy_two.bin',
		'test_copy/copy_three.bin', 'test_copy/copy_four.bin' ]

# No output is valid, so we add it to the list.
verbose_output = [ '', '\'copy.bin\' -> \'verbose.copy\'',
				'\'no_flags.copy\' -> \'test_copy/no_flags.copy\'\n\'verbose.copy\' -> \'test_copy/verbose.copy\'', 
				'\'copy_one.bin\' -> \'test_copy/copy_one.bin\'\n\'copy_two.bin\' -> \'test_copy/copy_two.bin\'',
				'\'copy_three.bin\' -> \'test_copy/copy_three.bin\'',
				'\'copy_four.bin\' -> \'test_copy/copy_four.bin\'']

def create_random_file(filename):
	try:
		subprocess.run(['dd', 'if=/dev/urandom', 'of='+filename, 'bs=1024', 'count=1024'],
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
		print(sub.args)	
		return sub.stdout.decode('ascii'), sub.stderr.decode('ascii')
	except subprocess.CalledProcessError as err:
		print(err.cmd)
		print(err.stdout.decode('ascii'))
		print(err.stderr.decode('ascii'))
		exit(1)

def delete_directory():
	'''delete the test directory using shell commands'''
	try:
		sub = subprocess.run(['rm', '-rf', test_dir],
				stdout=subprocess.PIPE, stderr=subprocess.PIPE,
				shell=False, check=True);
		print(sub.args)	
	except subprocess.CalledProcessError as err:
		print(err.cmd)
		print(err.stdout.decode('ascii'))
		print(err.stderr.decode('ascii'))
		exit(1)

def check_output_files():
	for out_file in output_files:
		if os.path.isfile(out_file):
			if out_file.endswith('.bin'):
				# compare 'copy_XXXX.bin' files.
				if not filecmp.cmp(input_files[1], output_files[4]):
					print('1. file {0} not equal to {1}'.format(input_files[1], output_files[4]))
					exit(1)
				if not filecmp.cmp(input_files[2], output_files[5]):
					print('2. file {0} not equal to {1}'.format(input_files[2], output_files[5]))
					exit(1)
				if not filecmp.cmp(input_files[3], output_files[6]):
					print('3. file {0} not equal to {1}'.format(input_files[3], output_files[6]))
					exit(1)
				if not filecmp.cmp(input_files[4], output_files[7]):
					print('4. file {0} not equal to {1}'.format(input_files[4], output_files[7]))
					exit(1)
			elif not filecmp.cmp(input_files[0], out_file):
				print('5. file {0} not equal to {1}'.format(input_files[0], out_file))
				exit(1)
		else:
			print('file {0} does not exist'.format(out_file))
			exit(1)

	# delete all files if all checks have succeeded	
	for f in output_files:
		os.remove(f)
	for f in input_files:
		os.remove(f)	
	delete_directory()

# Use OrderedDict such that iterations are always in the order of elements inserted.
# To make sure that 'force' tests always succeed overwriting previously created file.
cmds = col.OrderedDict([
		 # File to File
		 ('clean', [ input_files[0], output_files[0]]),
		 ('force', [ '-f', input_files[0], output_files[0]]),
		 ('verbose', [ '-v', input_files[0], output_files[1]]),
		 ('force_verbose', [ '-v', '-f', input_files[0], output_files[1]]),
		 # File to Directory
		 ('dir_clean', [ input_files[3], test_dir]),
		 ('dir_force', ['-f', input_files[3], test_dir]),
		 ('dir_verbose', ['-v', input_files[4], test_dir]),
		 ('dir_verbose_force', ['-v', '-f', input_files[4], test_dir]),
	     # Files to Directory
		 ('files_clean', [ output_files[0], output_files[1], test_dir ]),
		 ('files_force', [ '-f', output_files[0], output_files[1], test_dir ]),
		 ('files_verbose', [ '-v', input_files[1], input_files[2], test_dir ]),
		 ('files_verbose_force', [ '-v', '-f', input_files[1], input_files[2], test_dir ])
		 ])

#   main()
if __name__ == "__main__":
	if sys.argv[1] is not None:
		ncp = sys.argv[1]

	if not os.path.exists(test_dir):
		os.mkdir(test_dir)

	for f in input_files:
		create_random_file(f)
	
	for k,v in cmds.items():
		stdout, stderr = ncp_copy(v, ncp)
		# Check output, if any
		if stdout.rstrip() not in verbose_output:
			print(stdout)
			exit(1)
		elif stderr:
			print(stderr)
			exit(1)

	check_output_files()
