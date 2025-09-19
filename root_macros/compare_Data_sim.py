#!/usr/bin/env python
import ROOT
import ROOTHelperFunctions as RHF

ROOT.gROOT.SetStyle("ATLAS")
ROOT.gStyle.SetPalette(112)
ROOT.gStyle.SetNumberContours(255)
ROOT.gROOT.SetBatch(1)

latVsub="V#lower[0.2]{#scale[0.6]{sub}}" # Vsub latex style
latEl= "e#lower[-2.2]{#scale[0.6]{- }}" #e- latex style


data_filename = "root_input/xybinsIDB100IBIAS05_Clsize_Xbin16_XNsteps1_Xstepsize0_Yfix-1.root"
sim_filename = "root_input/SimOutput_250917.root"
    
sim_infile = ROOT.TFile(sim_filename) # READ only
sim_AvEff = sim_infile.Get("AverageEff").Clone()
sim_AvEff.SetTitle("Simulation")

data_infile = ROOT.TFile(data_filename) # READ only
data_AvEff = data_infile.Get("MultiEff").Clone()
#data_AvEff.SetTitle("Data")

grList = data_AvEff.GetListOfGraphs()
RHF.Remove_everyNpoints(grList[0], 2, 1) # remove every 2nd point
RHF.Remove_everyNpoints(grList[0], 2, 1) # remove again every 2nd point
grList[0].GetListOfFunctions().Clear() # clear fit function
RHF.addRelativeErrors(grList[0], 0.03,"X") # 3% uncertainty on Threshold-values
grList[0].SetTitle("Data")
#grList[0].SetFillColorAlpha(38, 0.5)
#print(len(grList))
#print(grList)
#data_AvEff.SetTitle("Data")


c=ROOT.TCanvas("cGr","Eff_1D",800,600)
mg_AvEff = ROOT.TMultiGraph()
mg_AvEff.SetTitle(";Threshold [{}];Efficiency [%]".format(latEl))
mg_AvEff.Add(grList[0])
mg_AvEff.Add(sim_AvEff)

"""
sim_AvEff.SetTitle("; Track position from centre [#mum] ; MPV [{}]".format(latEl)) 
sim_AvEff.SetMarkerSize(2.0)
sim_AvEff.SetMarkerStyle(43)
sim_AvEff.SetMarkerColor(ROOT.kOrange+2)
sim_AvEff.GetYaxis().SetTitleOffset(1.3)
"""
mg_AvEff.Draw("ap") # "ap"
#data_AvEff.Draw("ap same")
#grList[0].Draw("ap same")
leg = c.BuildLegend(0.2, 0.15, 0.5, 0.4)
leg.SetFillStyle(0)
leg.SetBorderSize(0)

t = ROOT.TLatex() # this should come after BuildLegend.
t.SetTextSize(0.04)
t.DrawLatexNDC(0.8,0.87, "#splitline{#bf{MALTA2}}{30#mum EPI}")
c.Print("Comparison_Data_Sim_AvEff.pdf")
c.Close()

c2=ROOT.TCanvas("c2Gr","ClSize_1D",800,600)
sim_AvClSize = sim_infile.Get("AverageClSize")

data_AvClSize = data_infile.Get("MultiClSize").Clone()
#data_AvEff.SetTitle("Data")

grList_AvClSize = data_AvClSize.GetListOfGraphs()
grList_AvClSize[0].SetTitle("Data ClSize")

mg_AvClSize = ROOT.TMultiGraph()
mg_AvClSize.SetTitle(";Threshold [{}];<cluster size>".format(latEl))
mg_AvClSize.Add(sim_AvClSize)
mg_AvClSize.Add(grList_AvClSize[0])
mg_AvClSize.Draw("ap")
c2.Print("Comparison_Data_Sim_AvClSize.pdf")
c2.Close()
