import ROOT
from array import array

# remove everyN point starting from Pointindex of "startat".
# stop when all graph points has been looped over or amount of removed points has reached "stopat".
def Remove_everyNpoints(gr, everyN=2, startat=1, stopat=100):
    N_removed = 0
    count = startat # point index of where to start
    while (count < gr.GetN() and N_removed<stopat):
        gr.RemovePoint(count) # point removal will decrease the index of each later point by one.
        count +=everyN -1  # skip more points if everyN is > 2
        N_removed +=1
    return 0

# remove all points that are below threshold".
# stop when all graph points has been looped over.
def Remove_belowThresh(gr, threshold=1000.):
    N_removed = 0
    count = 0 
    while (count < gr.GetN()):
        if gr.GetPointX(count)< threshold: # below threshold
            gr.RemovePoint(count) # point removal will decrease the index of each later point by one.  
            N_removed +=1
        else:
            count += 1 # go to next point
    return 0

# add relative error to all X or Y values of a graph
# axis = "X" or "Y" to set X or Y Error
# if addConst >0: this is constant uncert. that is added in quadrature to relative one.
# typically for thresholds, this is at least 10e-
def addRelativeErrors(gr, fraction, axisError = "X", addConst=0.):
    for i in range(gr.GetN()):
        xval = gr.GetPointX(i)
        yval = gr.GetPointY(i)
        if axisError=="X":
            rel_xerr = xval * fraction
            xerr = (rel_xerr**2 + addConst**2)**0.5 if addConst > 0. else rel_xerr
            gr.SetPointError(i,xerr, gr.GetErrorY(i))
        else:
            rel_yerr = yval * fraction
            yerr = (rel_yerr**2 + addConst**2)**0.5 if addConst > 0. else rel_yerr
            gr.SetPointError(i,gr.GetErrorX(i), yerr)
    return 0

# add content of gr2 to gr
def addGrtoGr(gr, gr2):
    N0 = gr.GetN()
    for i in range(gr2.GetN()):
        xval = gr2.GetPointX(i)
        yval = gr2.GetPointY(i)
        xErr = gr2.GetErrorX(i)
        yErr = gr2.GetErrorY(i)
        gr.SetPoint(N0+i,xval, yval)
        gr.SetPointError(N0+i,xErr, yErr)
    return 0

# estimate statistical uncertainty for a given uncertainty and a total sample size of N_TOT
# eff in percent.
## uncertainty result in percent.
def estimate_EffStatUnc(eff, N_TOT):
    alpha    = 0.3173   # 68.27% CL (1 sigma)

    N_det = int(eff/100.*N_TOT) # estimate number of measured events back.
    
    low_unc = ROOT.TEfficiency.ClopperPearson(N_TOT, N_det, alpha, False)*100  # lower bound
    up_unc  = ROOT.TEfficiency.ClopperPearson(N_TOT, N_det, alpha, True)*100   # upper bound
    print(N_det, " ", N_TOT, " ",low_unc, " ", up_unc )
    return abs(eff-low_unc), up_unc-eff

def make_band_from_tgrapherrors(gr, color=ROOT.kBlue, alpha=0.3, estimEffUnc = False):
    """
    Build a TGraphAsymmErrors (shaded band) from a TGraphErrors.
    """
    n = gr.GetN()
    band = ROOT.TGraphAsymmErrors(n)

    for i in range(n):
        x = gr.GetPointX(i)
        y = gr.GetPointY(i)
        ex = gr.GetErrorX(i)
        ey1 = gr.GetErrorY(i)
        ey2 = gr.GetErrorY(i)
        if estimEffUnc == True:
            ey1, ey2 = estimate_EffStatUnc(y, 100) # for 10^6 events

        band.SetPoint(i, x, y)
        band.SetPointError(i, ex, ex, ey1, ey2)

    band.SetFillColorAlpha(color, alpha)
    band.SetLineColor(0)

    return band

def make_xband_from_tgrapherrors(gr, color=ROOT.kBlue, alpha=0.2):
    """
    Build a TGraph polygon representing the X-uncertainty band of a TGraphErrors.
    """
    n = gr.GetN()
    x_upper, y_upper = [], []
    x_lower, y_lower = [], []

    # Go forward: upper edge (x + ex)
    for i in range(n):
        x = gr.GetPointX(i)
        y = gr.GetPointY(i)
        ex = gr.GetErrorX(i)
        x_upper.append(float(x) + ex)
        y_upper.append(float(y))

    # Go backward: lower edge (x - ex)
    for i in reversed(range(n)):
        x = gr.GetPointX(i)
        y = gr.GetPointY(i)
        ex = gr.GetErrorX(i)
        x_lower.append(float(x) - ex)
        y_lower.append(float(y))

    # Combine into polygon
    x_poly = array('d', x_upper + x_lower)
    y_poly = array('d', y_upper + y_lower)

    band = ROOT.TGraph(len(x_poly), x_poly, y_poly)
    band.SetFillColorAlpha(color, alpha)
    band.SetLineColor(0)

    return band
