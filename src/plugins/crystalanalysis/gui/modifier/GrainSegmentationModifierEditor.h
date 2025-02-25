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

#ifndef __OVITO_GRAIN_SEGMENTATION_MODIFIER_EDITOR_H
#define __OVITO_GRAIN_SEGMENTATION_MODIFIER_EDITOR_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/gui/modifier/ParticleModifierEditor.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * Properties editor for the GrainSegmentationModifier class.
 */
class GrainSegmentationModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE GrainSegmentationModifierEditor() {}

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

#endif // __OVITO_GRAIN_SEGMENTATION_MODIFIER_EDITOR_H
