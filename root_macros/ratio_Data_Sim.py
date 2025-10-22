import ROOT

ROOT.gStyle.SetPadTickX(1)
ROOT.gStyle.SetPadTickY(1)

# --- User settings ---
root_file_path = "SimOutput_40.root"
hist_from_root_name = "InPixelMatch_trackunc_1400Thr"

macro_file_path = "CompareDataSim_TH2D/W5R23IBIAS052DInPix_TOT_Eff_ITHR80_thr1397e_range0to100perc.C"  # The C macro defining the other TH2D
hist_from_macro_name = "TOT_Eff"         # Name of histogram defined in macro

output_pdf = "ratio_hist.pdf"

# --- Load histogram from ROOT file ---
f = ROOT.TFile.Open(root_file_path)
if not f or f.IsZombie():
    raise FileNotFoundError(f"Cannot open {root_file_path}")

h_root = f.Get(hist_from_root_name)
if not isinstance(h_root, ROOT.TH2D):
    raise TypeError(f"{hist_from_root_name} is not a TH2D in {root_file_path}")

# --- Load histogram from C macro ---
# This assumes the macro defines a TH2D with name hist_from_macro_name
ROOT.gROOT.LoadMacro(macro_file_path)
#h_macro = getattr(ROOT, hist_from_macro_name, None)

ROOT.W5R23IBIAS052DInPix_TOT_Eff_ITHR80_thr1397e_range0to100perc()  # <-- Run the function defined in macro
h_macro = ROOT.TOT_Eff  # Now it exists

if not isinstance(h_macro, ROOT.TH2D):
    raise TypeError(f"Could not find TH2D '{hist_from_macro_name}' in macro {macro_file_path}")

# --- Check binning compatibility ---
if (h_root.GetNbinsX() != h_macro.GetNbinsX()) or (h_root.GetNbinsY() != h_macro.GetNbinsY()):
    raise ValueError("Histograms do not have matching binning!")

# --- Create ratio histogram ---
ratio = h_root.Clone("ratio")
ratio.SetTitle("Ratio: ROOT file / Macro")
ratio.Reset()

# --- Compute bin-by-bin ratio ---
for ix in range(1, h_root.GetNbinsX() + 1):
    for iy in range(1, h_root.GetNbinsY() + 1):
        num = h_root.GetBinContent(ix, iy)
        den = h_macro.GetBinContent(ix, iy)
        val = num / den if den != 0 else 0
        ratio.SetBinContent(ix, iy, val)
        print(val)

# --- Draw and save ---
canvas = ROOT.TCanvas("c1", "Ratio", 800, 700)
ROOT.gStyle.SetOptStat(0)
ratio.Draw("COLZ")
ratio.SetMinimum(0.8)#
ratio.SetMaximum(-1111)# magic ROOT code = "auto-scale"
#ratio.GetZaxis().SetRangeUser(-5.,5.)
canvas.SaveAs(output_pdf)

# --- Close ---
f.Close()

print(f"✅ Ratio histogram saved as {output_pdf}")
