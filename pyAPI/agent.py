import os
import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd
import build.lib.simulator as S


class Simulator:
    '''Agent simulator invoking
    '''

    def __init__(self, working_dir, spec) -> None:
        self.working_dir = working_dir
        self.spec = spec
        prev_cwd = os.getcwd()
        os.chdir(self.working_dir)
        self.instance = S.SpatialChip(self.spec)
        os.chdir(prev_cwd)

    def run(self) -> int:
        self.cycle = self.instance.run()
        return self.cycle

    def compute_cycles(self) -> list:
        return self.instance.compute_cycles()
    
    def communicate_cycles(self) -> list:
        return self.instance.communicate_cycles()
    
    def busy_ratio(self) -> list:
        core_busy_ratio = [comp / (comm + comp + 1) for comp, comm in zip(self.compute_cycles(), self.communicate_cycles())]
        return core_busy_ratio
    
    def print_stats(self):
        print("Overall Cycles: {}, Core busy ratio: {}".format(self.cycle, self.busy_ratio()))
    
    def plot_busy_ratio(self, fig_path, ignore):
        df = pd.DataFrame({"busy_ratio": [i for i in self.busy_ratio() if i not in ignore]})
        plot = sns.histplot(data=df, x="busy_ratio", bins=10, binrange=(0, 1), common_norm=True)
        fig = plot.get_figure()
        fig.savefig(fig_path)