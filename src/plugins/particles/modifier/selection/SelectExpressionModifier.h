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
#include "../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

/**
 * \brief Selects particles based on a user-defined Boolean expression.
 */
class OVITO_PARTICLES_EXPORT SelectExpressionModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE SelectExpressionModifier(DataSet* dataset) : ParticleModifier(dataset) {
		INIT_PROPERTY_FIELD(expression);
	}

	/// \brief Returns the list of available input variables.
	const QStringList& inputVariableNames() const { return _variableNames; }

	/// \brief Returns a human-readable text listing the input variables.
	const QString& inputVariableTable() const { return _variableTable; }

protected:

	/// \brief This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp) override;

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// The expression that is used to select atoms.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, expression, setExpression);

	/// The list of input variables during the last evaluation.
	QStringList _variableNames;

	/// Human-readable text listing the input variables during the last evaluation.
	QString _variableTable;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Expression select");
	Q_CLASSINFO("ModifierCategory", "Selection");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


