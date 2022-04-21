
# order: H, W, Kernel size, stride, XYxy
def to_frame(tox, toy, H, W, K=3, s=1) -> str:
    return "{:032b} {:032b} {:032b} {:032b} {:04b}{:04b}{:04b}{:04b}".format(H, W, K, s, 0, 0, tox, toy)


# Input: 32 x 64 x 64
# Output: 8 x 16 x 16
# Weight: 8 x 32 x 3 x 3
# tiling order: input channel -> H -> W
with open("f00", "w") as f0:
    for i in range(4 * 4 * 4):
        print(to_frame(0, 1, 16, 16), file=f0)
        print(to_frame(1, 0, 16, 16), file=f0)


with open("f01", "w") as f1:
    for i in range(4 * 4):
        for i in range(4 - 1):
            print(to_frame(1, 1, 16, 16), file=f1)      # imitate reducing input channels locally
        print(to_frame(1, 1, 16, 16), file=f1)


with open("f10", "w") as f2:
    for i in range(4 * 4):
        for i in range(4 - 1):
            print(to_frame(1, 1, 16, 16), file=f2)     # imitate reducing input channels locally
        print(to_frame(1, 1, 16, 16), file=f2)


with open("f11", "w") as f3:
    pass
