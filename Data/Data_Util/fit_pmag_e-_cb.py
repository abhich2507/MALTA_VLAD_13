

import ROOT, math

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptFit(1111)
base_path = "/Users/snip/Documents/MALTA_VLAD_13/malta_simulation/Data/"
FILE = base_path + "pdgsensor_af_electron_allfiles.root"
TREE = "ForwardMPGDEndcapHits_maxsensor_tree"
OUT = base_path + "New_fit_results/electron_pmag_simple_fit.png"
OUT_ROOT = base_path + "New_fit_results/electron_pmag_simple_fit.root"
OUT_TXT = base_path + "New_fit_results/electron_cb_par_pmag.txt"
OUT_PDF = base_path + "New_fit_results/electron_cb_par_pmag.pdf"


f = ROOT.TFile(FILE, "READ")
t = f.Get(TREE)

# flatten p_mag into a histogram
h = ROOT.TH1D("h", ";|p| [GeV/c];Entries", 50, 0.0, 20.0)
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
xmin = 0.001
xmax = 20.0
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
h.SetStats(0)

h.Scale(1.0 / h.Integral())  # Normalize histogram to unit area
r = h.Fit(cb, "RQS")

chi2 = cb.GetChisquare()
ndf = cb.GetNDF()
print(f"chi2/ndf = {chi2:.3g}/{ndf} = {chi2/ndf:.3g}  prob={r.Prob():.3g}")

for i in range(cb.GetNpar()):
    print(f"p[{i}] = {cb.GetParameter(i):.6g} +- {cb.GetParError(i):.6g}")

# --- plot ---
c = ROOT.TCanvas("c", "c", 1100, 700)
c.SetLogy()
#c.SetLogx()
h.GetXaxis().SetRangeUser(0., 20.)
h.GetYaxis().SetRangeUser(0.000005, 1500.5)
h.SetLineWidth(1)
h.GetXaxis().SetTitleSize(0.045)
h.GetXaxis().SetLabelSize(0.045)
h.GetYaxis().SetTitleSize(0.045)
h.GetYaxis().SetLabelSize(0.045)
h.GetYaxis().SetTitleOffset(0.85)
gpad = ROOT.gPad
gpad.SetLeftMargin(0.09)
gpad.SetRightMargin(0.02)
gpad.SetBottomMargin(0.14)
gpad.SetTopMargin(0.05)
h.Draw("E1")
TLatex = ROOT.TLatex()
TLatex.SetNDC()
TLatex.SetTextFont(42)
TLatex.SetTextSize(0.048)
TLatex.DrawLatex(0.55, 0.90, f"#chi^{2} / ndf = {chi2:.3g}/{ndf} = {chi2/ndf:.3g}")
TLatex.DrawLatex(0.55, 0.83, f"p-value = {r.Prob():.3g}")
TLatex.DrawLatex(0.55, 0.76, f"#mu = {cb.GetParameter(1):.3g} #pm {cb.GetParError(1):.3g}")

TLatex.DrawLatex(0.55, 0.69, f"#sigma = {cb.GetParameter(2):.3g} #pm {cb.GetParError(2):.3g}")
TLatex.DrawLatex(0.55, 0.62, f"#alpha = {cb.GetParameter(3):.3g} #pm {cb.GetParError(3):.3g}")
TLatex.DrawLatex(0.55, 0.55, f"n = {cb.GetParameter(4):.3g} #pm {cb.GetParError(4):.3g}")
cb.SetLineColor(ROOT.kRed)
cb.Draw("same")
c.SaveAs(OUT)
c.SaveAs(OUT_ROOT)
c.SaveAs(OUT_PDF)
print("saved", OUT)

# --- Save parameters to text file ---
# with open(OUT_TXT, "w") as f_txt:
#     # Extract all parameters into a list of strings
#     params = [str(cb.GetParameter(i)) for i in range(cb.GetNpar())]
#     # Join them with a space and write to the file
#     f_txt.write(" ".join(params) + "\n")
# print("saved parameters to", OUT_TXT)