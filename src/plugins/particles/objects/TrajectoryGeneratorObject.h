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

#ifndef __OVITO_TRAJECTORY_GENERATOR_OBJECT_H
#define __OVITO_TRAJECTORY_GENERATOR_OBJECT_H

#include <plugins/particles/Particles.h>
#include "TrajectoryObject.h"

namespace Ovito { namespace Particles {

/**
 * \brief Generates trajectory data from a particles object.
 */
class OVITO_PARTICLES_EXPORT TrajectoryGeneratorObject : public TrajectoryObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE TrajectoryGeneratorObject(DataSet* dataset);

	/// Returns the object node providing the input particle data.
	ObjectNode* source() const { return _source; }

	/// Sets the object node providing the input particle data.
	void setSource(ObjectNode* source) { _source = source; }

	/// Returns which particles trajectories are created for.
	bool onlySelectedParticles() const { return _onlySelectedParticles; }

	/// Controls which particles trajectories are created for.
	void setOnlySelectedParticles(bool onlySelected) { _onlySelectedParticles = onlySelected; }

	/// Returns whether the created trajectories span the entire animation interval or a sub-interval.
	bool useCustomInterval() { return _useCustomInterval; }

	/// Controls whether the created trajectories span the entire animation interval or a sub-interval.
	void setUseCustomInterval(bool customInterval) { _useCustomInterval = customInterval; }

	/// Returns the the custom time interval.
	TimeInterval customInterval() const { return TimeInterval(_customIntervalStart, _customIntervalEnd); }

	/// Returns the start of the custom time interval.
	TimePoint customIntervalStart() const { return _customIntervalStart; }

	/// Sets the start of the custom time interval.
	void setCustomIntervalStart(TimePoint start) { _customIntervalStart = start; }

	/// Returns the end of the custom time interval.
	TimePoint customIntervalEnd() const { return _customIntervalEnd; }

	/// Sets the end of the custom time interval.
	void setCustomIntervalEnd(TimePoint end) { _customIntervalEnd = end; }

	/// Returns the sampling frequency for creating trajectories.
	int everyNthFrame() const { return _everyNthFrame; }

	/// Sets the sampling frequency for creating trajectories.
	void setEveryNthFrame(int n) { _everyNthFrame = n; }

	/// Returns whether trajectories are unwrapped when crossing periodic boundaries.
	bool unwrapTrajectories() const { return _unwrapTrajectories; }

	/// Sets whether trajectories should be unwrapped when crossing periodic boundaries.
	void setUnwrapTrajectories(bool unwrap) { _unwrapTrajectories = unwrap; }

	/// Updates the stored trajectories from the source particle object.
	bool generateTrajectories(AbstractProgressDisplay* progressDisplay = nullptr);

private:

	/// The object node providing the input particles.
	ReferenceField<ObjectNode> _source;

	/// Controls which particles trajectories are created for.
	PropertyField<bool> _onlySelectedParticles;

	/// Controls whether the created trajectories span the entire animation interval or a sub-interval.
	PropertyField<bool> _useCustomInterval;

	/// The start of the custom time interval.
	PropertyField<TimePoint> _customIntervalStart;

	/// The end of the custom time interval.
	PropertyField<TimePoint> _customIntervalEnd;

	/// The sampling frequency for creating trajectories.
	PropertyField<int> _everyNthFrame;

	/// Controls whether trajectories are unwrapped when crossing periodic boundaries.
	PropertyField<bool> _unwrapTrajectories;

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_REFERENCE_FIELD(_source);
	DECLARE_PROPERTY_FIELD(_onlySelectedParticles);
	DECLARE_PROPERTY_FIELD(_useCustomInterval);
	DECLARE_PROPERTY_FIELD(_customIntervalStart);
	DECLARE_PROPERTY_FIELD(_customIntervalEnd);
	DECLARE_PROPERTY_FIELD(_everyNthFrame);
	DECLARE_PROPERTY_FIELD(_unwrapTrajectories);
};

}	// End of namespace
}	// End of namespace

#endif // __OVITO_TRAJECTORY_GENERATOR_OBJECT_H
