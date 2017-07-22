#!/usr/bin/env python3

#   test_fnvsum.py
# test 'fnvsum' utility for correctness

import subprocess
import sys
import os

hashes = {
        b''                                              : 0xcbf29ce484222325,
        b'the quick brown fox jumped over the lazy dog'  : 0x4fb124b03ec8f8f8,
        b'/the/enemy/gate/is/down'                       : 0x7814fb571359f23e,
        b'\t{}[]*\&^%$#@!'                               : 0xa8d4c7c3b9738aef,
        b'eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee'        : 0xb47617d43071893b,
        b'The player formerly known as mousecop'         : 0x400b51cb52c3929d,
	b'Dan Smith'                                     : 0x088a7d587bd339f3,
        b'blaar'                                         : 0x4b64e9abbc760b0d
        }

fnvsum = 'util/fnvsum'



def fnvsum_stdin(string, expect_fnv):
    '''run fnvsum with 'string' as stdinput; verify that the output matches 'expect_fnv'
    '''
    try:
        sub = subprocess.run([fnvsum, '-'], input=string,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            shell=False, check=True);
    except subprocess.CalledProcessError as err:
        print(err.cmd)
        print(err.stdout.decode('ascii'))
        print(err.stderr.decode('ascii'))
        exit(1)
    
    res = int(sub.stdout.decode('ascii')[:16], 16)
    if res != expect_fnv:
        print('%xd != %xd;  %s' % (res, expect_fnv, string), sys.stderr)
        exit(1)



def fnvsum_file(string, expect_fnv):
    '''put 'string' in a file; run fnvsum against that file;
        verify that output matches 'expect_fnv'
    '''
    # write string to file
    with open('./temp', 'wb') as fl:
        fl.write(string)

    try:
        sub = subprocess.run([fnvsum, './temp'],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            shell=False, check=True);
    except subprocess.CalledProcessError as err:
        print(err.cmd)
        print(err.stdout.decode('ascii'))
        print(err.stderr.decode('ascii'))
        exit(1)
    
    res = int(sub.stdout.decode('ascii')[:16], 16)
    if res != expect_fnv:
        print('%xd != %xd;  %s' % (res, expect_fnv, string), sys.stderr)
        exit(1)

    os.remove('./temp');



#   main()
if __name__ == "__main__":
    if sys.argv[1] is not None:
        fnvsum = sys.argv[1]

    for string, fnv in hashes.items():
        fnvsum_stdin(string, fnv)
        fnvsum_file(string, fnv)
