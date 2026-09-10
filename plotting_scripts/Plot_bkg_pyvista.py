import uproot
import numpy as np
import pyvista as pv

# --- Minimal Config ---
run = 13
base_dir = "./Results_10mev_e_mp_mc_coin_proton120GeV_custom_gen"
n_points = 200          

# INCREASED LENGTH: Change this to make arrows even longer
scale_factor = 50.0   
# ----------------------

# 1. Load Data
path = f"{base_dir}/local_{run:04d}/output0_t0.root:TruthVertex"
cols = ["trueVertexX", "trueVertexY", "trueVertexZ", "trueMomX", "trueMomY", "trueMomZ"]

raw = uproot.concatenate([path], filter_name=cols + ["mcFlag"], library="np")
mask = raw["mcFlag"] == 1

x = raw["trueVertexX"][mask][:n_points]
y = raw["trueVertexY"][mask][:n_points]
z = raw["trueVertexZ"][mask][:n_points]
px = raw["trueMomX"][mask][:n_points]
py = raw["trueMomY"][mask][:n_points]
pz = raw["trueMomZ"][mask][:n_points]

# 2. Prepare Point and Vector Arrays
points = np.column_stack((x, y, z))
vectors = np.column_stack((px, py, pz))

norms = np.linalg.norm(vectors, axis=1, keepdims=True)
norms[norms == 0] = 1.0
vectors = (vectors / norms) * scale_factor

# 3. Plot with PyVista
plotter = pv.Plotter()

cloud = pv.PolyData(points)
cloud['momentum'] = vectors

# Draw the vertices
plotter.add_points(cloud, color='red', point_size=10, render_points_as_spheres=True)

# ==========================================
# THE FIX: Custom Thin Arrow Geometry
# ==========================================
# Default values for reference: shaft_radius=0.05, tip_radius=0.1
thin_arrow = pv.Arrow(
    shaft_radius=0.009,  # Make the line/shaft extremely thin
    tip_radius=0.03,    # Make the arrowhead very thin
    tip_length=0.16     # Keep the arrowhead relatively short compared to the long shaft
)

# Apply the custom thin arrow to the glyph
arrows = cloud.glyph(geom=thin_arrow, orient='momentum', scale='momentum', factor=1.0)
plotter.add_mesh(arrows, color='cyan')

# Stretch X and Y by 40x and leave Z at 1x to fix the aspect ratio
plotter.set_scale(xscale=100.0, yscale=100.0, zscale=10.0)

plotter.show_grid()
plotter.show()