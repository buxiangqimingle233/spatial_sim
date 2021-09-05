import pandas as pd
with open("rate.txt", "r") as f:
    df = pd.Series(f.readline().split(" "))

df = df.astype("float")
print(len(df))