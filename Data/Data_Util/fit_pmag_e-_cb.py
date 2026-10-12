import ROOT, math

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptFit(1111)
base_path = "/Users/snip/Documents/MALTA_VLAD_13/malta_simulation/Data/"
FILE = base_path + "pdgsensor_af_electron_allfiles.root"
TREE = "ForwardMPGDEndcapHits_maxsensor_tree"
OUT = base_path + "New_fit_results/electron_pmag_simple_fit.png"
OUT_ROOT = base_path + "New_fit_results/electron_pmag_simple_fit.root"
OUT_TXT = base_path + "New_fit_results/electron_cb_par_pmag.txt"


f = ROOT.TFile(FILE, "READ")
t = f.Get(TREE)

# flatten p_mag into a histogram
h = ROOT.TH1D("h", "electron p_mag;|p| [GeV/c];Entries", 1000, 0.0, 18.0)
h.SetDirectory(0)  # detach from the open file BEFORE closing it
vec = ROOT.vector("double")()
t.SetBranchAddress("p_mag", vec)
n = t.GetEntries()
for i in range(n):
    t.GetEntry(i)
    for v in vec:
        h.Fill(v)
f.Close()

#xmin = h.GetXaxis().GetXmin()
#xmax = h.GetXaxis().GetXmax()
xmin = 0.0005
xmax = 18.
# Use ROOT's built-in C++ crystal ball function
cb = ROOT.TF1("cb", "crystalball", xmin, xmax)

# Parameter order: [0]=Constant, [1]=Mean, [2]=Sigma, [3]=Alpha, [4]=n
cb.SetParameters(
    h.GetMaximum(),                      # [0] Constant
    h.GetBinCenter(h.GetMaximumBin()),   # [1] Mean
    max(h.GetRMS(), 1e-3),               # [2] Sigma
    -1.0,                                # [3] Alpha (NEGATIVE for a right-side tail)
    3.0                                  # [4] n 
)

cb.SetParLimits(0, 0, 1e12)      # Constant
cb.SetParLimits(1, xmin, xmax)   # Mean
cb.SetParLimits(2, 1e-4, 1e3)    # Sigma
cb.SetParLimits(3, -50, -0.01)   # Alpha (Forced negative to keep the tail on the right)
cb.SetParLimits(4, 0.1, 50)      # n

# R = Use range of TF1, Q = Quiet mode, S = return TFitResultPtr
r = h.Fit(cb, "RQS")

chi2 = cb.GetChisquare()
ndf = cb.GetNDF()
print(f"chi2/ndf = {chi2:.3g}/{ndf} = {chi2/ndf:.3g}  prob={r.Prob():.3g}")

for i in range(cb.GetNpar()):
    print(f"p[{i}] = {cb.GetParameter(i):.6g} +- {cb.GetParError(i):.6g}")

# --- plot ---
c = ROOT.TCanvas("c", "c", 800, 600)
c.SetLogy()
c.SetLogx()
h.SetMinimum(0.001)
h.Draw("E1")
cb.SetLineColor(ROOT.kRed)
cb.Draw("same")
c.SaveAs(OUT)
c.SaveAs(OUT_ROOT)
print("saved", OUT)

# --- Save parameters to text file ---
with open(OUT_TXT, "w") as f_txt:
    # Extract all parameters into a list of strings
    params = [str(cb.GetParameter(i)) for i in range(cb.GetNpar())]
    # Join them with a space and write to the file
    f_txt.write(" ".join(params) + "\n")
print("saved parameters to", OUT_TXT)