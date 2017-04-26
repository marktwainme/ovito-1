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

#include <gui/GUI.h>
#include <core/scene/objects/DataObject.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/scene/pipeline/Modifier.h>
#include "ModificationListItem.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ModificationListItem, RefMaker);
DEFINE_FLAGS_REFERENCE_FIELD(ModificationListItem, object, "Object", RefTarget, PROPERTY_FIELD_NO_UNDO|PROPERTY_FIELD_WEAK_REF|PROPERTY_FIELD_NO_CHANGE_MESSAGE);
DEFINE_FLAGS_VECTOR_REFERENCE_FIELD(ModificationListItem, modifierApplications, "ModifierApplications", ModifierApplication, PROPERTY_FIELD_NO_UNDO|PROPERTY_FIELD_WEAK_REF|PROPERTY_FIELD_NO_CHANGE_MESSAGE);

/******************************************************************************
* Constructor.
******************************************************************************/
ModificationListItem::ModificationListItem(RefTarget* object, ModificationListItem* parent, const QString& title) :
	_parent(parent), _title(title)
{
	INIT_PROPERTY_FIELD(object);
	INIT_PROPERTY_FIELD(modifierApplications);

	_object = object;
}

/******************************************************************************
* This method is called when the object presented by the modifier
* list item generates a message.
******************************************************************************/
bool ModificationListItem::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// The modifier stack list must be updated if a modifier has been added or removed
	// from a PipelineObject.
	if((event->type() == ReferenceEvent::ReferenceAdded || event->type() == ReferenceEvent::ReferenceRemoved || event->type() == ReferenceEvent::ReferenceChanged)
		&& source == object() && dynamic_object_cast<PipelineObject>(object()))
	{
		Q_EMIT subitemsChanged(this);
	}
	/// Update item if an object has been enabled or disabled.
	else if(event->type() == ReferenceEvent::TargetEnabledOrDisabled && source == object() && event->sender() == object()) {
		Q_EMIT itemChanged(this);
	}
	/// Update an entry if the evaluation status of the modifier has changed.
	else if(event->type() == ReferenceEvent::ObjectStatusChanged || event->type() == ReferenceEvent::TitleChanged) {
		Q_EMIT itemChanged(this);
	}
	/// If the list of sub-objects changes for one of the entries, we need
	/// to update everything.
	else if(event->type() == ReferenceEvent::SubobjectListChanged && source == object() && event->sender() == object()) {
		Q_EMIT subitemsChanged(this);
	}

	return RefMaker::referenceEvent(source, event);
}

/******************************************************************************
* Returns the status of the object represented by the list item.
******************************************************************************/
PipelineStatus ModificationListItem::status() const
{
	if(Modifier* modifier = dynamic_object_cast<Modifier>(object())) {
		return modifier->status();
	}
	else if(DataObject* dataObj = dynamic_object_cast<DataObject>(object())) {
		return dataObj->status();
	}
	else if(DisplayObject* displayObj = dynamic_object_cast<DisplayObject>(object())) {
		return displayObj->status();
	}
	else {
		return PipelineStatus();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
