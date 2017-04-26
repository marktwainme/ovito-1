import ovito
from ovito.io import *
from ovito.modifiers import *
import numpy as np

node = import_file("../../files/NetCDF/sheared_aSi.nc")

modifier = WignerSeitzAnalysisModifier()
node.modifiers.append(modifier)
modifier.reference.load("../../files/NetCDF/sheared_aSi.nc")

ovito.dataset.anim.current_frame = 4

print("Parameter defaults:")

print("  eliminate_cell_deformation: {}".format(modifier.eliminate_cell_deformation))
modifier.eliminate_cell_deformation = True

print("  frame_offset: {}".format(modifier.frame_offset))
modifier.frame_offset = 0

print("  reference_frame: {}".format(modifier.reference_frame))
modifier.reference_frame = 0

print("  use_frame_offset: {}".format(modifier.use_frame_offset))
modifier.use_frame_offset = False

node.compute()

print("Output:")
print("  vacancy_count= {}".format(modifier.vacancy_count))
print("  interstitial_count= {}".format(modifier.interstitial_count))
print("  vacancy_count= {}".format(node.output.attributes['WignerSeitz.vacancy_count']))
print("  interstitial_count= {}".format(node.output.attributes['WignerSeitz.interstitial_count']))
print(node.output["Occupancy"].array)

assert(node.output.attributes['WignerSeitz.vacancy_count'] == 970)
assert(modifier.vacancy_count == 970)
