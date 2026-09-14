import uproot
import numpy as np
import pyvista as pv

# --- Minimal Config ---
run = 5
base_dir = "./Results"
n_points = 200          

# INCREASED LENGTH: Change this to make arrows even longer
scale_factor = 50.0   
# ----------------------

# 1. Load Data
path = f"{base_dir}/local_{run:04d}/output0_t0.root:TruthVertex"
cols = ["trueVertexX", "trueVertexY", "trueVertexZ", "trueMomX", "trueMomY", "trueMomZ"]

raw = uproot.concatenate([path], filter_name=cols + ["mcFlag"], library="np")
mask = raw["mcFlag"] == 0

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

# Stretch X and Y less aggressively so arrows keep a more uniform on-screen length
plotter.set_scale(xscale=30.0, yscale=30.0, zscale=10.0)

# ==========================================
# Draw the two sensor planes (module ladder)
# ==========================================
# MALTA2 module: 1.86368 x 1.86368 cm; 4 modules in x, planes at z = 0 and 10 cm.
module_x = 1.86368    # cm
module_y = 1.86368    # cm
x_centers = [0.0, 1.86369, 3.72737, 5.59105]   # cm (module x-centres)
plane_z = [0.0, 10.0]                          # cm (plane z positions)

ladder_x_min = x_centers[0]  - module_x / 2.0
ladder_x_max = x_centers[-1] + module_x / 2.0
ladder_cx = (ladder_x_min + ladder_x_max) / 2.0
ladder_w  = ladder_x_max - ladder_x_min

for zc in plane_z:
    plane = pv.Plane(
        center=(ladder_cx * 10.0, 0.0, zc * 10.0),   # mm
        direction=(0.0, 0.0, 1.0),
        i_size=ladder_w * 10.0,                      # mm (x)
        j_size=module_y * 10.0,                      # mm (y)
    )
    plotter.add_mesh(plane, color="blue", opacity=0.25, show_edges=True,
                     name=f"plane_{zc:.0f}cm")

plotter.show_grid()
plotter.show()