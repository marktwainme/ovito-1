///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <core/utilities/concurrent/ProgressDisplay.h>
#include "XYZExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, XYZExporter, FileColumnParticleExporter);
DEFINE_FLAGS_PROPERTY_FIELD(XYZExporter, _subFormat, "XYZSubFormat", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(XYZExporter, _subFormat, "Format style");

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool XYZExporter::exportObject(SceneNode* sceneNode, int frameNumber, TimePoint time, const QString& filePath, AbstractProgressDisplay* progress)
{
	// Get particle positions.
	const PipelineFlowState& state = getParticleData(sceneNode, time);
	ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(state, ParticleProperty::PositionProperty);

	size_t atomsCount = posProperty->size();
	textStream() << atomsCount << '\n';

	const OutputColumnMapping& mapping = columnMapping();
	if(mapping.empty())
		throwException(tr("No particle properties have been selected for export to the XYZ file. Cannot write file with zero columns."));
	OutputColumnWriter columnWriter(mapping, state, true);

	SimulationCellObject* simulationCell = state.findObject<SimulationCellObject>();

	if(subFormat() == ParcasFormat) {
		textStream() << QStringLiteral("Frame %1").arg(frameNumber);
		if(simulationCell) {
			AffineTransformation simCell = simulationCell->cellMatrix();
			textStream() << " cell_orig " << simCell.translation().x() << " " << simCell.translation().y() << " " << simCell.translation().z();
			textStream() << " cell_vec1 " << simCell.column(0).x() << " " << simCell.column(0).y() << " " << simCell.column(0).z();
			textStream() << " cell_vec2 " << simCell.column(1).x() << " " << simCell.column(1).y() << " " << simCell.column(1).z();
			textStream() << " cell_vec3 " << simCell.column(2).x() << " " << simCell.column(2).y() << " " << simCell.column(2).z();
			textStream() << " pbc " << simulationCell->pbcX() << " " << simulationCell->pbcY() << " " << simulationCell->pbcZ();
		}
	}
	else if(subFormat() == ExtendedFormat) {
		if(simulationCell) {
			AffineTransformation simCell = simulationCell->cellMatrix();
			// Save cell information in extended XYZ format:
			// see http://jrkermode.co.uk/quippy/io.html#extendedxyz for details
			QString latticeStr;
			latticeStr = latticeStr.sprintf("Lattice=\"%16.8f %16.8f %16.8f %16.8f %16.8f %16.8f %16.8f %16.8f %16.8f\" ",
							simCell.column(0).x(), simCell.column(0).y(), simCell.column(0).z(),
							simCell.column(1).x(), simCell.column(1).y(), simCell.column(1).z(),
							simCell.column(2).x(), simCell.column(2).y(), simCell.column(2).z());
			textStream() << latticeStr;
		}
		// Save column information in extended XYZ format:
		// see http://jrkermode.co.uk/quippy/io.html#extendedxyz for details
		textStream() << QStringLiteral("Properties=");
		QString propertiesStr;
		int i = 0;
		while(i < (int)mapping.size()) {
			const ParticlePropertyReference& pref = mapping[i];

			// Convert from OVITO property type and name to extended XYZ property name
			// Naming conventions followed are those of the QUIP code
			QString columnName;
			switch(pref.type()) {
			case ParticleProperty::ParticleTypeProperty: columnName = QStringLiteral("species"); break;
			case ParticleProperty::PositionProperty: columnName = QStringLiteral("pos"); break;
			case ParticleProperty::SelectionProperty: columnName = QStringLiteral("selection"); break;
			case ParticleProperty::ColorProperty: columnName = QStringLiteral("color"); break;
			case ParticleProperty::DisplacementProperty: columnName = QStringLiteral("disp"); break;
			case ParticleProperty::DisplacementMagnitudeProperty: columnName = QStringLiteral("disp_mag"); break;
			case ParticleProperty::PotentialEnergyProperty: columnName = QStringLiteral("local_energy"); break;
			case ParticleProperty::KineticEnergyProperty: columnName = QStringLiteral("kinetic_energy"); break;
			case ParticleProperty::TotalEnergyProperty: columnName = QStringLiteral("total_energy"); break;
			case ParticleProperty::VelocityProperty: columnName = QStringLiteral("velo"); break;
			case ParticleProperty::VelocityMagnitudeProperty: columnName = QStringLiteral("velo_mag"); break;
			case ParticleProperty::RadiusProperty: columnName = QStringLiteral("radius"); break;
			case ParticleProperty::ClusterProperty: columnName = QStringLiteral("cluster"); break;
			case ParticleProperty::CoordinationProperty: columnName = QStringLiteral("n_neighb"); break;
			case ParticleProperty::StructureTypeProperty: columnName = QStringLiteral("structure_type"); break;
			case ParticleProperty::IdentifierProperty: columnName = QStringLiteral("id"); break;
			case ParticleProperty::StressTensorProperty: columnName = QStringLiteral("stress"); break;
			case ParticleProperty::StrainTensorProperty: columnName = QStringLiteral("strain"); break;
			case ParticleProperty::DeformationGradientProperty: columnName = QStringLiteral("deform"); break;
			case ParticleProperty::OrientationProperty: columnName = QStringLiteral("orientation"); break;
			case ParticleProperty::ForceProperty: columnName = QStringLiteral("force"); break;
			case ParticleProperty::MassProperty: columnName = QStringLiteral("mass"); break;
			case ParticleProperty::ChargeProperty: columnName = QStringLiteral("charge"); break;
			case ParticleProperty::PeriodicImageProperty: columnName = QStringLiteral("map_shift"); break;
			case ParticleProperty::TransparencyProperty: columnName = QStringLiteral("transparency"); break;
			case ParticleProperty::DipoleOrientationProperty: columnName = QStringLiteral("dipoles"); break;
			case ParticleProperty::DipoleMagnitudeProperty: columnName = QStringLiteral("dipoles_mag"); break;
			case ParticleProperty::AngularVelocityProperty: columnName = QStringLiteral("omega"); break;
			case ParticleProperty::AngularMomentumProperty: columnName = QStringLiteral("angular_momentum"); break;
			case ParticleProperty::TorqueProperty: columnName = QStringLiteral("torque"); break;
			case ParticleProperty::SpinProperty: columnName = QStringLiteral("spin"); break;
			case ParticleProperty::CentroSymmetryProperty: columnName = QStringLiteral("centro_symmetry"); break;
			default:
				columnName = pref.name();
				columnName.remove(QRegExp("[^A-Za-z\\d_]"));
			}

			// Find matching property
			ParticlePropertyObject* property = pref.findInState(state);
			if(property == nullptr && pref.type() != ParticleProperty::IdentifierProperty)
				throwException(tr("Particle property '%1' cannot be exported because it does not exist.").arg(pref.name()));

			// Count the number of consecutive columns with the same property.
			int nCols = 1;
			while(++i < (int)mapping.size() && pref.name() == mapping[i].name() && pref.type() == mapping[i].type())
				nCols++;

			// Convert OVITO property data type to extended XYZ type code: 'I','R','S','L'
			int dataType = property ? property->dataType() : qMetaTypeId<int>();
			QString dataTypeStr;
			if(dataType == qMetaTypeId<FloatType>())
				dataTypeStr = QStringLiteral("R");
			else if(dataType == qMetaTypeId<char>() || pref.type() == ParticleProperty::ParticleTypeProperty)
				dataTypeStr = QStringLiteral("S");
			else if(dataType == qMetaTypeId<int>())
				dataTypeStr = QStringLiteral("I");
			else if(dataType == qMetaTypeId<bool>())
				dataTypeStr = QStringLiteral("L");
			else
				throwException(tr("Unexpected data type '%1' for property '%2'.").arg(QMetaType::typeName(dataType) ? QMetaType::typeName(dataType) : "unknown").arg(pref.name()));

			if(!propertiesStr.isEmpty()) propertiesStr += QStringLiteral(":");
			propertiesStr += QStringLiteral("%1:%2:%3").arg(columnName).arg(dataTypeStr).arg(nCols);
		}
		textStream() << propertiesStr;
	}
	textStream() << '\n';

	if(progress) progress->setMaximum(100);
	for(size_t i = 0; i < atomsCount; i++) {
		columnWriter.writeParticle(i, textStream());

		if(progress && (i % 4096) == 0) {
			progress->setValue((quint64)i * 100 / atomsCount);
			if(progress->wasCanceled())
				return false;
		}
	}

	return true;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
