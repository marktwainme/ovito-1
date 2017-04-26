from ovito.io import *
from ovito.modifiers import *

import numpy

node = import_file("../../files/LAMMPS/animation.dump.gz")
node.modifiers.append(AssignColorModifier(color = (0,1,0)))

assert((node.compute().particle_properties.color.array[0] == numpy.array([0,1,0])).all())
