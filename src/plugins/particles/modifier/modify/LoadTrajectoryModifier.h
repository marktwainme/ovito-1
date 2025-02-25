///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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

#ifndef __OVITO_LOAD_TRAJECTORY_MODIFIER_H
#define __OVITO_LOAD_TRAJECTORY_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/modifier/ParticleModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief Loads particle trajectories from a separate file and injects them into the modification pipeline.
 */
class OVITO_PARTICLES_EXPORT LoadTrajectoryModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE LoadTrajectoryModifier(DataSet* dataset);

	/// Returns the data object that provides the particle trajectories.
	DataObject* trajectorySource() const { return _trajectorySource; }

	/// Sets the object that will provide the particle trajectories.
	void setTrajectorySource(DataObject* refConf) { _trajectorySource = refConf; }

protected:

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// The source for trajectory data.
	ReferenceField<DataObject> _trajectorySource;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Load trajectory");
	Q_CLASSINFO("ModifierCategory", "Modification");

	DECLARE_REFERENCE_FIELD(_trajectorySource);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_LOAD_TRAJECTORY_MODIFIER_H
