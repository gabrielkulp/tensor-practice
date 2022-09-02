#!/usr/bin/env python3
import subprocess
import os
import matplotlib.pyplot as plt

# branching factor, overprovision factor, RAM portion, storage portion
# portions are B+ tree stats divided by hash table stats
runs = []

orders = [4, 6, 8, 16, 24, 32]
overprovs = [1.1, 1.15, 1.2, 1.25, 1.3, 1.35, 1.4, 1.45, 1.5]
orders.reverse()

print("generating tensor...")
os.system("../generator.py .5 10 10 10 10 > ../B.coo")
print("running tests...")

comment = ""
lines = []
runs = []
# with open("test-data.pyobj", "r") as f:
#     comment = str(f.readline())
#     runs = eval(f.read(1000000000))
if not runs:
    for order in orders:
        for overprov in overprovs:
            cmd = f"gcc -DBPT_ORDER={order} -DHT_OVERPROVISION={overprov} *.c"
            print(cmd[4:-4])
            os.system(cmd)
            output = subprocess.check_output("./a.out", shell=True)
            lines = output.split(b'\n')
            runs.append({
                "branching": int(lines[-10].split(b' ')[-1]),
                "overprovision": float(lines[-9].split(b' ')[-1]),
                "RAM": float(lines[-4].split(b' ')[-1][:-1])/100.0,
                "size": float(lines[-2].split(b' ')[-1][:-1])/100.0
            })
            if runs[-1]["branching"] != order:
                print("what1")
            if runs[-1]["overprovision"] != overprov:
                print("what2")
    os.system("rm a.out")
    comment = ", ".join([x.decode().strip() for x in lines[-8:-6]])
with open("test-data.pyobj", "w") as f:
    f.write(comment+"\n")
    f.write(str(runs))


def show_data(data, title):
    plt.title(title + " (lower is better)")
    plt.imshow(data, interpolation='nearest')
    plt.colorbar()
    plt.xticks(range(len(overprovs)), overprovs)
    plt.xlabel("Hashtable overprovision factor")
    plt.yticks(range(len(orders)), orders)
    plt.ylabel("B+ Tree branching factor")
    plt.show()


# RAM access count
ram_data = []
i = 0
for order in orders:
    ram_data.append([])
    for overprov in overprovs:
        ram_data[-1].append(runs[i]["RAM"])
        i += 1
show_data(ram_data, "RAM transaction count comparison")

# estimated RAM transfer size
throughput_data = []
i = 0
for order in orders:
    throughput_data.append([])
    for overprov in overprovs:
        throughput_data[-1].append(runs[i]["RAM"] * runs[i]["branching"])
        i += 1
show_data(throughput_data, "RAM data transfer volume comparison")

# data structure size
size_data = []
i = 0
for order in orders:
    size_data.append([])
    for overprov in overprovs:
        size_data[-1].append(runs[i]["size"])
        i += 1
show_data(size_data, "Datastructure size comparison")
