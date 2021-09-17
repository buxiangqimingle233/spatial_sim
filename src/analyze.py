import pandas as pd

df = pd.read_csv(open("out.txt", "r"), header=None)
df.columns = ["node", "avg", "max", "min"]
max_avg = df["max"].max()
print(max_avg)