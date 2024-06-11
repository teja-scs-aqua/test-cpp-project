#python

# This script uses Python to load an existing K-3D document, load a sample OBJ file
# into the document, then render the results using RenderMan. It provides an example
# of a "batch rendering" scenario where the user has prepared a document with the K-3D
# graphical user interface, then uses a script to modify the document and render the
# results to a file in the current working directory.
# 
# This scenario can be adapted to many different use-cases.  For example, this
# script could be modified to load a series of OBJ files into the document, rendering
# each in-turn.  The script could also modify other properties of the document to
# create animated effects.
#
# To run the script, do the following at the shell:
#
# $ k3d --ui=none --script-file=/path/to/batch_render.py --batch --exit
#
# The 

import k3d
import os

# Load our "template" document.  We assume that the document already contains  materials / lighting / render options to suit.
document = k3d.open_document(k3d.share_path() / k3d.filesystem.generic_path("scripts/shell/batch_render.k3d"))

# Load an external OBJ file into the document.
obj_reader = k3d.node.lookup_one(document, "OBJMeshReader")
obj_reader.filepath = k3d.share_path() / k3d.filesystem.generic_path("geometry/knot.obj")

# Figure-out where to put the rendered results.
output = k3d.filesystem.native_path(os.getcwd()) / k3d.filesystem.generic_path("batch_render.tiff")
print output

# Render the document to the current directory.
camera = k3d.node.lookup_one(document, "Camera")
render_engine = k3d.node.lookup_one(document, "RenderMan Engine")
render_engine.render_camera_frame(camera, output, False)

