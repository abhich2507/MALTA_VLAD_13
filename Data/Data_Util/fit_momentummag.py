import ROOT, math, os

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptFit(1111)

PATH = "/Users/snip/Documents/MALTA_VLAD_13/malta_simulation/Data/pdgsensor_af_electron_allfiles.root"
HIST = "ForwardMPGDEndcapHits/1415.0/ForwardMPGDEndcapHits.1415.0.maxsensor.momentummag"
OUTDIR = "/Users/snip/Documents/MALTA_VLAD_13/malta_simulation/Data/New_Data"
os.makedirs(OUTDIR, exist_ok=True)

f = ROOT.TFile(PATH, "READ")
if f.IsZombie():
    raise SystemExit(f"Cannot open {PATH}")
h = f.Get(HIST)
if h is None:
    raise SystemExit(f"No '{HIST}' in {PATH}")
h.SetDirectory(0)
f.Close()

nbins = h.GetNbinsX()
xmin = h.GetXaxis().GetXmin()
xmax = h.GetXaxis().GetXmax()
print(f"nbins={nbins} range=[{xmin},{xmax}] entries={h.GetEntries():.0f} integral={h.Integral():.5g}")
print(f"mean={h.GetMean():.4g} RMS={h.GetRMS():.4g} mode={h.GetBinCenter(h.GetMaximumBin()):.4g}")

def crystal_ball(x, p):
    t = (x[0] - p[3]) / p[4]
    a, n = p[1], p[2]
    if t <= a:
        return p[0] * math.exp(-0.5 * t * t)
    A = math.pow(n / a, n) * math.exp(-0.5 * a * a)
    B = n / a - a
    return p[0] * A * math.pow(B + t, -n)

cb = ROOT.TF1("cb", crystal_ball, xmin, xmax, 5)
cb.SetParameters(h.GetMaximum(), 1.0, 3.0, h.GetBinCenter(h.GetMaximumBin()), max(h.GetRMS(), 1e-3))
cb.SetParLimits(0, 0, 1e12)
cb.SetParLimits(1, 0.01, 50)
cb.SetParLimits(2, 0.1, 50)
cb.SetParLimits(3, xmin, xmax)
cb.SetParLimits(4, 1e-4, 1e3)

r = h.Fit(cb, "RQS")
chi2 = cb.GetChisquare()
ndf = cb.GetNDF()
print(f"chi2/ndf = {chi2:.3g}/{ndf} = {chi2/ndf:.3g}  prob={r.Prob():.3g}")
for i in range(cb.GetNpar()):
    print(f"p[{i}] = {cb.GetParameter(i):.6g} +- {cb.GetParError(i):.6g}")

c1 = ROOT.TCanvas("c1", "c1", 900, 650)
h.SetTitle("e- |p| (momentummag);|p| [GeV/c];Entries")
h.SetMarkerStyle(20)
h.SetMarkerSize(0.5)
h.SetLineColor(ROOT.kBlue)
h.Draw("E1")
cb.SetLineColor(ROOT.kRed)
cb.Draw("same")
c1.SetLogy()
c1.SaveAs(OUTDIR + "/momentummag_fit.png")
c1.SaveAs(OUTDIR + "/momentummag_fit.root")

with open(OUTDIR + "/momentummag_fit_params.txt", "w") as fp:
    fp.write(" ".join(f"{cb.GetParameter(i):.9g}" for i in range(cb.GetNpar())) + "\n")

print("params =", " ".join(f"{cb.GetParameter(i):.6g}" for i in range(cb.GetNpar())))
