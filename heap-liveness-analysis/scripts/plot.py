#!/usr/bin/python3

import matplotlib.pyplot as plt
import numpy as np

xpoints = np.array([2, 4, 5, 6, 7, 8])
ypoints = np.array([2, 4, 5, 6, 7, 8])

plt.plot(xpoints, ypoints, linestyle='--', marker='o', color='b')
plt.xlabel("No of free calls inserted")
plt.ylabel("No. of malloc calls in the program")
# plt.show()
plt.savefig("./fig")

