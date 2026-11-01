#!/usr/bin/env python3
"""
plot_eff_vs_energy.py
======================

Scatter plot of efficiency (Y) vs signal energy (X), gathered from the
Results_* directories.

For every Results_* folder it:
  * parses the signal energy from the folder name (Results_pi2.5GeV -> 2.5 GeV)
  * reads the coincidence efficiency from coin_eff.csv (the run with RATE)
  * reads the per-plane signal-hit efficiency from bkg_eff.csv (RATE row)

The background rate used to pick the data rows is RATE, matched to a run
number through configs/bkg_rate.csv (run -> rate).

Run from malta_simulation/ with the Python that matches the ROOT build:
    python3.14 plotting_scripts/plot_eff_vs_energy.py
"""

import os
import re
import glob

import ROOT

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))   # malta_simulation/
RATE = 1.584          # background rate (particles/event) used to select rows
OUT = os.path.join(BASE, "eff_vs_energy")


def parse_energy(name):
    """'Results_<particle><energy>GeV' -> (particle, energy); None if it doesn't match."""
    m = re.match(r"Results_(.*?)(\d+(?:\.\d+)?)GeV$", name)
    return (m.group(1), float(m.group(2))) if m else (None, None)


def run_for_rate():
    """Return the run number whose rate in configs/bkg_rate.csv equals RATE."""
    csv = os.path.join(BASE, "configs", "bkg_rate.csv")
    if not os.path.exists(csv):
        return None
    with open(csv) as f:
        next(f, None)  # header
        for line in f:
            line = line.strip()
            if not line:
                continue
            run, rate = line.split(",")
            if abs(float(rate) - RATE) < 1e-9:
                return int(run)
    return None


def read_coin_eff(csv, target_run):
    """coin_eff.csv -> (eff_percent, binomial error) for the given run."""
    if target_run is None or not os.path.exists(csv):
        return None
    with open(csv) as f:
        hdr = f.readline().strip().split(",")
        i_run, i_n = hdr.index("run"), hdr.index("nGen")
        i_cnt, i_eff = hdr.index("coinCount"), hdr.index("eff_percent")
        for line in f:
            v = line.strip().split(",")
            if len(v) < 5:
                continue
            if int(v[i_run]) == target_run:
                eff = float(v[i_eff])
                n = float(v[i_n])
                err = (eff * (100.0 - eff) / n) ** 0.5
                return eff, err
    return None


def read_bkg_eff(csv, rate):
    """bkg_eff.csv -> {plane: (avgEff, effError)} for rows with bkgRateMean == rate."""
    out = {}
    if not os.path.exists(csv):
        return out
    with open(csv) as f:
        hdr = f.readline().strip().split(",")
        ip, ir, ie, ier = hdr.index("plane"), hdr.index("bkgRateMean"), \
                          hdr.index("avgEff"), hdr.index("effError")
        for line in f:
            v = line.strip().split(",")
            if len(v) < 4:
                continue
            if abs(float(v[ir]) - rate) < 1e-9:
                out[int(v[ip])] = (float(v[ie]), float(v[ier]))
    return out


# ---- gather data -----------------------------------------------------------
target_run = run_for_rate()
coin, p0, p1 = [], [], []

for d in sorted(glob.glob(os.path.join(BASE, "Results_*"))):
    name = os.path.basename(d)
    particle, energy = parse_energy(name)
    if energy is None:
        print(f"skip {name!r} (cannot parse energy from name)")
        continue
    if not particle.startswith("pi") or energy < 1.0 or energy > 5.0:
        print(f"skip {name!r} (outside pi 1-5 GeV scan)")
        continue

    c = read_coin_eff(os.path.join(d, "coin_eff.csv"), target_run)
    b = read_bkg_eff(os.path.join(d, "bkg_eff.csv"), RATE)

    print(f"{name:22s}  E={energy:6.2f} GeV  coin={c}  bkg0={b.get(0)}  bkg1={b.get(1)}")

    if c is not None:
        coin.append((energy, c[0], c[1]))
    if 0 in b:
        p0.append((energy, b[0][0], b[0][1]))
    if 1 in b:
        p1.append((energy, b[1][0], b[1][1]))

coin.sort()
p0.sort()
p1.sort()

if not (coin or p0 or p1):
    raise SystemExit("no data found in Results_* directories")


def errors_graph(data, color, style, title):
    g = ROOT.TGraphErrors(len(data))
    for i, (e, ef, er) in enumerate(data):
        g.SetPoint(i, e, ef)
        g.SetPointError(i, 0.0, er)
    g.SetMarkerStyle(style)
    g.SetMarkerColor(color)
    g.SetLineColor(color)
    g.SetMarkerSize(1.4)
    g.SetTitle(title)
    return g


g_coin = errors_graph(coin, ROOT.kBlue + 2, 20,
                      ";Signal energy [GeV];Efficiency [%]")
g_p0 = errors_graph(p0, ROOT.kRed, 21, "")
g_p1 = errors_graph(p1, ROOT.kGreen + 2, 22, "")

# ---- draw ------------------------------------------------------------------
c = ROOT.TCanvas("c", "c", 900, 700)
c.SetLeftMargin(0.12)
c.SetRightMargin(0.04)
c.SetBottomMargin(0.12)
c.SetTopMargin(0.06)

# Explicit axis frame so titles and tick labels are always drawn.
frame = ROOT.TH1D("frame", ";Signal energy [GeV];Efficiency [%]", 1, 0.9, 5.5)
Axes = frame.GetXaxis()
Axes.SetMaxDigits(2)   # avoid scientific notation on X axis
frame.SetMinimum(90.0)    # zoom to the interesting efficiency range
frame.SetMaximum(100.0)
frame.GetXaxis().SetTitleSize(0.045)
frame.GetXaxis().SetLabelSize(0.045)
frame.GetXaxis().SetNdivisions(8, False)   # 0.5 GeV ticks over 1-5 GeV
frame.GetYaxis().SetTitleSize(0.045)
frame.GetYaxis().SetLabelSize(0.045)
frame.GetYaxis().SetNdivisions(3, False)   # 90, 95, 100, 105
frame.Draw("AXIS")        # draw only the axes

g_p0.Draw("P")
g_p1.Draw("P")
g_coin.Draw("P")

# uncomment for a logarithmic energy axis (handy when 120 GeV is present):
# c.SetLogx()

leg = ROOT.TLegend(0.15, 0.15, 0.55, 0.38)
leg.SetBorderSize(0)
leg.AddEntry(g_coin, "Coincidence efficiency", "p")
leg.AddEntry(g_p0, f"Signal-hit eff., plane 0  (#lambda = {RATE:g})", "p")
leg.AddEntry(g_p1, f"Signal-hit eff., plane 1  (#lambda = {RATE:g})", "p")
leg.Draw()

c.SaveAs(OUT + ".png")
c.SaveAs(OUT + ".pdf")
print("saved", OUT + ".png / .pdf")
