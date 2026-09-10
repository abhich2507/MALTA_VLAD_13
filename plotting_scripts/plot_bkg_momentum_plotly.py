#!/usr/bin/env python3
"""
plot_bkg_momentum_plotly.py

Interactive 3D plot of background particle positions (mcFlag == 1) with
momentum-direction cones (arrows), using plotly.graph_objects.Cone.
Saves a rotatable/zoomable HTML file that opens in a browser.

Requires uproot + plotly:
    python3 plotting_scripts/plot_bkg_momentum_plotly.py 13

Usage:
    plot_bkg_momentum_plotly.py [run] [--base DIR] [--nthreads N]
                                [--maxpts N] [--scale LEN] [--out FILE] [--show]
"""

import argparse
import os

import numpy as np
import uproot
import plotly.graph_objects as go


def main():
    ap = argparse.ArgumentParser(description="Background position + momentum cones (3D, plotly)")
    ap.add_argument("run", type=int, nargs="?", default=13, help="run number")
    ap.add_argument("--base", default="./Results_10mev_e_mp_mc_coin_proton120GeV_custom_gen",
                    help="base directory containing local_XXXX folders")
    ap.add_argument("--nthreads", type=int, default=6, help="number of output thread files")
    ap.add_argument("--maxpts", type=int, default=3000,
                    help="max background vertices/cones to draw (downsampled)")
    ap.add_argument("--scale", type=float, default=80.0,
                    help="cone length in mm (momentum is normalised to direction)")
    ap.add_argument("--out", default=None, help="output HTML path (default: run dir)")
    ap.add_argument("--show", action="store_true", help="open in a browser tab")
    args = ap.parse_args()

    path = f"{args.base}/local_{args.run:04d}"
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

    # Downsample for readability / HTML size
    n_draw = min(n_bkg, args.maxpts)
    rng = np.random.default_rng(42)
    idx = rng.choice(n_bkg, size=n_draw, replace=False)

    x = arr["trueVertexX"][idx]
    y = arr["trueVertexY"][idx]
    z = arr["trueVertexZ"][idx]
    px = arr["trueMomX"][idx]
    py = arr["trueMomY"][idx]
    pz = arr["trueMomZ"][idx]

    # Normalise momentum to a unit direction (arrow length set by --scale)
    p = np.sqrt(px * px + py * py + pz * pz)
    p[p == 0.0] = 1.0
    ux = px / p * args.scale
    uy = py / p * args.scale
    uz = pz / p * args.scale

    print(f"Background vertices: {n_bkg}  (drawing {n_draw})")
    print(f"  pos mean = ({x.mean():.1f}, {y.mean():.1f}, {z.mean():.1f}) mm")
    print(f"  mom mean = ({px.mean():.4f}, {py.mean():.4f}, {pz.mean():.4f}) GeV")

    fig = go.Figure()

    # Background vertex positions
    fig.add_trace(go.Scatter3d(
        x=x, y=y, z=z,
        mode="markers",
        marker=dict(size=2, color="red", opacity=0.35),
        name="background vertices",
    ))

    # Momentum-direction cones (anchored at the tail, pointing along momentum)
    fig.add_trace(go.Cone(
        x=x, y=y, z=z,
        u=ux, v=uy, w=uz,
        anchor="tail",
        sizemode="absolute",
        sizeref=1.0,
        colorscale=[[0, "blue"], [1, "blue"]],
        showscale=False,
        name="momentum direction",
    ))

    fig.update_layout(
        title=f"Background position + momentum direction (run {args.run}, "
              f"{n_draw}/{n_bkg} shown)",
        scene=dict(
            xaxis_title="x [mm]",
            yaxis_title="y [mm]",
            zaxis_title="z [mm]",
            aspectmode="data",
        ),
    )

    out = args.out or f"{path}/bkg_momentum3D.html"
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    fig.write_html(out)
    print(f"Saved: {out}")

    if args.show:
        fig.show()


if __name__ == "__main__":
    main()
