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
                for sz_exp in range(8, 21) ] #21

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

#from matplotlib.colors import LogNorm

def make_plots_alt(bm):
    lin_x = bm['X_ratio'] # for legibility only
    log_y = [ log(y, 2) for y in bm['Y_size'] ]
    lin_y = [ y for y in bm['Y_size'] ] # used for labels
    X, Y = np.meshgrid(lin_x, log_y)

    # Repeat plotting work for the following:
    do_plots = { "Z_inef" : ("Inefficiency", "inefficiency"),
                "Z_enc" : ("Encode Bitrate (Gb/s)", "encode" ),
                "Z_dec" : ("Decode Bitrate (Gb/s)", "decode" )
                }

    for k, v in do_plots.items():
        lin_z = bm[k]
        Z = np.array(lin_z).reshape(X.shape) 

        _, ax = plt.subplots()
        p = plt.pcolormesh(X, Y, Z, cmap=cm.get_cmap('Vega20'), axes=ax,
                            vmin=bm[k][0], vmax=bm[k][-1])

        ax.set_xlabel('ratio')
        ax.set_xticks(lin_x, minor=False)
        ax.set_ylabel('block size (log2)')
        ax.set_yticks(log_y, minor=False)
        ax.set_yticklabels(lin_y)

        # standard label formatting
        ax.tick_params(labelsize = 5)
        ax.set_frame_on(False)
        ax.grid(True)

        # colorbar
        cb = plt.colorbar(p, ax=ax, orientation='horizontal', aspect=20)
        cb.ax.set_xlabel(v[0])
        # generate precisely 20 tick values, so they will fit on the borders
        #+  of the 20 colormap squares
        step = (lin_z[-1] - lin_z[0]) / 20
        ticks = [ lin_z[0] + step * i for i in range(20) ]
        cb.set_ticks(ticks)
        # truncate the tick label to 5 characters without affecting the values
        cb.set_ticklabels([ '%.5s' % z for z in ticks ])
        # standard label formatting
        cb.ax.tick_params(labelsize = 5)
        cb.ax.set_frame_on(False)
        cb.ax.grid(True)

        # layout; save
        plt.title('ffec: [commit]'.format(v[0]))
        plt.tight_layout()
        plt.savefig('{0}.pdf'.format(v[1]), dpi=600,
                    papertype='a4', orientation='landscape')


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
        ax.plot_surface(X, Y, Z, cmap=cm.gist_ncar, antialiased=False, linewidth=0, shade=True)

        #ticks
        ax.set_xlabel('ratio')
        ax.set_xticks(bm['X_ratio'], minor=True)
        ax.minorticks_on()
        ax.invert_xaxis()
#        ax.tick_params(axis='x',which='minor',bottom='off')
        ax.set_ylabel('block size (log2)')
        ax.invert_yaxis()
#        ax.axis(v = 'tight')

        # labels
        ax.tick_params(labelsize = 6)
        ax.set_frame_on(False)
        ax.grid(True)

        # layout; save
        plt.title('ffec: {0}; [commit]'.format(v[0]))
#        plt.tight_layout()
        plt.savefig('{0}.pdf'.format(v[1]), dpi=600,
                    papertype='letter', orientation='landscape')



def current_commit():
    '''returns the current git commit; or dies
    '''
    # get current git commit; ergo must run inside repo
    try:
        #TODO: This is hardcoded. That is bad.
        sub = subprocess.run([ 'git', 'rev-parse', '--short', 'HEAD' ],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            shell=False, check=True);
    except subprocess.CalledProcessError as err:
        print(err.cmd)
        print(err.stdout.decode('ascii'))
        print(err.stderr.decode('ascii'))
        exit(1)

    return sub.stdout.decode('ascii')



import sys
import os
import json
import socket
from datetime import datetime

if __name__ == "__main__":

    # allow caller to specify the filename, otherwise assume 'benchmark.json'
    if len(sys.argv) > 1:
        filename = sys.argv[1]
    else:
        filename = '{0}.{1}.{2}.bench.json'.format(
                datetime.now().date(),
                current_commit(),
                socket.gethostname())

    # look for an existing benchmark.json; if exists load that one
    if os.path.isfile(filename):
        with open(filename) as f:
            bm = json.load(f)
    else:
        bm = gen_benchmark()
        # dump to file
        with open(filename, 'w') as f:
            json.dump(bm, f)

#    make_plots(bm)
    make_plots_alt(bm)
