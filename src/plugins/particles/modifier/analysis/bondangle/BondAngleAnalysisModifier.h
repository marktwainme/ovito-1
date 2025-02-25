///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#ifndef __OVITO_BOND_ANGLE_ANALYSIS_MODIFIER_H
#define __OVITO_BOND_ANGLE_ANALYSIS_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief A modifier that performs the structure analysis developed by Ackland and Jones.
 *
 * See G. Ackland, PRB(2006)73:054104.
 */
class OVITO_PARTICLES_EXPORT BondAngleAnalysisModifier : public StructureIdentificationModifier
{
public:

	/// The structure types recognized by the bond angle analysis.
	enum StructureType {
		OTHER = 0,				//< Unidentified structure
		FCC,					//< Face-centered cubic
		HCP,					//< Hexagonal close-packed
		BCC,					//< Body-centered cubic
		ICO,					//< Icosahedral structure

		NUM_STRUCTURE_TYPES 	//< This just counts the number of defined structure types.
	};
	Q_ENUMS(StructureType);

public:

	/// Constructor.
	Q_INVOKABLE BondAngleAnalysisModifier(DataSet* dataset);

protected:

	/// Creates and initializes a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Computes the modifier's results.
	class BondAngleAnalysisEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		BondAngleAnalysisEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell, const QVector<bool>& typesToIdentify, ParticleProperty* selection) :
			StructureIdentificationEngine(validityInterval, positions, simCell, typesToIdentify, selection) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;
	};

	/// Determines the coordination structure of a single particle using the bond-angle analysis method.
	static StructureType determineStructure(NearestNeighborFinder& neighFinder, size_t particleIndex, const QVector<bool>& typesToIdentify);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Bond-angle analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_BOND_ANGLE_ANALYSIS_MODIFIER_H
