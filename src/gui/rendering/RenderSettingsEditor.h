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


#include <gui/GUI.h>
#include <gui/properties/PropertiesEditor.h>
#include <core/reference/RefTarget.h>
#include <core/animation/TimeInterval.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * The editor component for the RenderSettings class.
 */
class RenderSettingsEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE RenderSettingsEditor() {}

protected:
	
	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private Q_SLOTS:

	/// Lets the user choose a filename for the output image.
	void onChooseImageFilename();

	/// Is called when the user selects an output size preset from the drop-down list.
	void onSizePresetActivated(int index);
	
	/// Lets the user choose a different plug-in rendering engine.
	void onSwitchRenderer();

private:

	QComboBox* sizePresetsBox;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


