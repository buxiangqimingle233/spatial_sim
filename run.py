from subprocess import Popen, PIPE
import os 
from os.path import join
import re
import argparse

root = os.path.dirname(os.path.abspath(__file__))
bin_path = join(root, "src", "simulator")
tech_file_path = join(root, "src", "techfile.txt")
config_path = join(root, "runfiles", "focusconfig")

os.chdir(root)


def compile_kernel(mode):
    '''Make booksim in local at its source folder
    Input:
        mode: snn / ann
    '''
    # Compile kernel in snn mode
    src_dir = "src"
    os.chdir(join(root, src_dir))   # booksim2/src
    os.system("make clean && make {} -j > /dev/null 2>&1".format(mode))   # Compile twice to avoid bugs in compiling yacc
    os.system("make {} -j > /dev/null 2>&1".format(mode))

    print("INFO: Succeed in compiling kernel in {} mode\n".format(mode))


def simulate_all_benchmarks():
    '''
    Start simulation
    Working directory: booksim2/test/`benchmark_name`
    Trace directory: booksim2/test/`benchmark_name`/traces
    '''
    for benchmark in os.listdir(join(root, "benchmark")):
        simulate_one_benchmark(benchmark)


def simulate_one_benchmark(benchmark, config_file_path=config_path, kernel=bin_path):

    # Make & clane the working directory
    test_dir = join(root, "test")       # booksim2/test
    working_dir = join(test_dir, benchmark)
    if not os.path.exists(working_dir):
        os.mkdir(working_dir)
    os.system("rm -rf {}/*".format(working_dir))
    os.chdir(working_dir)

    # Copy the technical file and the trace files 
    os.system("/bin/cp {} {}".format(tech_file_path, working_dir))
    os.system("/bin/cp -rf {} {}".format(join(root, "benchmark", benchmark), join(working_dir, "traces")))

    # Simulate all cases in parallel
    workers = []
    traces = join(working_dir, "traces")
    for father, _, files in os.walk(traces):
        for case in files:
            trace_path = join(father, case)
            cmd = "{} {} {}".format(kernel, config_file_path, trace_path)
            workers.append(
                (case, Popen(cmd, cwd=working_dir, shell=True, stdout=PIPE, preexec_fn=os.setpgrp))
                # case, Popen(cmd, cwd=working_dir, shell=True, preexec_fn=os.setpgrp)
            )

    # Extract processing cycles
    print("INFO: Gernerate a brief report\n")
    sim_results = []
    with open(join(working_dir, "run.log"), "w") as logf:
        for case, w in workers:
            out = w.communicate()[0].decode("utf-8")
            print(out, file=logf)
            try:
                pattern = re.compile(r"Time taken is ([0-9]+)")
                ret = pattern.findall(out)[0]
            except IndexError:
                print("ERROR: Benchmark{}, case{} failed in simulation".format(benchmark, case))
                ret = -1

            bit_width = re.findall(r"(\d+)", case)[0]
            full_name = "{}_{}".format(benchmark, bit_width)     # `benchmark`_`flitsize`
            sim_results.append(
                (full_name, bit_width, int(ret))
            )

    print("="*80, "="*80, sep="\n")
    print("Simulated performance for benchmark {} is listed in below: \n".format(benchmark))

    with open(join(working_dir, "brief_report.csv"), "w") as f:
        # Export short results
        for name, bit_width, cycle in sim_results:
            print("Case {} runs {} cycles".format(name, cycle))
            print(name, bit_width , cycle, file=f, sep=",")

    return sim_results


def compare_two_benchmarks(benchmark1, benchmark2):
    cycles1 = simulate_one_benchmark(benchmark1)
    cycles2 = simulate_one_benchmark(benchmark2)

    print("="*80, "="*80, sep="\n")
    for c1, c2 in zip(cycles1, cycles2):
        if c1 > c2:
            speedup = (c1 - c2) / c2
            print("{} uses {:%} more cycles than {}\n".format(benchmark1, speedup, benchmark2))
        if c2 >= c1:
            speedup = (c2 - c1) / c2
            print("{} uses {:%} less cycles than {}\n".format(benchmark1, speedup, benchmark2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "mode", 
        help="all: [test all snn benchmarks], \
            single: [test a single snn benchmark], \
            comp: [compare two benchmarks]. \
            hybrid: [run flappybird with hybrid mode].    \
            build: [build the kernel].   \
            if comp or single is specified, please also specify --bm")
    parser.add_argument("--bm", nargs="+", help="Benchmarks to test")
    parser.add_argument("--target", help="Which mode is the kernel built ( snn / ann )")
    args = parser.parse_args()

    if args.mode == "build":
        target = args.target
        if target not in ["snn", "ann"]:
            print("ERROR: Undefined build mode")
        else:
            compile_kernel(target)
    elif args.mode == "all":
        simulate_all_benchmarks()
    elif args.mode == "single":
        simulate_one_benchmark(args.bm[0])
    elif args.mode == "comp":
        compare_two_benchmarks(*args.bm)
    else:
        print("ERROR: Undefined running mode")
