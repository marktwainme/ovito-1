///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __OVITO_LAMMPS_DATA_FILE_EXPORTER_H
#define __OVITO_LAMMPS_DATA_FILE_EXPORTER_H

#include <plugins/particles/Particles.h>
#include "../ParticleExporter.h"
#include <plugins/particles/import/lammps/LAMMPSDataImporter.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

/**
 * \brief Exporter that writes the particles to a LAMMPS data file.
 */
class OVITO_PARTICLES_EXPORT LAMMPSDataExporter : public ParticleExporter
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE LAMMPSDataExporter(DataSet* dataset) : ParticleExporter(dataset), _atomStyle(LAMMPSDataImporter::AtomStyle_Atomic) {
		INIT_PROPERTY_FIELD(LAMMPSDataExporter::_atomStyle);
	}

	/// \brief Returns the file filter that specifies the files that can be exported by this service.
	virtual QString fileFilter() override { return QStringLiteral("*"); }

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	virtual QString fileFilterDescription() override { return tr("LAMMPS Data File"); }

	/// Returns the format variant being written by this data file exporter.
	LAMMPSDataImporter::LAMMPSAtomStyle atomStyle() const { return _atomStyle; }

	/// Sets the kind of data file to write.
	void setAtomStyle(LAMMPSDataImporter::LAMMPSAtomStyle style) { _atomStyle = style; }

protected:

	/// \brief Writes the particles of one animation frame to the current output file.
	virtual bool exportObject(SceneNode* sceneNode, int frameNumber, TimePoint time, const QString& filePath, AbstractProgressDisplay* progressDisplay) override;

private:

	/// Selects the kind of data file to write.
	PropertyField<LAMMPSDataImporter::LAMMPSAtomStyle, int> _atomStyle;

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_PROPERTY_FIELD(_atomStyle);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_LAMMPS_DATA_FILE_EXPORTER_H
