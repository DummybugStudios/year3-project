#!env python3

# This runs the different algorithms multiple times
# and zips up the results for later

import os
import sys

totalruns = 0

def run(algo):
    for i in range(totalruns):
        status = os.system(f'./waf --run "project-example --algo={algo} --run={i} --RngRun={i+1} --scenario=2"')

        if (status != 0):
            exit("Run failed")

def zip(filename):
    os.chdir("results")
    filenames = [f"results{i}.csv" for i in range(totalruns)]
    os.system(f"zip {filename}.zip {' '.join(filenames)}")
    os.chdir("..")


if len(sys.argv) < 2:
    exit(f"usage: {sys.argv[0]} [number of runs]")

totalruns = int(sys.argv[1])



run(0)
zip("baseline")
run(1)
zip("aggregate")
run(2)
zip("trip")
