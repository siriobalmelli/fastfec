#!/usr/bin/env python3

#   ffec_bench.py
#
# Quick tool to run ffec repeatedly and get performance measurements
# It's disgusting - needs thorough pythonization - but fails to suck sufficiently
#+  for the purpose of some quick measurements.
#
# (c) 2017 Sirio Balmelli



from math import ceil

def confidence_guess(block_size):
    '''Guess at the number of iterations necessary to provide a confident average.
    Returns suggested number of iterations.

    As block_size increases, we want less iterations.

    block_size          :   in Bytes
    '''
    confidence = int(ceil(1000000000 * (1.0 / block_size)))
    if confidence < 5:
        confidence = 5;
    print("block_size {0} confidence {1}".format(block_size, confidence))
    return confidence



import subprocess
import re
rex = re.compile('.*inefficiency=([0-9.]+).*enc=([0-9]+).*dec=([0-9]+).*', re.DOTALL)

def run_single(block_size, fec_ratio):
    '''run ffec_test once with 'block_size' and 'fec_ratio'

    returns:
        inefficiency    :   float > 1.0
        encode_bitrate  :   Mb/s
        decode_bitrate  :   Mb/s
    '''
    try:
        #TODO: This is hardcoded. That is bad.
        sub = subprocess.run(["test/ffec_test", "-f {}".format(fec_ratio), "-o {}".format(block_size)],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            shell=False, check=True);
    except subprocess.CalledProcessError as err:
        print(err.cmd)
        print(err.stdout.decode('ascii'))
        print(err.stderr.decode('ascii'))
        exit(1)
    
    res = rex.match(sub.stdout.decode('ascii'))
    try:
        return float(res.group(1)), int(res.group(2)), int(res.group(3))
    except:
        print("regex didn't match!")
        print(sub.stdout.decode('ascii'))
        exit(1)


from statistics import mean

def run_average(block_size, fec_ratio):
    '''run tests a large amount of times; average results
    returns mean values for inef, enc, dec
    '''
    runs = []
    for i in range(confidence_guess(block_size)):
        runs.append(run_single(block_size, fec_ratio))
    return mean(a[0] for a in runs), mean(a[1] for a in runs), mean(a[2] for a in runs)



import json

if __name__ == "__main__":
    # TODO: take range arguments from the command line

    # test across an exponential range of sizes
    symbol_sz = 1280
    X_size = [ 2**sz_exp * symbol_sz
                for sz_exp in range(8, 10) ] #20
    # test for a linear gradient of fec_ratios 
    Y_ratio = [ 1.0 + (ratio_decimal / 100)
                for ratio_decimal in range(1, 3) ] #16

    # execute the runs
    Z_inef, Z_enc, Z_dec = map(list,zip(*[ (run_average(size, fec_ratio))
                        for size in X_size
                        for fec_ratio in Y_ratio ]))

    # dump to file
    benchmark = { "X_size" : X_size, "Y_ratio" : Y_ratio, 
            "Z_inef" : Z_inef, "Z_enc" : Z_enc, "Z_dec" : Z_dec }
    with open('benchmark.json', 'w') as f:
        json.dump(benchmark, f)

    #TODO: 3D plot!
