import ovito
from ovito.io import import_file

test_data_dir = "../../files/"

node1 = import_file(test_data_dir + "LAMMPS/animation.dump.gz")
assert(len(ovito.dataset.scene_nodes) == 0)
import_file(test_data_dir + "CFG/fcc_coherent_twin.0.cfg")
import_file(test_data_dir + "CFG/shear.void.120.cfg")
import_file(test_data_dir + "Parcas/movie.0000000.parcas")
import_file(test_data_dir + "IMD/nw2.imd.gz")
import_file(test_data_dir + "FHI-aims/3_geometry.in.next_step")
import_file(test_data_dir + "GSD/test.gsd")
import_file(test_data_dir + "GSD/triblock.gsd")
import_file(test_data_dir + "PDB/SiShuffle.pdb")
import_file(test_data_dir + "PDB/trjconv_gromacs.pdb")
import_file(test_data_dir + "POSCAR/Ti_n1_PBE.n54_G7_V15.000.poscar.000")
import_file(test_data_dir + "NetCDF/C60_impact.nc")
import_file(test_data_dir + "NetCDF/sheared_aSi.nc")
import_file(test_data_dir + "VTK/mesh_test.vtk")
import_file(test_data_dir + "VTK/ThomsonTet_Gr1_rotmatNonRand_unstructGrid.vtk")
node = import_file(test_data_dir + "LAMMPS/multi_sequence_*.dump")
assert(ovito.dataset.anim.last_frame == 2)
node = import_file(test_data_dir + "LAMMPS/shear.void.dump.bin", 
                            columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
try:
    # This should generate an error:
    node = import_file(test_data_dir + "LAMMPS/shear.void.dump.bin",  
                                columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z", "ExtraProperty"])
    assert False
except RuntimeError:
    pass

node = import_file(test_data_dir + "LAMMPS/animation*.dump")
assert(ovito.dataset.anim.last_frame == 0)
node = import_file(test_data_dir + "LAMMPS/animation*.dump", multiple_frames = True)
assert(ovito.dataset.anim.last_frame == 10)
