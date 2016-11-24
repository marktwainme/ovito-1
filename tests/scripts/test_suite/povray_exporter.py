import ovito
from ovito.io import (import_file, export_file)
from ovito.vis import *
import os

test_data_dir = "../../files/"

node1 = import_file(test_data_dir + "LAMMPS/class2.data", atom_style = "full")
node1.add_to_scene()
node1.source.particle_properties.position.display.shape = ParticleDisplay.Shape.Square
node1.source.particle_properties.position.display.radius = 0.3
export_file(node1, "test.pov", "povray")
export_file(None, "test.pov", "povray")
os.remove("test.pov")
