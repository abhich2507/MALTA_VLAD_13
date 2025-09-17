#!/usr/bin/env python
import ROOT

ROOT.gROOT.SetStyle("ATLAS")
ROOT.gStyle.SetPalette(112)
ROOT.gStyle.SetNumberContours(255)
ROOT.gROOT.SetBatch(1)

latVsub="V#lower[0.2]{#scale[0.6]{sub}}" # Vsub latex style
latEl= "e#lower[-2.2]{#scale[0.6]{- }}" #e- latex style

def scale_tgraph_y(graph, factor):
    """
    Rescale all y-values of a TGraph by a constant factor.

    Parameters
    ----------
    graph : ROOT.TGraph
        The TGraph object to be scaled.
    factor : float
        The scaling factor for y-values.
    """
    #if not isinstance(graph, ROOT.TGraph) or isinstance(graph, ROOT.TGraphErrors):
    #    raise TypeError("Input must be a ROOT.TGraph")

    for i in range(graph.GetN()):
        graph.GetY()[i] *= factor

    return 0


data_filename = "root_input/xybinsIDB100IBIAS05_Clsize_Xbin16_XNsteps1_Xstepsize0_Yfix-1.root"
sim_filename = "root_input/SimOutput_250917.root"
    
sim_infile = ROOT.TFile(sim_filename) # READ only
sim_AvEff = sim_infile.Get("AverageEff").Clone()
sim_AvEff.SetTitle("Simulation")

data_infile = ROOT.TFile(data_filename) # READ only
data_AvEff = data_infile.Get("MultiEff").Clone()
#data_AvEff.SetTitle("Data")

grList = data_AvEff.GetListOfGraphs()
grList[0].SetTitle("Data")
#print(len(grList))
#print(grList)
#data_AvEff.SetTitle("Data")


c=ROOT.TCanvas("cGr","Eff_1D",800,600)
mg_AvEff = ROOT.TMultiGraph()
mg_AvEff.SetTitle(";Threshold [{}];Efficiency [%]".format(latEl))
scale_tgraph_y(sim_AvEff, 100.) # plot in percent.
mg_AvEff.Add(sim_AvEff)
mg_AvEff.Add(grList[0])

"""
sim_AvEff.SetTitle("; Track position from centre [#mum] ; MPV [{}]".format(latEl)) 
sim_AvEff.SetMarkerSize(2.0)
sim_AvEff.SetMarkerStyle(43)
sim_AvEff.SetMarkerColor(ROOT.kOrange+2)
sim_AvEff.GetYaxis().SetTitleOffset(1.3)
"""
mg_AvEff.Draw("ap")
#data_AvEff.Draw("ap same")
#grList[0].Draw("ap same")
c.BuildLegend()
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
