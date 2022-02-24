import sys
import os 
import pandas as pd
import argparse


def transform(benchmark, n, dest_root):
    prj_root = os.path.dirname(os.path.dirname((os.path.abspath(__file__))))
    benchmark_dir = os.path.join(prj_root, "benchmark/", benchmark)
    trans_benchmark_dir = os.path.join(dest_root, "traces")
    if not os.path.exists(trans_benchmark_dir):
        os.mkdir(trans_benchmark_dir)

    # Transform the packet-centric trace to node-centric trace
    for father, _, files in os.walk(benchmark_dir):
        for file in files:
            try:
                df = pd.read_csv(os.path.join(father, file), sep=" ", index_col=False, header=None)
            except pd.errors.EmptyDataError:
                continue

            df.columns = ["ofs", "sx", "sy", "dx", "dy"]
            df["sid"] = df["sx"] * n + df["sy"]
            df["did"] = df["dx"] * n + df["dy"]

            # node_id, packet_number [offset, dest_node_id]
            with open(os.path.join(trans_benchmark_dir, file), "w") as wf:
                for sid, group in df.groupby("sid"):
                    srt_group = group.sort_values(by=["ofs", "sid"])
                    print("{} {}".format(sid, len(srt_group)), file=wf)
                    for _, line in srt_group.iterrows():
                        print("{} {}".format(line["ofs"], line["did"]), file=wf)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("name", help="The name of tested benchmark", type=str)
    parser.add_argument("n", help="Array size", type=int)
    parser.add_argument("wf", help="Working folder", type=str)

    args = parser.parse_args()
    transform(args.name, args.n, args.wf)
