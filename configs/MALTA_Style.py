# <dir>/MALTAStyle.py
import ROOT

def MALTAStyle(canvas=None):
    ROOT.gROOT.SetStyle("ATLAS")

    g = ROOT.gStyle
    g.SetCanvasPreferGL(True)
    g.SetPadLeftMargin(0.17)
    g.SetPadRightMargin(0.05)
    g.SetPadTopMargin(0.10)
    g.SetPadBottomMargin(0.12)

    g.SetOptStat(0)
    g.SetOptFit(0)

    g.SetLegendBorderSize(0)
    g.SetLegendTextSize(0.04)

    g.SetMarkerStyle(20)
    g.SetMarkerSize(2.0)
    g.SetLineWidth(3)

    g.SetTitleOffset(1.2, "X")
    g.SetTitleOffset(1.7, "Y")
    g.SetLabelSize(0.04, "XYZ")
    g.SetTitleSize(0.05, "XYZ")

    g.SetTextFont(42)
    g.SetTextSize(0.04)
    g.SetHistLineWidth(3)

    if canvas is not None:
        canvas.SetLeftMargin(g.GetPadLeftMargin())
        canvas.SetRightMargin(g.GetPadRightMargin())
        canvas.SetTopMargin(g.GetPadTopMargin())
        canvas.SetBottomMargin(g.GetPadBottomMargin())