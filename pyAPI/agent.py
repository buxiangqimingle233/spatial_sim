import os
import build.lib.simulator as S


class Simulator:
    '''Agent simulator invoking
    '''

    def __init__(self, working_dir, spec) -> None:
        self.working_dir = working_dir
        self.spec = spec

    def run(self) -> int:
        prev_cwd = os.getcwd()
        os.chdir(self.working_dir)

        instance = S.SpatialChip(self.spec)
        cycle = instance.run()
        
        os.chdir(prev_cwd)
        return cycle
