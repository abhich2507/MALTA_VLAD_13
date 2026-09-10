#!/usr/bin/env python3
"""
plot_bkg_momentum.py

3D position distribution of background particles (mcFlag == 1) overlaid with
momentum-direction arrows, read from the TruthVertex ntuple.

Requires uproot + matplotlib:
    python3 plotting_scripts/plot_bkg_momentum.py 13

Usage:
    plot_bkg_momentum.py [run] [--base DIR] [--nthreads N]
                         [--maxpts N] [--scale LEN] [--out FILE] [--show]

Example:
    plot_bkg_momentum.py 13 --maxpts 3000 --scale 80
"""

import argparse
import os

import numpy as np
import matplotlib
matplotlib.use("Agg")  # headless; use --show with an interactive backend if desired
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401  (registers 3d projection)

import uproot


def main():
    ap = argparse.ArgumentParser(description="Background position + momentum direction (3D)")
    ap.add_argument("run", type=int, nargs="?", default=13, help="run number")
    ap.add_argument("--base", default="./Results_10mev_e_mp_mc_coin_proton120GeV_custom_gen",
                    help="base directory containing local_XXXX folders")
    ap.add_argument("--nthreads", type=int, default=6, help="number of output thread files")
    ap.add_argument("--maxpts", type=int, default=3000,
                    help="max background vertices/arrows to draw (downsampled)")
    ap.add_argument("--scale", type=float, default=80.0,
                    help="arrow length in mm (momentum is normalised to direction)")
    ap.add_argument("--out", default=None, help="output image path (default: run dir)")
    ap.add_argument("--show", action="store_true", help="open an interactive window")
    args = ap.parse_args()

    path = f"{args.base}/local_{args.run:04d}"
    glob = f"{path}/output0_t*.root"
    if not any(os.path.isfile(f"{path}/output0_t{t}.root") for t in range(args.nthreads)):
        raise SystemExit(f"No output0_t*.root files found under {path}")

    cols = ["trueVertexX", "trueVertexY", "trueVertexZ",
            "trueMomX", "trueMomY", "trueMomZ"]
    files = [f"{path}/output0_t{t}.root:TruthVertex" for t in range(args.nthreads)]
    raw = uproot.concatenate(files, filter_name=cols + ["mcFlag"], library="np")

    mask = raw["mcFlag"] == 1
    arr = {c: raw[c][mask] for c in cols}
    n_bkg = len(arr["trueVertexX"])
    if n_bkg == 0:
        raise SystemExit(f"Run {args.run} has no background particles (mcFlag == 1). "
                         "Check particleCount in flags.cfg (must be > 1).")

    # Downsample for readability
    n_draw = min(n_bkg, args.maxpts)
    rng = np.random.default_rng(42)
    idx = rng.choice(n_bkg, size=n_draw, replace=False)

    x = arr["trueVertexX"][idx]
    y = arr["trueVertexY"][idx]
    z = arr["trueVertexZ"][idx]
    px = arr["trueMomX"][idx]
    py = arr["trueMomY"][idx]
    pz = arr["trueMomZ"][idx]

    print(f"Background vertices: {n_bkg}  (drawing {n_draw})")
    print(f"  pos  mean = ({x.mean():.1f}, {y.mean():.1f}, {z.mean():.1f}) mm")
    print(f"  mom  mean = ({px.mean():.4f}, {py.mean():.4f}, {pz.mean():.4f}) GeV")

    fig = plt.figure(figsize=(12, 10))
    ax = fig.add_subplot(111, projection="3d")

    ax.scatter(x, y, z, s=1.5, c="red", alpha=0.45, depthshade=True,
               label="background vertices")
    ax.quiver(x, y, z, px, py, pz,
              length=args.scale, normalize=True,
              color="blue", linewidth=0.7, arrow_length_ratio=0.2,
              label="momentum direction")

    ax.set_xlabel("x [mm]")
    ax.set_ylabel("y [mm]")
    ax.set_zlabel("z [mm]")
    ax.set_title(f"Background position + momentum direction (run {args.run}, "
                 f"{n_draw}/{n_bkg} shown)")
    ax.legend(loc="upper left")

    out = args.out or f"{path}/bkg_momentum3D.png"
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    fig.savefig(out, dpi=150)
    print(f"Saved: {out}")

    if args.show:
        plt.show()


if __name__ == "__main__":
    main()
