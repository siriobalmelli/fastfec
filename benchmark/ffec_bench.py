#!/usr/bin/env python3

#   ffec_bench.py
#
# Quick tool to run ffec repeatedly and get performance measurements
# It's disgusting - needs thorough pythonization - but fails to suck sufficiently
#+  for the purpose of some quick measurements.
#
# JSON naming format:
#   $(date +%Y-%m-%d).$(printf "%.8s" $(git rev-parse HEAD)).$(hostname).bench.json
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
        return float(res.group(1)), float(res.group(2)) / 1000, float(res.group(3)) / 1000
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



import numpy as np 

def gen_benchmark():
    '''actually run the benchmark.
    Return a dictionary of results.
    '''
    ret = {}

    # test for a linear gradient of fec_ratios in 0.5% increments until 10%
    ret['X_ratio'] = [ 1.0 + (i / 1000) for i in range (1, 105, 5) ]
    # test across an exponential range of sizes
    ret['symbol_sz'] = 1280
    ret['Y_size'] = [ 2**sz_exp * ret['symbol_sz']
                for sz_exp in range(8, 20) ] #20

    # Generate mesh of graphing coordinates against which
    #+  to run tests (so they execute in the right order!)
    X, Y = np.meshgrid(ret['X_ratio'], ret['Y_size'])

    # execute the runs
    # NOTE: don't use a list comprehension: avoid map(list, zip())
    #+  and more explicitly show loop nesting (important when making a mesh to plot, later)
    ret['Z_inef'], ret['Z_enc'], ret['Z_dec'] = map(list,zip(*[ 
                            run_average(fec_ratio = X[i][j], block_size = Y[i][j])
                                for i in range(len(X)) 
                                for j in range(len(X[0]))
                            ]))

    return ret


from math import log
from mpl_toolkits.mplot3d import Axes3D
#do not use Xwindows 
#Needs to be called before pyplot is imported
import matplotlib 
matplotlib.use('Agg')
import matplotlib.pyplot as plt 
from matplotlib import cm

def format_plot(ax):
    '''common plot formatting code in one place.
    ax : a subplot.
    '''

def make_plots(bm):
    '''take a benchmarks dictionary 'bm' and write plots to disk
    '''
    X, Y = np.meshgrid(bm['X_ratio'], [ log(y, 2) for y in bm['Y_size'] ])

    # Repeat plotting work for the following:
    do_plots = { "Z_inef" : ("Inefficiency", "inefficiency"),
                "Z_enc" : ("Encode Bitrate (Gb/s)", "encode" ),
                "Z_dec" : ("Decode Bitrate (Gb/s)", "decode" )
                }

    for k, v in do_plots.items():
        # plot
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')
        Z = np.array(bm[k]).reshape(X.shape) 
        ax.plot_surface(X, Y, Z, cmap=cm.gist_ncar, antialiased=True, linewidth=0, shade=False)
        # labels
        ax.tick_params(labelsize = 6)
        ax.set_frame_on(False)
        ax.grid(True)
#        ax.axis(v = 'tight', xmin = bm['X_ratio'][0], xmax = bm['X_ratio'][-1],
#                ymin = log(bm['Y_size'][0], 2), ymax = log(bm['Y_size'][-1], 2))
        ax.set_xlabel('Ratios')
        ax.invert_xaxis()
        ax.set_ylabel('Sizes (log2)')
        ax.invert_yaxis()
        # layout; save
        plt.title('ffec; {0}; [commit]'.format(v[0]))
        plt.tight_layout()
        plt.savefig('{0}.pdf'.format(v[1]), dpi=600,
                    papertype='letter', orientation='landscape')



import os
import json

if __name__ == "__main__":

    # look for an existing benchmark.json; if exists load that one
    filename = 'benchmark.json'
    if os.path.isfile(filename):
        with open(filename) as f:
            bm = json.load(f)
    else:
        bm = gen_benchmark()
        # dump to file
        with open(filename, 'w') as f:
            json.dump(bm, f)

    make_plots(bm)
