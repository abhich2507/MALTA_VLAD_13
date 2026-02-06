import numpy as np
import matplotlib.pyplot as plt

# Define the function
def f(x, threshold, T, TrefThr, x0, n, t0=0.):
    return T / np.power((x * TrefThr / threshold) - x0, n) + t0

# Define the function
def f_t110(x):
    return 236.0 / np.power(x - 104.3, 0.83)

# Define the function
def f_t150(x):
    return 390.0 / np.power(x - 149.8, 0.65)

# Define the function
def f_t200(x):
    return 2152.0 / np.power(x - 199.6, 0.85)

# Define x range (avoid x near 149.8*s to prevent division by zero)
xmax = 2700
x = np.linspace(1, xmax, 3000)
#x = np.linspace(2000, 2200, 1000)

# Scale values
thresh = [110, 150., 200., 1000., 2000.]
colors = ['blue', 'green', 'red', 'black', "orange"]

## Parameters for threshol reference:
## 150 e-
T = 390.
TrefThr=150.
x0=149.8
n=0.65
t0=0.

## 200 e-
#T = 2152.
#TrefThr=200. 
#x0=199.6
#n=0.85
#t0=0.

# Plot
plt.figure(figsize=(8, 6))
for t, c in zip(thresh, colors):
    plt.plot(x, f(x, t, T, TrefThr, x0, n, t0), label=f'thr={t:.0f}e-', color=c)

plt.plot(x, f_t110(x), label=f'thr=110e- (data)', linestyle="--", color = "blue")
plt.plot(x, f_t150(x), label=f'thr=150e- (data)', linestyle="--", color = "green")
plt.plot(x, f_t200(x), label=f'thr=200e- (data)', linestyle="--", color = "red")

plt.xlabel('[Charge e-]')
plt.ylabel('timewalk [ns]')
# Limit only the upper y bound
plt.ylim(-1, 200)
plt.xlim(0, xmax)
plt.title('$t = '+"{:.1f} / ((x*({:.1f} /threshold) - {:.1f})^{{{:.2f}}})$".format(T, TrefThr, x0, n))
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("TimeWalkThresholdScaling.pdf", format="pdf")  
plt.show()
