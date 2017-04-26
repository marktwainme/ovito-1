from ovito.io import *
from ovito.modifiers import *
import numpy as np

node = import_file("../../files/CFG/shear.void.120.cfg")

modifier = CoordinationNumberModifier()
node.modifiers.append(modifier)

print("Parameter defaults:")

print("  cutoff: {}".format(modifier.cutoff))
print("  number_of_bins: {}".format(modifier.number_of_bins))
modifier.cutoff = 3.0
modifier.number_of_bins = 400

node.compute()

print("Output:")
print(node.output.coordination)
print(node.output.coordination.array)
print("RDF:")
print(modifier.rdf)
