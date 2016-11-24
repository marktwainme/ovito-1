# Load dependencies
import ovito
import ovito.data
import ovito.modifiers
import ovito.modifiers.particles
import numpy

# Load the native code modules
import Particles
import ParticlesImporter
import ParticlesExporter
import CrystalAnalysis

# Inject selected classes into parent module.
ovito.data.DislocationNetwork = CrystalAnalysis.DislocationNetwork
ovito.data.DislocationSegment = CrystalAnalysis.DislocationSegment
ovito.data.PartitionMesh = CrystalAnalysis.PartitionMesh

# Implement 'dislocations' attribute of DataCollection class.
def _DataCollection_dislocations(self):
    """
    Returns the :py:class:`DislocationNetwork` object in this :py:class:`!DataCollection`.
    
    Accessing this property raises an ``AttributeError`` if the data collection
    contains no dislocations object.
    """
    for obj in self.objects:
        if isinstance(obj, ovito.data.DislocationNetwork):
            return obj
    raise AttributeError("This DataCollection contains no dislocations.")
ovito.data.DataCollection.dislocations = property(_DataCollection_dislocations)

