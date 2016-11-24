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

#include <core/Core.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/objects/DataObject.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/viewport/Viewport.h>
#include <core/dataset/DataSetContainer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Core, ObjectNode, SceneNode);
DEFINE_REFERENCE_FIELD(ObjectNode, _dataProvider, "SceneObject", DataObject);
DEFINE_VECTOR_REFERENCE_FIELD(ObjectNode, _displayObjects, "DisplayObjects", DisplayObject);
SET_PROPERTY_FIELD_LABEL(ObjectNode, _dataProvider, "Object");

/******************************************************************************
* Constructor.
******************************************************************************/
ObjectNode::ObjectNode(DataSet* dataset) : SceneNode(dataset)
{
	INIT_PROPERTY_FIELD(ObjectNode::_dataProvider);
	INIT_PROPERTY_FIELD(ObjectNode::_displayObjects);
}

/******************************************************************************
* Destructor.
******************************************************************************/
ObjectNode::~ObjectNode()
{
}

/******************************************************************************
* Evaluates the geometry pipeline of this scene node at the given time.
******************************************************************************/
const PipelineFlowState& ObjectNode::evalPipeline(TimePoint time)
{
	// Check if the caches need to be updated.
	if(_displayCache.stateValidity().contains(time) == false) {
		if(_pipelineCache.stateValidity().contains(time) == false) {
			if(dataProvider()) {

				// Avoid recording the creation of transient objects on the undo stack
				// while evaluating the pipeline.
				UndoSuspender suspendUndo(dataset()->undoStack());

				// Evaluate data flow pipeline and store results in local cache.
				_pipelineCache = dataProvider()->evaluate(time);

				// Update list of active display objects.

				// First discard those display objects which are no longer needed.
				// (Only when we got the final pipeline results.)
				if(_pipelineCache.status().type() != PipelineStatus::Pending) {
					for(int i = displayObjects().size() - 1; i >= 0; i--) {
						DisplayObject* displayObj = displayObjects()[i];
						// Check if the display object is still being referenced by any of the objects
						// that left the pipeline.
						if(std::none_of(_pipelineCache.objects().begin(), _pipelineCache.objects().end(),
								[displayObj](DataObject* obj) { return obj->displayObjects().contains(displayObj); })) {
							_displayObjects.remove(i);
						}
					}
				}

				// Now add any new display objects.
				for(const auto& dataObj : _pipelineCache.objects()) {
					for(DisplayObject* displayObj : dataObj->displayObjects()) {
						OVITO_CHECK_OBJECT_POINTER(displayObj);
						if(displayObjects().contains(displayObj) == false)
							_displayObjects.push_back(displayObj);
					}
				}

				OVITO_ASSERT(_pipelineCache.stateValidity().contains(time));
			}
			else {
				// Reset cache if this node doesn't have a data source.
				invalidatePipelineCache();
				// Discard any display objects as well.
				_displayObjects.clear();
			}
		}

		// Let display objects prepare the data for rendering.
		_displayCache = _pipelineCache;
		for(const auto& dataObj : _displayCache.objects()) {
			for(DisplayObject* displayObj : dataObj->displayObjects()) {
				if(displayObj && displayObj->isEnabled()) {
					displayObj->prepare(time, dataObj, _displayCache);
				}
			}
		}
	}
	else {
		OVITO_ASSERT(_pipelineCache.stateValidity().contains(time));
	}
	return _displayCache;
}

/******************************************************************************
* Renders the node's data.
******************************************************************************/
void ObjectNode::render(TimePoint time, SceneRenderer* renderer)
{
	const PipelineFlowState& state = evalPipeline(time);
	for(const auto& dataObj : state.objects()) {
		for(DisplayObject* displayObj : dataObj->displayObjects()) {
			if(displayObj && displayObj->isEnabled()) {
				displayObj->render(time, dataObj, state, renderer, this);
			}
		}
	}
}

/******************************************************************************
* This method is called when a referenced object has changed.
******************************************************************************/
bool ObjectNode::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == dataProvider()) {
		if(event->type() == ReferenceEvent::TargetChanged || event->type() == ReferenceEvent::PendingStateChanged) {
			invalidatePipelineCache();
		}
		else if(event->type() == ReferenceEvent::TargetDeleted) {
			// Data provider has been deleted -> delete node as well.
			if(!dataset()->undoStack().isUndoingOrRedoing())
				deleteNode();
		}
		else if(event->type() == ReferenceEvent::TitleChanged) {
			notifyDependents(ReferenceEvent::TitleChanged);
		}
	}
	else if(_displayObjects.contains(source)) {
		if(event->type() == ReferenceEvent::TargetChanged || event->type() == ReferenceEvent::PendingStateChanged) {
			// Refresh display cache.
			_displayCache.clear();
			// Update cached bounding box when display settings change.
			invalidateBoundingBox();
		}
	}
	return SceneNode::referenceEvent(source, event);
}

/******************************************************************************
* Gets called when the data object of the node has been replaced.
******************************************************************************/
void ObjectNode::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(ObjectNode::_dataProvider)) {
		invalidatePipelineCache();

		// When the data object is being replaced, the pending state of the node might change.
		// Even though we don't know for sure if the state has really changed, we send a notification event here.
		notifyDependents(ReferenceEvent::PendingStateChanged);
	}

	SceneNode::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Returns the bounding box of the object node in local coordinates.
******************************************************************************/
Box3 ObjectNode::localBoundingBox(TimePoint time)
{
	Box3 bb;
	const PipelineFlowState& state = evalPipeline(time);

	// Compute bounding boxes of data objects.
	for(DataObject* dataObj : state.objects()) {
		for(DisplayObject* displayObj : dataObj->displayObjects()) {
			if(displayObj && displayObj->isEnabled())
				bb.addBox(displayObj->boundingBox(time, dataObj, this, state));
		}
	}

	return bb;
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void ObjectNode::saveToStream(ObjectSaveStream& stream)
{
	SceneNode::saveToStream(stream);
	stream.beginChunk(0x01);
	// For future use...
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void ObjectNode::loadFromStream(ObjectLoadStream& stream)
{
	SceneNode::loadFromStream(stream);
	stream.expectChunk(0x01);
	// For future use...
	stream.closeChunk();
}

/******************************************************************************
* Returns the title of this object.
******************************************************************************/
QString ObjectNode::objectTitle()
{
	// If a name has been assigned to this node, return it as the node's display title.
	if(!name().isEmpty())
		return name();

	// Otherwise, use the title of the node's data source object.
	if(DataObject* sourceObj = sourceObject())
		return sourceObj->objectTitle();

	// Fall back to default behavior.
	return SceneNode::objectTitle();
}

/******************************************************************************
* Applies a modifier by appending it to the end of the node's modification
* pipeline.
******************************************************************************/
void ObjectNode::applyModifier(Modifier* modifier)
{
	if(!dataProvider())
		throwException("Cannot insert modifier into a modification pipeline without a data source.");

	PipelineObject* pipelineObj = dynamic_object_cast<PipelineObject>(dataProvider());
	if(!pipelineObj) {
		OORef<PipelineObject> p = new PipelineObject(dataset());
		p->setSourceObject(dataProvider());
		setDataProvider(p);
		pipelineObj = p;
	}
	pipelineObj->insertModifier(pipelineObj->modifierApplications().size(), modifier);
}

/******************************************************************************
* Returns the modification pipeline source object, i.e., the input of this
* node's modification pipeline.
******************************************************************************/
DataObject* ObjectNode::sourceObject() const
{
	DataObject* obj = dataProvider();
	while(obj) {
		if(PipelineObject* pipeline = dynamic_object_cast<PipelineObject>(obj))
			obj = pipeline->sourceObject();
		else
			break;
	}
	return obj;
}

/******************************************************************************
* Sets the data source of this node's pipeline, i.e., the object that provides the
* input data entering the pipeline.
******************************************************************************/
void ObjectNode::setSourceObject(DataObject* sourceObject)
{
	PipelineObject* pipeline = dynamic_object_cast<PipelineObject>(dataProvider());
	if(!pipeline) {
		setDataProvider(sourceObject);
	}
	else {
		for(;;) {
			if(PipelineObject* pipeline2 = dynamic_object_cast<PipelineObject>(pipeline->sourceObject()))
				pipeline = pipeline2;
			else
				break;
		}
		pipeline->setSourceObject(sourceObject);
	}
	OVITO_ASSERT(this->sourceObject() == sourceObject);
}

/******************************************************************************
* This function blocks execution until the node's modification
* pipeline has been fully evaluated.
******************************************************************************/
bool ObjectNode::waitUntilReady(TimePoint time, const QString& message, AbstractProgressDisplay* progressDisplay)
{
	return dataset()->container()->waitUntil([this, time]() {
		return evalPipeline(time).status().type() != PipelineStatus::Pending;
	}, message, progressDisplay);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
