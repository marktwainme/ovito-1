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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/objects/BondType.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/StringParameterUI.h>
#include "BondTypeEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(BondTypeEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(BondType, BondTypeEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void BondTypeEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Bond Type"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout1 = new QGridLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
#ifndef Q_OS_MACX
	layout1->setSpacing(0);
#endif
	layout1->setColumnStretch(1, 1);
	
	// Text box for the name of particle type.
	StringParameterUI* namePUI = new StringParameterUI(this, PROPERTY_FIELD(BondType::name));
	layout1->addWidget(new QLabel(tr("Name:")), 0, 0);
	layout1->addWidget(namePUI->textBox(), 0, 1);
	
	// Display color parameter.
	ColorParameterUI* colorPUI = new ColorParameterUI(this, PROPERTY_FIELD(BondType::color));
	layout1->addWidget(colorPUI->label(), 1, 0);
	layout1->addWidget(colorPUI->colorPicker(), 1, 1);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
