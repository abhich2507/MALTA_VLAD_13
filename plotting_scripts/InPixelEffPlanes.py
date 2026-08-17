#!/usr/bin/env python3
# plotting_scripts/InPixelEffPlanes.py
# Plots in-pixel efficiency with binomial error bars for planeZ0 and planeZ1
# as a function of background count, reading Results/inPixel_eff.csv.
# Usage (from malta_simulation/):  python plotting_scripts/InPixelEffPlanes.py

import array
import ROOT

CSV = "Results/inPixel_eff.csv"

counts, eff0, err0, eff1, err1 = [], [], [], [], []
with open(CSV) as f:
    lines = f.read().strip().splitlines()

if len(lines) < 2:
    raise SystemExit("No data rows found in " + CSV)

for line in lines[1:]:
    if not line.strip():
        continue
    parts = [p.strip() for p in line.split(",")]
    run = int(parts[0])
    count = int(parts[1])
    e0, s0, e1, s1 = (float(x) for x in parts[2:6])
    # skip sentinel rows (missing histograms are written as -999)
    if e0 < -900.0 or e1 < -900.0:
        continue
    counts.append(count)
    eff0.append(e0)
    err0.append(s0)
    eff1.append(e1)
    err1.append(s1)

n = len(counts)
if n == 0:
    raise SystemExit("No valid data points after filtering sentinels.")

x = array.array("d", counts)
y0 = array.array("d", eff0)
e0 = array.array("d", err0)
y1 = array.array("d", eff1)
e1 = array.array("d", err1)
ex = array.array("d", [0.0] * n)

ROOT.gStyle.SetOptStat(0)
ROOT.gROOT.SetStyle("ATLAS")

gr0 = ROOT.TGraphErrors(n, x, y0, ex, e0)
gr0.SetTitle("")
#gr0.SetName("Plane0")
gr0.SetMarkerStyle(34)
gr0.SetMarkerSize(1.2)
gr0.SetMarkerColor(ROOT.kBlue + 2)
gr0.SetLineColor(ROOT.kBlue + 2)
gr0.SetLineWidth(2)

gr1 = ROOT.TGraphErrors(n, x, y1, ex, e1)
gr1.SetTitle("")
#gr1.SetName("")
gr1.SetMarkerStyle(21)
gr1.SetMarkerSize(1.2)
gr1.SetMarkerColor(ROOT.kRed + 1)
gr1.SetLineColor(ROOT.kRed + 1)
gr1.SetLineWidth(2)


c = ROOT.TCanvas("c", "in-pixel efficiency per plane", 800, 600)
c.SetLeftMargin(0.14)
c.SetBottomMargin(0.14)
c.SetGrid()

yMin = min(min(eff0), min(eff1))
yMax = max(max(eff0), max(eff1))
pad = 0.08 * (yMax - yMin) if yMax > yMin else 5.0

gr0.Draw("AP")
gr0.SetTitle("Hit Efficiency vs Background Count")
gr0.GetXaxis().SetTitle("Background(e-) count")
gr0.GetYaxis().SetTitle("Hit efficiency [%] (signal+BKG)")
gr0.GetXaxis().SetLimits(min(counts) - 2.0, max(counts) + 2.0)
gr0.GetYaxis().SetRangeUser(yMin - pad, yMax + pad)

gr1.Draw("P SAME")

leg = ROOT.TLegend(0.62, 0.58, 0.88, 0.76)
leg.SetBorderSize(1)
leg.SetFillStyle(0)
leg.AddEntry(gr0, "plane0", "lep")
leg.AddEntry(gr1, "plane1", "lep")
leg.Draw()

c.SaveAs("Results/inPixel_eff_z0_z1.png")
c.SaveAs("Results/inPixel_eff_z0_z1.pdf")
print("Saved Results/inPixel_eff_z0_z1.png and Results/inPixel_eff_z0_z1.pdf")
