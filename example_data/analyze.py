
with open('2022-11-20_example_data.csv', "r") as f:
    dat = f.read()
    lines = dat.split("\n")[4:]
    vals = [l.split(",")[6] for l in lines if l]

    vald = list(map(float, vals))

print(vald[:10])