import ROOT, math, os

PATH = "/Users/snip/Documents/MALTA_VLAD_13/malta_simulation/Data/New_Data/electron_sensor_dist.root"
OUT  = "/Users/snip/Documents/MALTA_VLAD_13/malta_simulation/Data/New_Data/electron_sensor_mag.root"

f = ROOT.TFile(PATH, "READ")
if f.IsZombie():
    raise SystemExit(f"Cannot open {PATH}")

# ------------------------------------------------------------------
# 1) Recursively list every object (name + class) so you can verify
# ------------------------------------------------------------------
def list_keys(obj, prefix=""):
    keys = obj.GetListOfKeys()
    out = []
    for i in range(keys.GetSize()):
        k = keys.At(i)
        name, cls = k.GetName(), k.GetClassName()
        full = prefix + name
        out.append((full, cls))
        if "TDirectory" in cls:
            out += list_keys(obj.Get(name), full + "/")
    return out

all_keys = list_keys(f)
maxsens = [n for n, c in all_keys if "maxsensor" in n]
print("=== maxsensor objects ===")
for n in maxsens:
    print("  ", n)

# ------------------------------------------------------------------
# 2) Grab the three momentum histograms for maxsensor
# ------------------------------------------------------------------
def find(name_part):
    for n, c in all_keys:
        if name_part in n and ("momentumx" in n or "momentumy" in n or "momentumz" in n) == (name_part in ("momentumx","momentumy","momentumz")):
            pass
    # simpler: exact suffix match
    for n, c in all_keys:
        if n.endswith("maxsensor." + name_part):
            return f.Get(n)
    raise SystemExit(f"missing maxsensor.{name_part}")

hx = find("momentumx")
hy = find("momentumy")
hz = find("momentumz")

print("\n=== momentum histograms ===")
print(f"x: {hx.GetName()}  bins={hx.GetNbinsX()}  range=({hx.GetXaxis().GetXmin()},{hx.GetXaxis().GetXmax()})")
print(f"y: {hy.GetName()}  bins={hy.GetNbinsX()}")
print(f"z: {hz.GetName()}  bins={hz.GetNbinsX()}")

# "max sensor momentum" per axis = highest bin centre with content
def max_bin(h):
    b = h.GetMaximumBin()
    return h.GetBinCenter(b), h.GetBinContent(b)

print(f"max momentum x = {max_bin(hx)[0]:.4g}  (entries {max_bin(hx)[1]:.0f})")
print(f"max momentum y = {max_bin(hy)[0]:.4g}  (entries {max_bin(hy)[1]:.0f})")
print(f"max momentum z = {max_bin(hz)[0]:.4g}  (entries {max_bin(hz)[1]:.0f})")

# ------------------------------------------------------------------
# 3) Final momentum MAGNITUDE histogram:  |p| = sqrt(px^2+py^2+pz^2)
#    The file stores only the three 1-D marginals, so we sample each
#    independently -- exactly like PrimaryGenerator::SampleCDF does.
# ------------------------------------------------------------------
N = 1_000_000
hx.SetDirectory(0); hy.SetDirectory(0); hz.SetDirectory(0)

xmax = max(hx.GetXaxis().GetXmax(), hy.GetXaxis().GetXmax(), hz.GetXaxis().GetXmax())
hmag = ROOT.TH1D("hmag", "e- |p| at max sensor;|p| [GeV/c];Entries", 200, 0.0, xmax)

rng = ROOT.TRandom3(12345)          # seed for reproducibility
for _ in range(N):
    px = hx.GetRandom(rng)          # pass rng explicitly -> deterministic
    py = hy.GetRandom(rng)
    pz = hz.GetRandom(rng)
    hmag.Fill(math.sqrt(px*px + py*py + pz*pz))

print(f"\n=== magnitude |p| ===")
print(f"mean = {hmag.GetMean():.4g} GeV/c   max = {hmag.GetMaximum():.4g} GeV/c")

# ------------------------------------------------------------------
# 4) Save + quick plot
# ------------------------------------------------------------------
outf = ROOT.TFile(OUT, "RECREATE")
hmag.Write()
hx.Write("maxsensor_momentumx")
hy.Write("maxsensor_momentumy")
hz.Write("maxsensor_momentumz")
outf.Close()
print(f"saved -> {OUT}")

c = ROOT.TCanvas("c", "c", 900, 600)
hmag.Draw()
c.SaveAs("/Users/snip/Documents/MALTA_VLAD_13/malta_simulation/Data/New_Data/electron_sensor_mag.png")
print("saved plot -> electron_sensor_mag.png")

# ------------------------------------------------------------------
# 5) (Optional) dump CSVs in the same 'x weight' format as momentumCSV/
# ------------------------------------------------------------------
def dump_csv(h, path):
    with open(path, "w") as fh:
        for b in range(1, h.GetNbinsX() + 1):
            fh.write(f"{h.GetBinCenter(b)} {h.GetBinContent(b)}\n")

CSVDIR = "/Users/snip/Documents/MALTA_VLAD_13/malta_simulation/Data/momentumCSV"
os.makedirs(CSVDIR, exist_ok=True)
dump_csv(hx, CSVDIR + "/momentumx.csv")
dump_csv(hy, CSVDIR + "/momentumy.csv")
dump_csv(hz, CSVDIR + "/momentumz.csv")
print("wrote CSVs ->", CSVDIR)