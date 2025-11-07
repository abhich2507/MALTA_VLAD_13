import numpy as np
import matplotlib.pyplot as plt

# Define the function
def f(x, s):
    return 390.0 / np.power((x / s) - 149.8, 0.65)

# Define the function
def f_t110(x):
    return 236.0 / np.power(x - 104.3, 0.83)

# Define the function
def f_t200(x):
    return 2152.0 / np.power(x - 199.6, 0.85)

# Define x range (avoid x near 149.8*s to prevent division by zero)
xmax = 2700
x = np.linspace(1, xmax, 3000)
#x = np.linspace(2000, 2200, 1000)

# Scale values
scales = [1.1/1.5, 1, 2/1.5, 1000./150., 2000./150.]
#scales = [20, 1, 2/1.5, 4, 8]
colors = ['blue', 'green', 'red', 'black', "orange"]

# Plot
plt.figure(figsize=(8, 6))
for s, c in zip(scales, colors):
    plt.plot(x, f(x, s), label=f'thr={s*150:.0f}e-', color=c)

plt.plot(x, f_t110(x), label=f'thr=110e- (data)', linestyle="--", color = "blue")
plt.plot(x, f_t200(x), label=f'thr=200e- (data)', linestyle="--", color = "red")

plt.xlabel('[Charge e-]')
plt.ylabel('timewalk [ns]')
# Limit only the upper y bound
plt.ylim(-1, 200)
plt.xlim(0, xmax)
plt.title(r'$t = 390 / ((x*(150/threshold) - 149.8)^{0.65})$')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("TimeWalkThresholdScaling.pdf", format="pdf")  
plt.show()
