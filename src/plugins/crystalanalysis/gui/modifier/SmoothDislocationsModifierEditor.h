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

#ifndef __OVITO_CA_SMOOTH_DISLOCATIONS_MODIFIER_EDITOR_H
#define __OVITO_CA_SMOOTH_DISLOCATIONS_MODIFIER_EDITOR_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <gui/properties/PropertiesEditor.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * Properties editor for the SmoothDislocationsModifier class.
 */
class SmoothDislocationsModifierEditor : public PropertiesEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE SmoothDislocationsModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_CA_SMOOTH_DISLOCATIONS_MODIFIER_EDITOR_H
