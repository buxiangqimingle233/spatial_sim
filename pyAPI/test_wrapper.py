import os
from time import time
import build.lib.simulator as s

path = "tasks/bert-large_b8w1024_8x8/spatial_spec"
spatial_chip = s.SpatialChip(path)
start = time()
spatial_chip.run()
spatial_chip.
end = time()

# start2 = time()
# os.system("build/bin/spatialsim {}".format(path))
# end2 = time()

with open("res", "w") as f:
    print("wrapper takes: {}".format(end - start), file=f)
    print("executer takes: {}".format(end2 - start2), file=f)
