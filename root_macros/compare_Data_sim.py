#!/usr/bin/env python
import ROOT
import ROOTHelperFunctions as RHF

ROOT.gROOT.SetStyle("ATLAS")
ROOT.gStyle.SetPalette(112)
ROOT.gStyle.SetNumberContours(255)
ROOT.gROOT.SetBatch(1)

latVsub="V#lower[0.2]{#scale[0.6]{sub}}" # Vsub latex style
latEl= "e#lower[-2.2]{#scale[0.6]{- }}" #e- latex style

data_filename_HighThr = "root_input/xybinsIDB100IBIAS05_Clsize_Xbin16_XNsteps1_Xstepsize0_Yfix-1.root"
data_filename_LowThr = "root_input/xybinsIDB120IBIAS43_Clsize_Xbin16_XNsteps1_Xstepsize0_Yfix-1.root"

## wrong sigma_erf definition and z = 30um:
sim_inputs = [
    {"filename": "SimOutput_26.root", "label": "w/o charge sharing", "color": ROOT.kRed, "linestyle": 3, "linewidth": 1, "alpha": 0.5, "markerstyle": 25, "markersize": 1.0},
    #{"filename": "SimOutput_23.root", "label": "1.0", "color": ROOT.kBlack, "linestyle": 2, "linewidth": 1, "alpha": 0.5, "markerstyle": 25, "markersize": 1.0},
    {"filename": "SimOutput_22.root", "label": "#sigma_{erf}=2.0#mum", "color": ROOT.kBlack, "linestyle": 3, "linewidth": 1, "alpha": 0.5, "markerstyle": 25, "markersize": 1.0},
    {"filename": "SimOutput_21.root", "label": "#sigma_{erf}=3.0#mum", "color": ROOT.kBlack, "linestyle": 1, "linewidth": 1, "alpha": 1.0, "markerstyle": 21, "markersize": 1.2},
    #{"filename": "SimOutput_24.root", "label": "4.0", "color": ROOT.kBlack, "linestyle": 2, "linewidth": 1, "alpha": 0.5, "markerstyle": 25, "markersize": 1.0},
    {"filename": "SimOutput_20.root", "label": "#sigma_{erf}=4.3#mum", "color": ROOT.kRed, "linestyle": 1, "linewidth": 1, "alpha": 1.0, "markerstyle": 21, "markersize": 1.2},
    #{"filename": "SimOutput_25.root", "label": "5.0", "color": ROOT.kBlack, "linestyle": 2, "linewidth": 1, "alpha": 0.5, "markerstyle": 21, "markersize": 1.0},
    {"filename": "SimOutput_27.root", "label": "#sigma_{erf}=6.0#mum", "color": ROOT.kBlack, "linestyle": 2, "linewidth": 1, "alpha": 0.5, "markerstyle": 21, "markersize": 1.0},
    {"filename": "SimOutput_28.root", "label": "#sigma_{erf}=10.0#mum", "color": ROOT.kRed, "linestyle": 2, "linewidth": 1, "alpha": 0.5, "markerstyle": 21, "markersize": 1.0},
]

## Corrected sigma definition: Sim with sigma*sqrt(2) --> sigma 
# and z = 30 um
sim_inputs = [
    {"filename": "SimOutput_30.root", "label": "w/o charge sharing", "color": ROOT.kRed, "linestyle": 3, "linewidth": 1, "alpha": 0.5, "markerstyle": 25, "markersize": 1.0},
    {"filename": "SimOutput_31.root", "label": "#sigma_{erf}=2.0#mum", "color": ROOT.kBlack, "linestyle": 3, "linewidth": 1, "alpha": 0.5, "markerstyle": 25, "markersize": 1.0},
    {"filename": "SimOutput_32.root", "label": "#sigma_{erf}=4.3#mum", "color": ROOT.kRed, "linestyle": 1, "linewidth": 1, "alpha": 1.0, "markerstyle": 21, "markersize": 1.2},
    {"filename": "SimOutput_33.root", "label": "#sigma_{erf}=6.0#mum", "color": ROOT.kBlack, "linestyle": 2, "linewidth": 1, "alpha": 0.5, "markerstyle": 21, "markersize": 1.0},
    {"filename": "SimOutput_34.root", "label": "#sigma_{erf}=10.0#mum", "color": ROOT.kRed, "linestyle": 2, "linewidth": 1, "alpha": 0.5, "markerstyle": 21, "markersize": 1.0},
]

## Corrected sigma definition: Sim with sigma*sqrt(2) --> sigma 
# and z = 29.1 um
sim_inputs = [
    {"filename": "SimOutput_35.root", "label": "w/o charge sharing", "color": ROOT.kRed, "linestyle": 3, "linewidth": 1, "alpha": 0.5, "markerstyle": 25, "markersize": 1.0},
    #{"filename": "SimOutput_36.root", "label": "#sigma_{erf}=  2.0#mum", "color": ROOT.kBlack, "linestyle": 3, "linewidth": 1, "alpha": 0.5, "markerstyle": 25, "markersize": 1.0},
    {"filename": "SimOutput_37.root", "label": "#sigma_{erf}=  4.3#mum", "color": ROOT.kRed, "linestyle": 1, "linewidth": 1, "alpha": 1.0, "markerstyle": 21, "markersize": 1.2},
    #{"filename": "SimOutput_38.root", "label": "#sigma_{erf}=  6.0#mum", "color": ROOT.kBlack, "linestyle": 2, "linewidth": 1, "alpha": 0.5, "markerstyle": 21, "markersize": 1.0},
    {"filename": "SimOutput_39.root", "label": "#sigma_{erf}=10.0#mum", "color": ROOT.kRed, "linestyle": 2, "linewidth": 1, "alpha": 0.5, "markerstyle": 21, "markersize": 1.0},
]

## In Sim with sigma*sqrt(2) --> sigma 
# and sigma_erf = 4.3
# z variation:
sim_inputs_Zvar = [
    {"filename": "SimOutput_42.root", "label": "z=27.0#mum", "color": ROOT.kRed, "linestyle": 3, "linewidth": 1, "alpha": 1.0, "markerstyle": 25, "markersize": 1.0},
    {"filename": "SimOutput_41.root", "label": "z=28.0#mum", "color": ROOT.kBlack, "linestyle": 3, "linewidth": 1, "alpha": 1.0, "markerstyle": 25, "markersize": 1.0},
    {"filename": "SimOutput_40.root", "label": "z=29.1#mum", "color": ROOT.kRed, "linestyle": 1, "linewidth": 1, "alpha": 1.0, "markerstyle": 21, "markersize": 1.2},
    {"filename": "SimOutput_43.root", "label": "z=30.0#mum", "color": ROOT.kBlack, "linestyle": 2, "linewidth": 1, "alpha": 1.0, "markerstyle": 21, "markersize": 1.0},
    {"filename": "SimOutput_44.root", "label": "z=31.0#mum", "color": ROOT.kRed, "linestyle": 2, "linewidth": 1, "alpha": 1.0, "markerstyle": 21, "markersize": 1.0},
    {"filename": "SimOutput_45.root", "label": "z=32.0#mum", "color": ROOT.kGreen, "linestyle": 2, "linewidth": 1, "alpha": 1.0, "markerstyle": 21, "markersize": 1.0},
]


data_infile_HT = ROOT.TFile(data_filename_HighThr) # READ only
data_infile_LT = ROOT.TFile(data_filename_LowThr) # READ only
data_AvEff_HT = data_infile_HT.Get("MultiEff").Clone()
grList_AvEff_HT = data_AvEff_HT.GetListOfGraphs()

## data from High-threshold setting

RHF.Remove_belowThresh(grList_AvEff_HT[0], 850.)
RHF.Remove_everyNpoints(grList_AvEff_HT[0], 2, 1) # remove every 2nd point
RHF.Remove_everyNpoints(grList_AvEff_HT[0], 2, 1) # remove again every 2nd point
RHF.Remove_everyNpoints(grList_AvEff_HT[0], 2, 1) # remove again every 2nd point
grList_AvEff_HT[0].GetListOfFunctions().Clear() # clear fit function

## data from Low-Threshold setting
data_AvEff_LT = data_infile_LT.Get("MultiEff").Clone()
## add low threshold points to data:
grList = data_AvEff_LT.GetListOfGraphs()
RHF.addGrtoGr(grList[0], grList_AvEff_HT[0])
RHF.addRelativeErrors(grList[0], 0.03,"X") # 3% uncertainty on Threshold-values

#for p in range(grList[0].GetN()):
#    print(grList[0].GetPointY(p), " +-", grList[0].GetErrorY(p))
#RHF.addRelativeErrors(grList[0], 0.01,"Y") # 3% uncertainty on y vals
grList[0].SetTitle("#bf{Data}")
grList[0].SetMarkerColor(ROOT.kBlue)
#print(grList[0].GetMarkerSize())
grList[0].SetMarkerSize(2.4)
grList[0].SetLineColor(ROOT.kBlue)
grList[0].SetLineWidth(2)
#grList[0].SetFillColorAlpha(38, 0.5)
#print(len(grList))
#print(grList)

# Create error band
#yerror_band = RHF.make_band_from_tgrapherrors(grList[0], ROOT.kBlue, 0.3, True)

# Make X-uncertainty band
xerror_band = RHF.make_xband_from_tgrapherrors(grList[0])

c=ROOT.TCanvas("cGr","Eff_1D",800,600)
mg_AvEff = ROOT.TMultiGraph()
mg_AvEff.SetTitle(";Threshold [{}];Efficiency [%]".format(latEl))
#mg_AvEff.Add(yerror_band, "3")
mg_AvEff.Add(grList[0], "P")

for inp in sim_inputs:
    sim_infile = ROOT.TFile(inp["filename"]) # READ only
    sim_AvEff = sim_infile.Get("AverageEff").Clone()
    sim_AvEff.SetTitle("#bf{Sim. "+inp["label"]+"}")
    sim_AvEff.SetLineColorAlpha(inp["color"], inp["alpha"])
    sim_AvEff.SetMarkerColorAlpha(inp["color"], inp["alpha"])
    sim_AvEff.SetMarkerSize(inp["markersize"])
    sim_AvEff.SetMarkerStyle(inp["markerstyle"])
    sim_AvEff.SetLineStyle(inp["linestyle"])
    #sim_AvEff.SetLineWidth(inp["linewidth"])
    #sim_AvEff.SetLineWidth(2)
    mg_AvEff.Add(sim_AvEff, "PL")


"""
sim_AvEff.SetTitle("; Track position from centre [#mum] ; MPV [{}]".format(latEl)) 
sim_AvEff.SetMarkerSize(2.0)
sim_AvEff.SetMarkerStyle(43)
sim_AvEff.SetMarkerColor(ROOT.kOrange+2)
sim_AvEff.GetYaxis().SetTitleOffset(1.3)
"""
mg_AvEff.Draw("A") # "ap"
mg_AvEff.GetXaxis().SetRangeUser(0,2500)
mg_AvEff.SetMinimum(0)
leg = c.BuildLegend(0.18, 0.18, 0.6, 0.52) #0.18, 0.18, 0.58, 0.5)
mg_AvEff.Add(xerror_band, "F")
leg.SetFillStyle(0)
leg.SetBorderSize(0)

t = ROOT.TLatex() # this should come after BuildLegend.
t.SetTextSize(0.04)
t.DrawLatexNDC(0.8,0.87, "#splitline{#bf{MALTA2}}{30#mum EPI}")
c.Print("Comparison_Data_Sim_AvEff.pdf")
c.Print("Comparison_Data_Sim_AvEff.C")
c.Close()

c2=ROOT.TCanvas("c2Gr","ClSize_1D",800,600)
#sim_AvClSize = sim_infile.Get("AverageClSize")

## data from High-threshold setting
data_AvClSize_HT = data_infile_HT.Get("MultiClSize").Clone()
grList_AvClSize_HT = data_AvClSize_HT.GetListOfGraphs()
RHF.Remove_belowThresh(grList_AvClSize_HT[0], 860.)
RHF.Remove_everyNpoints(grList_AvClSize_HT[0], 2, 1) # remove every 2nd point
RHF.Remove_everyNpoints(grList_AvClSize_HT[0], 2, 1) # remove again every 2nd point
RHF.Remove_everyNpoints(grList_AvClSize_HT[0], 2, 1) # remove again every 2nd point

## data from Low-Threshold setting
data_infile_LT = ROOT.TFile(data_filename_LowThr) # READ only
data_AvClSize_LT = data_infile_LT.Get("MultiClSize").Clone()
#data_AvEff.SetTitle("Data")

## add low threshold points to data:
grList_AvClSize = data_AvClSize_LT.GetListOfGraphs()
RHF.addGrtoGr(grList_AvClSize[0], grList_AvClSize_HT[0])
RHF.addRelativeErrors(grList_AvClSize[0], 0.03,"X") # 3% uncertainty on Threshold-values
grList_AvClSize[0].SetTitle("#bf{Data}")
grList_AvClSize[0].SetMarkerColor(ROOT.kBlue)
grList_AvClSize[0].SetMarkerSize(2.4)
grList_AvClSize[0].SetLineColor(ROOT.kBlue)
grList_AvClSize[0].SetLineWidth(2)

# Make X-uncertainty band
xerror_band_clsize = RHF.make_xband_from_tgrapherrors(grList_AvClSize[0])

mg_AvClSize = ROOT.TMultiGraph()
mg_AvClSize.SetTitle(";Threshold [{}];<cluster size>".format(latEl))

for inp in sim_inputs:
    sim_infile = ROOT.TFile(inp["filename"]) # READ only
    sim_AvClSize = sim_infile.Get("AverageClSize").Clone()
    sim_AvClSize.SetTitle("#bf{Sim. "+inp["label"]+"}")
    sim_AvClSize.SetLineColorAlpha(inp["color"], inp["alpha"])
    sim_AvClSize.SetMarkerColorAlpha(inp["color"], inp["alpha"])
    sim_AvClSize.SetMarkerSize(inp["markersize"])
    sim_AvClSize.SetMarkerStyle(inp["markerstyle"])
    sim_AvClSize.SetLineStyle(inp["linestyle"])
    mg_AvClSize.Add(sim_AvClSize, "PL")

mg_AvClSize.Add(grList_AvClSize[0],"PL")
mg_AvClSize.Draw("a")
legCl = c2.BuildLegend(0.5, 0.53, 0.92, 0.92)
mg_AvClSize.Add(xerror_band_clsize, "F")
mg_AvClSize.GetXaxis().SetRangeUser(0,2500)
#mg_AvClSize.SetMinimum(0)
legCl.SetFillStyle(0)
legCl.SetBorderSize(0)

t = ROOT.TLatex() # this should come after BuildLegend.
t.SetTextSize(0.04)
t.DrawLatexNDC(0.8,0.4, "#splitline{#bf{MALTA2}}{30#mum EPI}")

c2.Print("Comparison_Data_Sim_AvClSize.pdf")
c2.Print("Comparison_Data_Sim_AvClSize.C")
c2.Close()
