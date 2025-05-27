
import argparse
import glob
import pandas as pd
import matplotlib.pyplot as plt
from scipy.signal import find_peaks
from scipy.ndimage import gaussian_filter1d
from scipy.optimize import curve_fit
import numpy as np

def is_float(s):
    try:
        float(s)
        return True
    except ValueError:
        return False
    
def gaussian(x, A, mu, sigma):
    return A * np.exp(-(x - mu)**2 / (2 * sigma**2))


parser = argparse.ArgumentParser(description="Process and upload a data file.")

# Add arguments
parser.add_argument("-f", "--f", type=str, help="Input file name")
parser.add_argument("-r", "--r", type=int, help="Run number")

args = parser.parse_args()

run_number = args.r
file_name = args.f
abs_path = "/home/vlad/Documents/KRANOS/Data/"
abs_path += file_name + "/"
# Find all .csv chunked files
csv_list = glob.glob(abs_path +  "Run" + str(run_number) + "_list.csv*")
# Sort the files in chunk order if it is not simulation data
if run_number != 999:
    csv_list = sorted(csv_list, key=lambda f: int(f.split("-")[-1]))
# Save the data into a pandas data frame object
pd_data = []




for file in csv_list:
    print("Analyzing file: "  + file)
    with open(file, 'r') as f:
        for line in f:
            line = line.strip()
            # Skip empty lines and headers
            if line and is_float(line.split(",")[0]) and len(line.split(",")) == 9:  
                #print(line)
                parts = line.split(",")
                # Convert to appropriate types (float for timestamp, int for rest)
                parsed = [
                    float(parts[0]),                        
                    int(parts[1]),                          
                    int(parts[2]),
                    int(parts[3]),
                    parts[4],                       
                    int(parts[5]),
                    parts[6],                       
                    int(parts[7]),
                    int(parts[8])
                ]
                pd_data.append(parsed)

# Define column names
columns = [
    "TStamp_us",
    "Trg_Id",
    "Board_Id",
    "Num_Hits",
    "ChannelMask",
    "Ch_Id",
    "DataType",
    "PHA_LG",
    "PHA_HG"
]

# Create DataFrame
df_run = pd.DataFrame(pd_data, columns=columns)

print(df_run.head())

# Sum up LG and HG data per event basis, using the same Trg ID
event_sum = df_run.groupby("Trg_Id")[["PHA_LG", "PHA_HG"]].sum().reset_index()
PHA_HG = event_sum["PHA_HG"]

x = np.arange(len(PHA_HG))

counts, bin_edges = np.histogram(PHA_HG, bins='auto')
bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2

# Find peaks. To be tuned accordingly
peaks, properties = find_peaks(counts)

# Last detected peak should be the photopeak
photo_peak = peaks[-1]

threshold_fraction = 0.3
peak_height = counts[photo_peak]
threshold = threshold_fraction * peak_height

start = photo_peak
end = photo_peak

while counts[start] > threshold:
    start-= 1

while counts[end] > threshold:
    end+=1

x_fit = bin_centers[start:end]
y_fit = counts[start:end]

p0 = [np.max(y_fit), bin_centers[photo_peak], 10000]

#try:
#    popt, pcov = curve_fit(gaussian, x_fit, y_fit, p0=p0)
#    A, mu, sigma = popt
#    fwhm = 2.355 * sigma
#    energy_res = fwhm * 100 / mu
#    print(f"Photopeak center = {mu:.2f}, FWHM = {fwhm:.2f}, Energy Resolution = {energy_res:.2f} perc.")
#except Exception as e:
#    print("Fit failed:", e)

#fig, ax = plt.subplots()
#plt.plot(x_fit, gaussian(x_fit, *popt), label='Gaussian Fit', linestyle='--')
#plt.axvline(mu - fwhm/2, color='r', linestyle=':', label='FWHM')
#plt.axvline(mu + fwhm/2, color='r', linestyle=':')


#plt.plot(bin_centers, counts, label='PHA_HG', color = "blue", alpha = 0.2)
#plt.plot(bin_centers[photo_peak], counts[photo_peak], 'rx', label='PhotoPeak')

fig, ax = plt.subplots()
plt.hist(PHA_HG, bins = 'auto')#, label = f"Energy Resolution = {energy_res:.2f} perc.")
plt.plot(bin_centers[photo_peak], counts[photo_peak], 'rx', label='PhotoPeak')
plt.legend(title = "Energy spectrum NaI + Cs source 64 channel SiPM", title_fontsize = 24, fontsize = 24)


plt.show()
