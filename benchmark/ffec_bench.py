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



def run_benchmark():
    '''actually run the benchmark.
    Return a dictionary of results.
    '''
    ret = {}

    # test for a linear gradient of fec_ratios 
    ret['X_ratio'] = [ 1.0 + (ratio_decimal / 100)
                for ratio_decimal in range(1, 16) ] #16
    # test across an exponential range of sizes
    ret['symbol_sz'] = 1280
    ret['Y_size'] = [ 2**sz_exp * ret['symbol_sz']
                for sz_exp in range(8, 20) ] #20

    # execute the runs
    ret['Z_inef'], ret['Z_enc'], ret['Z_dec'] = map(list,zip(*[ (run_average(size, fec_ratio))
                        for size in ret['Y_size']
                        for fec_ratio in ret['X_ratio'] ]))

    return ret



import math
import numpy as np 
from mpl_toolkits.mplot3d import Axes3D
import matplotlib 
#do not use Xwindows 
#Needs to be called before pyplot is imported
matplotlib.use('Agg')
import matplotlib.pyplot as plt 

def make_plots(bm):
    '''take a benchmarks dictionary 'bm' and write plots to disk
    '''
    #
    #   prep 3D plots
    #
    # Y axis (sizes) plotted logarithmically
    Y_sizes_log = [math.log(l,2) for l in bm['Y_size'] ]
    # If interval is 1 for y, then y is only 1 value, there have to be 2 for it to work
    # Seems that the generated Y, X from np.meshgrid have to be a multiple of the
    # total number of values in Z
    y = np.arange(Y_sizes_log[0], Y_sizes_log[-1], 0.5) 
    # X axis (ratios)
    x = np.arange(bm['X_ratio'][0], bm['X_ratio'][-1], 0.01)
    X, Y = np.meshgrid(x,y)

    # plot inefficiency
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    Z = np.array(bm['Z_inef']).reshape(X.shape) 
    ax.plot_surface(X, Y, Z)
    ax.set_xlabel('Ratios')
    ax.set_ylabel('Sizes (log2)')
    ax.set_zlabel('Inefficiency')
    plt.savefig('inefficiency.png', dpi=300)

    # Print the values of each array to see 
    #print(x)
    #print(y)
    #print(X)
    #print(Y)
    #print(Z)

    # plot encode bitrate
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    Z = np.array(bm['Z_enc']).reshape(X.shape) 
    ax.plot_surface(X, Y, Z)
    ax.set_xlabel('Ratios')
    ax.set_ylabel('Sizes (log2)')
    ax.set_zlabel('Encode Bitrate (Mb/s)')
    plt.savefig('encode.png', dpi=300)

    # plot decode bitrate
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    Z = np.array(bm['Z_dec']).reshape(X.shape) 
    ax.plot_surface(X, Y, Z)
    ax.set_xlabel('Ratios')
    ax.set_ylabel('Sizes (log2)')
    ax.set_zlabel('Decode Bitrate (Mb/s)')
    plt.savefig('decode.png', dpi=300)



import os
import json

if __name__ == "__main__":

    # look for an existing benchmark.json; if exists load that one
    filename = 'benchmark.json'
    if os.path.isfile(filename):
        with open(filename) as f:
            bm = json.load(f)
    else:
        bm = run_benchmark()
        # dump to file
        with open(filename, 'w') as f:
            json.dump(bm, f)

    make_plots(bm)
