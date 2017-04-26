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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/util/ParticleSelectionSet.h>
#include "../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

/**
 * Modifiers that allows the user to select individual particles by hand.
 */
class OVITO_PARTICLES_EXPORT ManualSelectionModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE ManualSelectionModifier(DataSet* dataset) : ParticleModifier(dataset) {}

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override { return TimeInterval::infinite(); }

	/// Adopts the selection state from the modifier's input.
	void resetSelection(ModifierApplication* modApp, const PipelineFlowState& state);

	/// Selects all particles.
	void selectAll(ModifierApplication* modApp, const PipelineFlowState& state);

	/// Deselects all particles.
	void clearSelection(ModifierApplication* modApp, const PipelineFlowState& state);

	/// Toggles the selection state of a single particle.
	void toggleParticleSelection(ModifierApplication* modApp, const PipelineFlowState& state, size_t particleIndex);

	/// Replaces the particle selection.
	void setParticleSelection(ModifierApplication* modApp, const PipelineFlowState& state, const QBitArray& selection, ParticleSelectionSet::SelectionMode mode);

protected:

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// Returns the selection set object stored in the ModifierApplication, or, if it does not exist, creates one when requested.
	ParticleSelectionSet* getSelectionSet(ModifierApplication* modApp, bool createIfNotExist = false);

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Manual selection");
	Q_CLASSINFO("ModifierCategory", "Selection");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


