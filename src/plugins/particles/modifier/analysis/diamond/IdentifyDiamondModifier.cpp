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

#include <plugins/particles/Particles.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/particles/modifier/analysis/cna/CommonNeighborAnalysisModifier.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "IdentifyDiamondModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(IdentifyDiamondModifier, StructureIdentificationModifier);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
IdentifyDiamondModifier::IdentifyDiamondModifier(DataSet* dataset) : StructureIdentificationModifier(dataset)
{
	// Create the structure types.
	createStructureType(OTHER, ParticleTypeProperty::PredefinedStructureType::OTHER);
	createStructureType(CUBIC_DIAMOND, ParticleTypeProperty::PredefinedStructureType::CUBIC_DIAMOND);
	createStructureType(CUBIC_DIAMOND_FIRST_NEIGH, ParticleTypeProperty::PredefinedStructureType::CUBIC_DIAMOND_FIRST_NEIGH);
	createStructureType(CUBIC_DIAMOND_SECOND_NEIGH, ParticleTypeProperty::PredefinedStructureType::CUBIC_DIAMOND_SECOND_NEIGH);
	createStructureType(HEX_DIAMOND, ParticleTypeProperty::PredefinedStructureType::HEX_DIAMOND);
	createStructureType(HEX_DIAMOND_FIRST_NEIGH, ParticleTypeProperty::PredefinedStructureType::HEX_DIAMOND_FIRST_NEIGH);
	createStructureType(HEX_DIAMOND_SECOND_NEIGH, ParticleTypeProperty::PredefinedStructureType::HEX_DIAMOND_SECOND_NEIGH);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> IdentifyDiamondModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	if(structureTypes().size() != NUM_STRUCTURE_TYPES)
		throwException(tr("The number of structure types has changed. Please remove this modifier from the modification pipeline and insert it again."));

	// Get modifier input.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Get particle selection.
	ParticleProperty* selectionProperty = nullptr;
	if(onlySelectedParticles())
		selectionProperty = expectStandardProperty(ParticleProperty::SelectionProperty)->storage();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<DiamondIdentificationEngine>(validityInterval, posProperty->storage(), simCell->data(), getTypesToIdentify(NUM_STRUCTURE_TYPES), selectionProperty);
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void IdentifyDiamondModifier::DiamondIdentificationEngine::perform()
{
	setProgressText(tr("Finding nearest neighbors"));

	// Prepare the neighbor list builder.
	NearestNeighborFinder neighborFinder(4);
	if(!neighborFinder.prepare(positions(), cell(), selection(), *this))
		return;

	// This data structure stores information about a single neighbor.
	struct NeighborInfo {
		Vector3 vec;
		int index;
	};
	// This array will be filled with the four nearest neighbors of each atom.
	std::vector<std::array<NeighborInfo,4>> neighLists(positions()->size());

	// Determine four nearest neighbors of each atom and store vectors in the working array.
	parallelFor(positions()->size(), *this, [this, &neighborFinder, &neighLists](size_t index) {
		// Skip particles that are not included in the analysis.
		if(selection() && selection()->getInt(index) == 0)
			return;
		NearestNeighborFinder::Query<4> neighQuery(neighborFinder);
		neighQuery.findNeighbors(index);
		for(int i = 0; i < neighQuery.results().size(); i++) {
			neighLists[index][i].vec = neighQuery.results()[i].delta;
			neighLists[index][i].index = neighQuery.results()[i].index;
			OVITO_ASSERT(!selection() || selection()->getInt(neighLists[index][i].index));
		}
		for(int i = neighQuery.results().size(); i < 4; i++) {
			neighLists[index][i].vec.setZero();
			neighLists[index][i].index = -1;
		}
	});

	// Create output storage.
	ParticleProperty* output = structures();

	// Perform structure identification.
	setProgressText(tr("Identifying diamond structures"));
	parallelFor(positions()->size(), *this, [&neighLists, output, this](size_t index) {
		// Mark atom as 'other' by default.
		output->setInt(index, OTHER);

		// Skip particles that are not included in the analysis.
		if(selection() && selection()->getInt(index) == 0)
			return;

		const std::array<NeighborInfo,4>& nlist = neighLists[index];

		// Generate list of second nearest neighbors.
		std::array<Vector3,12> secondNeighbors;
		auto vout = secondNeighbors.begin();
		for(size_t i = 0; i < 4; i++) {
			if(nlist[i].index == -1) return;
			const Vector3& v0 = nlist[i].vec;
			const std::array<NeighborInfo,4>& nlist2 = neighLists[nlist[i].index];
			for(size_t j = 0; j < 4; j++) {
				Vector3 v = v0 + nlist2[j].vec;
				if(v.isZero(1e-2f)) continue;
				if(vout == secondNeighbors.end()) return;
				*vout++ = v;
			}
			if(vout != secondNeighbors.begin() + i*3 + 3) return;
		}

		// Compute a local CNA cutoff radius from the average distance of the 12 second nearest neighbors.
		FloatType sum = 0;
		for(const Vector3& v : secondNeighbors)
			sum += v.length();
		sum /= 12;
		const FloatType factor = FloatType(1.2071068);   // = sqrt(2.0) * ((1.0 + sqrt(0.5)) / 2)
		FloatType localCutoff = sum * factor;
		FloatType localCutoffSquared = localCutoff * localCutoff;

		// Determine bonds between common neighbors using local cutoff.
		CommonNeighborAnalysisModifier::NeighborBondArray neighborArray;
		for(int ni1 = 0; ni1 < 12; ni1++) {
			neighborArray.setNeighborBond(ni1, ni1, false);
			for(int ni2 = ni1+1; ni2 < 12; ni2++)
				neighborArray.setNeighborBond(ni1, ni2, (secondNeighbors[ni1] - secondNeighbors[ni2]).squaredLength() <= localCutoffSquared);
		}

		// Determine whether second nearest neighbors form FCC or HCP using common neighbor analysis.
		int n421 = 0;
		int n422 = 0;
		for(int ni = 0; ni < 12; ni++) {

			// Determine number of neighbors the two atoms have in common.
			unsigned int commonNeighbors;
			int numCommonNeighbors = CommonNeighborAnalysisModifier::findCommonNeighbors(neighborArray, ni, commonNeighbors, 12);
			if(numCommonNeighbors != 4) return;

			// Determine the number of bonds among the common neighbors.
			CommonNeighborAnalysisModifier::CNAPairBond neighborBonds[12*12];
			int numNeighborBonds = CommonNeighborAnalysisModifier::findNeighborBonds(neighborArray, commonNeighbors, 12, neighborBonds);
			if(numNeighborBonds != 2) return;

			// Determine the number of bonds in the longest continuous chain.
			int maxChainLength = CommonNeighborAnalysisModifier::calcMaxChainLength(neighborBonds, numNeighborBonds);
			if(maxChainLength == 1) n421++;
			else if(maxChainLength == 2) n422++;
			else return;
		}
		if(n421 == 12 && typesToIdentify()[CUBIC_DIAMOND]) output->setInt(index, CUBIC_DIAMOND);
		else if(n421 == 6 && n422 == 6 && typesToIdentify()[HEX_DIAMOND]) output->setInt(index, HEX_DIAMOND);
	});

	// Mark first neighbors of crystalline atoms.
	for(size_t index = 0; index < output->size(); index++) {
		int ctype = output->getInt(index);
		if(ctype != CUBIC_DIAMOND && ctype != HEX_DIAMOND)
			continue;
		if(selection() && selection()->getInt(index) == 0)
			continue;

		const std::array<NeighborInfo,4>& nlist = neighLists[index];
		for(size_t i = 0; i < 4; i++) {
			OVITO_ASSERT(nlist[i].index != -1);
			if(output->getInt(nlist[i].index) == OTHER) {
				if(ctype == CUBIC_DIAMOND)
					output->setInt(nlist[i].index, CUBIC_DIAMOND_FIRST_NEIGH);
				else
					output->setInt(nlist[i].index, HEX_DIAMOND_FIRST_NEIGH);
			}
		}
	}

	// Mark second neighbors of crystalline atoms.
	for(size_t index = 0; index < output->size(); index++) {
		int ctype = output->getInt(index);
		if(ctype != CUBIC_DIAMOND_FIRST_NEIGH && ctype != HEX_DIAMOND_FIRST_NEIGH)
			continue;
		if(selection() && selection()->getInt(index) == 0)
			continue;

		const std::array<NeighborInfo,4>& nlist = neighLists[index];
		for(size_t i = 0; i < 4; i++) {
			if(nlist[i].index != -1 && output->getInt(nlist[i].index) == OTHER) {
				if(ctype == CUBIC_DIAMOND_FIRST_NEIGH)
					output->setInt(nlist[i].index, CUBIC_DIAMOND_SECOND_NEIGH);
				else
					output->setInt(nlist[i].index, HEX_DIAMOND_SECOND_NEIGH);
			}
		}
	}
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the modification pipeline.
******************************************************************************/
PipelineStatus IdentifyDiamondModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	// Let the base class output the structure type property to the pipeline.
	PipelineStatus status = StructureIdentificationModifier::applyComputationResults(time, validityInterval);

	// Also output structure type counts, which have been computed by the base class.
	if(status.type() == PipelineStatus::Success) {
		output().attributes().insert(QStringLiteral("IdentifyDiamond.counts.OTHER"), QVariant::fromValue(structureCounts()[OTHER]));
		output().attributes().insert(QStringLiteral("IdentifyDiamond.counts.CUBIC_DIAMOND"), QVariant::fromValue(structureCounts()[CUBIC_DIAMOND]));
		output().attributes().insert(QStringLiteral("IdentifyDiamond.counts.CUBIC_DIAMOND_FIRST_NEIGHBOR"), QVariant::fromValue(structureCounts()[CUBIC_DIAMOND_FIRST_NEIGH]));
		output().attributes().insert(QStringLiteral("IdentifyDiamond.counts.CUBIC_DIAMOND_SECOND_NEIGHBOR"), QVariant::fromValue(structureCounts()[CUBIC_DIAMOND_SECOND_NEIGH]));
		output().attributes().insert(QStringLiteral("IdentifyDiamond.counts.HEX_DIAMOND"), QVariant::fromValue(structureCounts()[HEX_DIAMOND]));
		output().attributes().insert(QStringLiteral("IdentifyDiamond.counts.HEX_DIAMOND_FIRST_NEIGHBOR"), QVariant::fromValue(structureCounts()[HEX_DIAMOND_FIRST_NEIGH]));
		output().attributes().insert(QStringLiteral("IdentifyDiamond.counts.HEX_DIAMOND_SECOND_NEIGHBOR"), QVariant::fromValue(structureCounts()[HEX_DIAMOND_SECOND_NEIGH]));
	}

	return status;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
