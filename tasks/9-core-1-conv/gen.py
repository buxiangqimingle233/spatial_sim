
d = 3               # d
station_factor = 4  # I : W
tensor_cnt = 0
w_dims = []
in_dims = []


def get_id(x, y):
    return x * d + y


def do_conv(w, i, to):
    global tensor_cnt

    o = tensor_cnt
    tensor_cnt += 1
    conv = "compute # conv # {} # {}, {}".format(o, w, i)
    tensor = "{} # {} # ".format(o, to)
    return conv, tensor


def do_send(tos, type_):
    global tensor_cnt

    o = tensor_cnt
    tensor_cnt += 1
    man = "manipulate # 1 # 1 # {}".format(o)
    if type_ == "w":
        tensor = "{} # {} # {}".format(o, ",".join(tos), ",".join(w_dims))
    elif type_ == "in":
        tensor = "{} # {} # {}".format(o, ",".join(tos), ",".join(in_dims))
    return man, tensor


workers = [get_id(1, 0), get_id(2, 0), get_id(1, 1), get_id(1, 2)]
w_dict = {w: 0 for w in workers}
in_dict = {w: 0 for w in workers}
out_dict = {w: 0 for w in workers}

# send weights: unicast
with open("c0.inst", "w") as f0:
    mans, tensors = [], []
    for w in workers:
        m, t = do_send([w], "w")
        w_dict[w] = tensor_cnt - 1
        mans.append(m)
        tensors.append(m)

    print("operators:")
    for m in mans:
        print(m, file=f0)
    print("\ndata:")
    for t in tensors:
        print(t, file=f0)


# send inputs: multicast
with open("c1.inst", "w") as f1:
    mans, tensors = [], []
    m, t = do_send(workers, "w")
    for w in workers:
        in_dict[w] = tensor_cnt - 1
    mans.append(m)
    tensors.append(m)

    print("operators:")
    for m in mans:
        print(m, file=f1)
    print("\ndata:")
    for t in tensors:
        print(t, file=f1)


for w in workers:
    mans, tensors = [], []
    