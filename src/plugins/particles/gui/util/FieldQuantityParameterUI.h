///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2017) Alexander Stukowski
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


#include <plugins/particles/gui/ParticlesGui.h>
#include <gui/properties/ParameterUI.h>
#include "FieldQuantityComboBox.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/**
 * \brief This parameter UI lets the user select a field quantity.
 */
class OVITO_PARTICLES_GUI_EXPORT FieldQuantityParameterUI : public PropertyParameterUI
{
public:

	/// Constructor.
	FieldQuantityParameterUI(QObject* parentEditor, const char* propertyName, bool showComponents = true, bool inputProperty = true);

	/// Constructor.
	FieldQuantityParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField, bool showComponents = true, bool inputProperty = true);
	
	/// Destructor.
	virtual ~FieldQuantityParameterUI();
	
	/// This returns the combo box managed by this ParameterUI.
	QComboBox* comboBox() const { return _comboBox; }
	
	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.  
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;
	
	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;
	
	/// Sets the tooltip text for the combo box widget.
	void setToolTip(const QString& text) const { 
		if(comboBox()) comboBox()->setToolTip(text); 
	}
	
	/// Sets the What's This helper text for the combo box.
	void setWhatsThis(const QString& text) const { 
		if(comboBox()) comboBox()->setWhatsThis(text); 
	}
	
public:
	
	Q_PROPERTY(QComboBox comboBox READ comboBox);
	
public Q_SLOTS:
	
	/// Takes the value entered by the user and stores it in the property field 
	/// this property UI is bound to. 
	void updatePropertyValue();
	
protected:

	/// The combo box of the UI component.
	QPointer<FieldQuantityComboBox> _comboBox;

	/// Controls whether the combo box should display a separate entry for each component of
	/// a quantity.
	bool _showComponents;

	/// Controls whether the combo box should list input or output properties.
	bool _inputProperty;

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


