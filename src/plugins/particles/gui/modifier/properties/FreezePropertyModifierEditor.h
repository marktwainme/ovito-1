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

#ifndef __OVITO_FREEZE_PROPERTY_MODIFIER_EDITOR_H
#define __OVITO_FREEZE_PROPERTY_MODIFIER_EDITOR_H

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/gui/modifier/ParticleModifierEditor.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the FreezePropertyModifier class.
 */
class FreezePropertyModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor
	Q_INVOKABLE FreezePropertyModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Takes a new snapshot of the current particle property values.
	void takeSnapshot();

	/// Is called when the user has selected a different source property.
	void onSourcePropertyChanged();

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_FREEZE_PROPERTY_MODIFIER_EDITOR_H
