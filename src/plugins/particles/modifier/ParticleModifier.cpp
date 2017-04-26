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

#include <plugins/particles/Particles.h>
#include <core/scene/pipeline/ModifierApplication.h>
#include <plugins/particles/objects/ParticleDisplay.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/objects/BondPropertyObject.h>
#include "ParticleModifier.h"

#include <QtConcurrent>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ParticleModifier, Modifier);

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus ParticleModifier::modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// This method is not re-entrant. If this method is called while the modifier is already being
	// evaluated then we are not able to process the request.
	if(!_input.isEmpty())
		return PipelineStatus(PipelineStatus::Error, tr("Cannot handle re-entrant modifier calls."));

	// Prepare internal fields.
	_input = state;
	_output = state;
	_modApp = modApp;
	PipelineStatus status;

	try {
		ParticlePropertyObject* posProperty = inputStandardProperty(ParticleProperty::PositionProperty);
		if(!posProperty && !this->isApplicableTo(_input))
			throwException(tr("This modifier cannot be evaluated because the input does not contain any particles."));
		_outputParticleCount = _inputParticleCount = (posProperty != nullptr) ? posProperty->size() : 0;

		// Verify input, make sure array length of particle properties is consistent.
		for(DataObject* obj : state.objects()) {
			if(ParticlePropertyObject* p = dynamic_object_cast<ParticlePropertyObject>(obj)) {
				if(p->size() != _inputParticleCount)
					throwException(tr("Detected invalid modifier input. Data array size is not the same for all particle properties."));
			}
		}

		// Determine number of input bonds.
		BondsObject* bondsObj = state.findObject<BondsObject>();
		_outputBondCount = _inputBondCount = (bondsObj != nullptr) ? bondsObj->size() : 0;

		// Let the derived class do the actual work.
		TimeInterval validityInterval = state.stateValidity();
		status = modifyParticles(time, validityInterval);

		// Put result into geometry pipeline.
		state = _output;
		state.intersectStateValidity(validityInterval);
	}
	catch(const Exception& ex) {
		// Transfer exception message to evaluation status.
		status = PipelineStatus(PipelineStatus::Error, ex.messages().join('\n'));
		state.intersectStateValidity(TimeInterval(time));
	}
	catch(const std::bad_alloc&) {
		status = PipelineStatus(PipelineStatus::Error, tr("Not enough memory to execute this modifier."));
		state.intersectStateValidity(TimeInterval(time));
	}
	catch(const PipelineStatus& thrown_status) {
		// Transfer exception message to evaluation status.
		status = thrown_status;
		state.intersectStateValidity(TimeInterval(time));
	}
	setStatus(status);

	// Cleanup
	_cloneHelper.reset();
	_input.clear();
	_output.clear();
	_modApp = nullptr;

	return status;
}

/******************************************************************************
* Sets the status returned by the modifier and generates a
* ReferenceEvent::ObjectStatusChanged event.
******************************************************************************/
void ParticleModifier::setStatus(const PipelineStatus& status)
{
	if(status == _modifierStatus) return;
	_modifierStatus = status;
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ParticleModifier::isApplicableTo(const PipelineFlowState& input)
{
	return (input.findObject<ParticlePropertyObject>() != nullptr);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ParticleModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	Modifier::propertyChanged(field);

	// Clear status when modifier is disabled.
	if(field == PROPERTY_FIELD(Modifier::isEnabled) && !isEnabled())
		setStatus(PipelineStatus(PipelineStatus::Success, tr("Modifier is currently disabled.")));
}

/******************************************************************************
* Returns a standard particle property from the input state.
******************************************************************************/
ParticlePropertyObject* ParticleModifier::inputStandardProperty(ParticleProperty::Type which) const
{
	OVITO_ASSERT(which != ParticleProperty::UserProperty);
	return ParticlePropertyObject::findInState(_input, which);
}

/******************************************************************************
* Returns a standard bond property from the input state.
******************************************************************************/
BondPropertyObject* ParticleModifier::inputStandardBondProperty(BondProperty::Type which) const
{
	OVITO_ASSERT(which != BondProperty::UserProperty);
	return BondPropertyObject::findInState(_input, which);
}

/******************************************************************************
* Returns the property with the given identifier from the input object.
******************************************************************************/
ParticlePropertyObject* ParticleModifier::expectCustomProperty(const QString& propertyName, int dataType, size_t componentCount) const
{
	for(DataObject* o : _input.objects()) {
		ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
		if(property && property->name() == propertyName) {
			if(property->dataType() != dataType)
				throwException(tr("The modifier cannot be evaluated because the particle property '%1' does not have the required data type.").arg(property->name()));
			if(property->componentCount() != componentCount)
				throwException(tr("The modifier cannot be evaluated because the particle property '%1' does not have the required number of components per particle.").arg(property->name()));

			OVITO_ASSERT(property->size() == _inputParticleCount);
			return property;
		}
	}
	throwException(tr("The modifier cannot be evaluated because the input does not contain the required particle property (name: %1).").arg(propertyName));
}

/******************************************************************************
* Returns the given standard particle property from the input object.
* The returned property may not be modified. If they input object does
* not contain the standard property then an exception is thrown.
******************************************************************************/
ParticlePropertyObject* ParticleModifier::expectStandardProperty(ParticleProperty::Type which) const
{
	ParticlePropertyObject* property = inputStandardProperty(which);
	if(!property) {
		if(which == ParticleProperty::SelectionProperty)
			throwException(tr("No particle selection has been defined. Please select some particles first."));
		else
			throwException(tr("The modifier cannot be evaluated because the input does not contain the required particle property '%1'.").arg(ParticleProperty::standardPropertyName(which)));
	}
	return property;
}

/******************************************************************************
* Returns the given standard bond property from the input object.
* The returned property may not be modified. If they input object does
* not contain the standard property then an exception is thrown.
******************************************************************************/
BondPropertyObject* ParticleModifier::expectStandardBondProperty(BondProperty::Type which) const
{
	BondPropertyObject* property = inputStandardBondProperty(which);
	if(!property) {
		if(which == BondProperty::SelectionProperty)
			throwException(tr("No bond selection has been defined. Please select some bonds first."));
		else
			throwException(tr("The modifier cannot be evaluated because the input does not contain the required bond property '%1'.").arg(BondProperty::standardPropertyName(which)));
	}
	return property;
}

/******************************************************************************
* Returns the input simulation cell.
******************************************************************************/
SimulationCellObject* ParticleModifier::expectSimulationCell() const
{
	SimulationCellObject* cell = _input.findObject<SimulationCellObject>();
	if(!cell)
		throwException(tr("The modifier cannot be evaluated because the input does not contain a simulation cell."));
	return cell;
}

/******************************************************************************
* Returns the input bonds.
******************************************************************************/
BondsObject* ParticleModifier::expectBonds() const
{
	BondsObject* bondsObj = _input.findObject<BondsObject>();
	if(!bondsObj)
		throwException(tr("The modifier cannot be evaluated because the input does not contain any bonds."));
	return bondsObj;
}

/******************************************************************************
* Creates a standard particle in the modifier's output.
* If the particle property already exists in the input, its contents are copied to the
* output property by this method.
******************************************************************************/
ParticlePropertyObject* ParticleModifier::outputStandardProperty(ParticleProperty::Type which, bool initializeMemory)
{
	// Check if property already exists in the input.
	OORef<ParticlePropertyObject> inputProperty = inputStandardProperty(which);

	// Check if property already exists in the output.
	OORef<ParticlePropertyObject> outputProperty = ParticlePropertyObject::findInState(_output, which);

	if(outputProperty) {
		// Is the existing output property still a shallow copy of the input?
		if(outputProperty == inputProperty) {
			// Make a real copy of the property, which may be modified.
			outputProperty = cloneHelper()->cloneObject(inputProperty, false);
			_output.replaceObject(inputProperty, outputProperty);

			// Create a new storage buffer to avoid copying the contents of the old one when
			// a deep copy is made on the first write access.
			if(!initializeMemory) {
				outputProperty->setStorage(new ParticleProperty(outputProperty->size(), which, 0, false));
			}
		}
	}
	else {
		// Create a new particle property in the output.
		outputProperty = ParticlePropertyObject::createStandardProperty(dataset(), _outputParticleCount, which, 0, initializeMemory);
		_output.addObject(outputProperty);
	}

	OVITO_ASSERT(outputProperty->size() == outputParticleCount());
	return outputProperty;
}

/******************************************************************************
* Creates a standard particle in the modifier's output and sets its content.
******************************************************************************/
ParticlePropertyObject* ParticleModifier::outputStandardProperty(ParticleProperty* storage)
{
	OVITO_CHECK_POINTER(storage);
	OVITO_ASSERT(storage->type() != ParticleProperty::UserProperty);
	OVITO_ASSERT(storage->size() == outputParticleCount());

	// Check if property already exists in the input.
	OORef<ParticlePropertyObject> inputProperty = inputStandardProperty(storage->type());

	// Check if property already exists in the output.
	OORef<ParticlePropertyObject> outputProperty = ParticlePropertyObject::findInState(_output, storage->type());

	if(outputProperty) {
		// Is the existing output property still a shallow copy of the input?
		if(outputProperty == inputProperty) {
			// Make a real copy of the property, which may be modified.
			outputProperty = cloneHelper()->cloneObject(inputProperty, false);
			_output.replaceObject(inputProperty, outputProperty);
		}
		OVITO_ASSERT(storage->size() == outputProperty->size());
		OVITO_ASSERT(storage->stride() == outputProperty->stride());
		outputProperty->setStorage(storage);
	}
	else {
		// Create a new particle property in the output.
		outputProperty = ParticlePropertyObject::createFromStorage(dataset(), storage);
		_output.addObject(outputProperty);
	}

	OVITO_ASSERT(outputProperty->size() == outputParticleCount());
	return outputProperty;
}

/******************************************************************************
* Creates a standard bond in the modifier's output.
* If the bond property already exists in the input, its contents are copied to the
* output property by this method.
******************************************************************************/
BondPropertyObject* ParticleModifier::outputStandardBondProperty(BondProperty::Type which, bool initializeMemory)
{
	// Check if property already exists in the input.
	OORef<BondPropertyObject> inputProperty = inputStandardBondProperty(which);

	// Check if property already exists in the output.
	OORef<BondPropertyObject> outputProperty = BondPropertyObject::findInState(_output, which);

	if(outputProperty) {
		// Is the existing output property still a shallow copy of the input?
		if(outputProperty == inputProperty) {
			// Make a real copy of the property, which may be modified.
			outputProperty = cloneHelper()->cloneObject(inputProperty, false);
			_output.replaceObject(inputProperty, outputProperty);

			// Create a new storage buffer to avoid copying the contents of the old one when
			// a deep copy is made on the first write access.
			if(!initializeMemory) {
				outputProperty->setStorage(new BondProperty(outputProperty->size(), which, 0, false));
			}
		}
	}
	else {
		// Create a new bond property in the output.
		outputProperty = BondPropertyObject::createStandardProperty(dataset(), outputBondCount(), which, 0, initializeMemory);
		_output.addObject(outputProperty);
	}

	OVITO_ASSERT(outputProperty->size() == outputBondCount());
	return outputProperty;
}

/******************************************************************************
* Creates a standard bond in the modifier's output and sets its content.
******************************************************************************/
BondPropertyObject* ParticleModifier::outputStandardBondProperty(BondProperty* storage)
{
	OVITO_CHECK_POINTER(storage);
	OVITO_ASSERT(storage->type() != BondProperty::UserProperty);
	OVITO_ASSERT(storage->size() == outputBondCount());

	// Check if property already exists in the input.
	OORef<BondPropertyObject> inputProperty = inputStandardBondProperty(storage->type());

	// Check if property already exists in the output.
	OORef<BondPropertyObject> outputProperty = BondPropertyObject::findInState(_output, storage->type());

	if(outputProperty) {
		// Is the existing output property still a shallow copy of the input?
		if(outputProperty == inputProperty) {
			// Make a real copy of the property, which may be modified.
			outputProperty = cloneHelper()->cloneObject(inputProperty, false);
			_output.replaceObject(inputProperty, outputProperty);
		}
		OVITO_ASSERT(storage->size() == outputProperty->size());
		OVITO_ASSERT(storage->stride() == outputProperty->stride());
		outputProperty->setStorage(storage);
	}
	else {
		// Create a new bond property in the output.
		outputProperty = BondPropertyObject::createFromStorage(dataset(), storage);
		_output.addObject(outputProperty);
	}

	OVITO_ASSERT(outputProperty->size() == outputBondCount());
	return outputProperty;
}

/******************************************************************************
* Creates a custom particle property in the modifier's output.
******************************************************************************/
ParticlePropertyObject* ParticleModifier::outputCustomProperty(const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory)
{
	// Check if property already exists in the input.
	OORef<ParticlePropertyObject> inputProperty;
	for(DataObject* o : input().objects()) {
		ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
		if(property && property->type() == ParticleProperty::UserProperty && property->name() == name) {
			inputProperty = property;
			if(property->dataType() != dataType)
				throwException(tr("Existing property '%1' has a different data type.").arg(name));
			if(property->componentCount() != componentCount)
				throwException(tr("Existing property '%1' has a different number of components.").arg(name));
			if(stride != 0 && property->stride() != stride)
				throwException(tr("Existing property '%1' has a different stride.").arg(name));
			break;
		}
	}

	// Check if property already exists in the output.
	OORef<ParticlePropertyObject> outputProperty;
	for(DataObject* o : output().objects()) {
		ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
		if(property && property->type() == ParticleProperty::UserProperty && property->name() == name) {
			outputProperty = property;
			OVITO_ASSERT(property->dataType() == dataType);
			OVITO_ASSERT(property->componentCount() == componentCount);
			break;
		}
	}

	if(outputProperty) {
		// Is the existing output property still a shallow copy of the input?
		if(outputProperty == inputProperty) {
			// Make a real copy of the property, which may be modified.
			outputProperty = cloneHelper()->cloneObject(inputProperty, false);
			_output.replaceObject(inputProperty, outputProperty);
		}
	}
	else {
		// Create a new particle property in the output.
		outputProperty = ParticlePropertyObject::createUserProperty(dataset(), _outputParticleCount, dataType, componentCount, stride, name, initializeMemory);
		_output.addObject(outputProperty);
	}

	OVITO_ASSERT(outputProperty->size() == outputParticleCount());
	return outputProperty;
}

/******************************************************************************
* Creates a custom particle property in the modifier's output and sets its content.
******************************************************************************/
ParticlePropertyObject* ParticleModifier::outputCustomProperty(ParticleProperty* storage)
{
	OVITO_CHECK_POINTER(storage);
	OVITO_ASSERT(storage->type() == ParticleProperty::UserProperty);
	OVITO_ASSERT(storage->size() == outputParticleCount());

	// Check if property already exists in the input.
	OORef<ParticlePropertyObject> inputProperty;
	for(DataObject* o : input().objects()) {
		ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
		if(property && property->type() == ParticleProperty::UserProperty && property->name() == storage->name()) {
			inputProperty = property;
			if(property->dataType() != storage->dataType() || property->dataTypeSize() != storage->dataTypeSize())
				throwException(tr("Existing property '%1' has a different data type.").arg(property->name()));
			if(property->componentCount() != storage->componentCount())
				throwException(tr("Existing property '%1' has a different number of components.").arg(property->name()));
			break;
		}
	}

	// Check if property already exists in the output.
	OORef<ParticlePropertyObject> outputProperty;
	for(DataObject* o : output().objects()) {
		ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
		if(property && property->type() == ParticleProperty::UserProperty && property->name() == storage->name()) {
			outputProperty = property;
			OVITO_ASSERT(property->dataType() == storage->dataType());
			OVITO_ASSERT(property->componentCount() == storage->componentCount());
			break;
		}
	}

	if(outputProperty) {
		// Is the existing output property still a shallow copy of the input?
		if(outputProperty == inputProperty) {
			// Make a real copy of the property, which may be modified.
			outputProperty = cloneHelper()->cloneObject(inputProperty, false);
			_output.replaceObject(inputProperty, outputProperty);
		}
		outputProperty->setStorage(storage);
	}
	else {
		// Create a new particle property in the output.
		outputProperty = ParticlePropertyObject::createFromStorage(dataset(), storage);
		_output.addObject(outputProperty);
	}

	OVITO_ASSERT(outputProperty->size() == outputParticleCount());
	return outputProperty;
}

/******************************************************************************
* Creates a custom bond property in the modifier's output.
******************************************************************************/
BondPropertyObject* ParticleModifier::outputCustomBondProperty(const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory)
{
	// Check if property already exists in the input.
	OORef<BondPropertyObject> inputProperty;
	for(DataObject* o : input().objects()) {
		BondPropertyObject* property = dynamic_object_cast<BondPropertyObject>(o);
		if(property && property->type() == BondProperty::UserProperty && property->name() == name) {
			inputProperty = property;
			if(property->dataType() != dataType)
				throwException(tr("Existing bond property '%1' has a different data type.").arg(name));
			if(property->componentCount() != componentCount)
				throwException(tr("Existing bond property '%1' has a different number of components.").arg(name));
			if(property->stride() != stride)
				throwException(tr("Existing bond property '%1' has a different stride.").arg(name));
			break;
		}
	}

	// Check if property already exists in the output.
	OORef<BondPropertyObject> outputProperty;
	for(DataObject* o : output().objects()) {
		BondPropertyObject* property = dynamic_object_cast<BondPropertyObject>(o);
		if(property && property->type() == BondProperty::UserProperty && property->name() == name) {
			outputProperty = property;
			OVITO_ASSERT(property->dataType() == dataType);
			OVITO_ASSERT(property->componentCount() == componentCount);
			break;
		}
	}

	if(outputProperty) {
		// Is the existing output property still a shallow copy of the input?
		if(outputProperty == inputProperty) {
			// Make a real copy of the property, which may be modified.
			outputProperty = cloneHelper()->cloneObject(inputProperty, false);
			_output.replaceObject(inputProperty, outputProperty);
		}
	}
	else {
		// Create a new bond property in the output.
		outputProperty = BondPropertyObject::createUserProperty(dataset(), _outputBondCount, dataType, componentCount, stride, name, initializeMemory);
		_output.addObject(outputProperty);
	}

	OVITO_ASSERT(outputProperty->size() == outputBondCount());
	return outputProperty;
}

/******************************************************************************
* Creates a custom bond property in the modifier's output and sets its content.
******************************************************************************/
BondPropertyObject* ParticleModifier::outputCustomBondProperty(BondProperty* storage)
{
	OVITO_CHECK_POINTER(storage);
	OVITO_ASSERT(storage->type() == BondProperty::UserProperty);
	OVITO_ASSERT(storage->size() == outputBondCount());

	// Check if property already exists in the input.
	OORef<BondPropertyObject> inputProperty;
	for(DataObject* o : input().objects()) {
		BondPropertyObject* property = dynamic_object_cast<BondPropertyObject>(o);
		if(property && property->type() == BondProperty::UserProperty && property->name() == storage->name()) {
			inputProperty = property;
			if(property->dataType() != storage->dataType() || property->dataTypeSize() != storage->dataTypeSize())
				throwException(tr("Existing bond property '%1' has a different data type.").arg(property->name()));
			if(property->componentCount() != storage->componentCount())
				throwException(tr("Existing bond property '%1' has a different number of components.").arg(property->name()));
			break;
		}
	}

	// Check if property already exists in the output.
	OORef<BondPropertyObject> outputProperty;
	for(DataObject* o : output().objects()) {
		BondPropertyObject* property = dynamic_object_cast<BondPropertyObject>(o);
		if(property && property->type() == BondProperty::UserProperty && property->name() == storage->name()) {
			outputProperty = property;
			OVITO_ASSERT(property->dataType() == storage->dataType());
			OVITO_ASSERT(property->componentCount() == storage->componentCount());
			break;
		}
	}

	if(outputProperty) {
		// Is the existing output property still a shallow copy of the input?
		if(outputProperty == inputProperty) {
			// Make a real copy of the property, which may be modified.
			outputProperty = cloneHelper()->cloneObject(inputProperty, false);
			_output.replaceObject(inputProperty, outputProperty);
		}
		outputProperty->setStorage(storage);
	}
	else {
		// Create a new particle property in the output.
		outputProperty = BondPropertyObject::createFromStorage(dataset(), storage);
		_output.addObject(outputProperty);
	}

	OVITO_ASSERT(outputProperty->size() == outputBondCount());
	return outputProperty;
}

/******************************************************************************
* Removes the given particle property from the modifier's output.
******************************************************************************/
void ParticleModifier::removeOutputProperty(ParticlePropertyObject* property)
{
	output().removeObject(property);
}

/******************************************************************************
* Returns the modifier's output simulation cell.
******************************************************************************/
SimulationCellObject* ParticleModifier::outputSimulationCell()
{
	SimulationCellObject* inputCell = expectSimulationCell();

	// Check if cell already exists in the output.
	OORef<SimulationCellObject> outputCell = output().findObject<SimulationCellObject>();
	if(outputCell) {
		// Is the existing output property still a shallow copy of the input?
		if(outputCell == inputCell) {
			// Make a real copy of the property, which may be modified.
			outputCell = cloneHelper()->cloneObject(inputCell, false);
			_output.replaceObject(inputCell, outputCell);
		}
	}
	else {
		// Create a new particle property in the output.
		outputCell = new SimulationCellObject(dataset());
		_output.addObject(outputCell);
	}

	return outputCell;
}

/******************************************************************************
* Deletes the particles given by the bit-mask.
* Returns the number of remaining particles.
******************************************************************************/
size_t ParticleModifier::deleteParticles(const boost::dynamic_bitset<>& mask, size_t deleteCount)
{
	OVITO_ASSERT(mask.size() == inputParticleCount());
	OVITO_ASSERT(mask.count() == deleteCount);
	OVITO_ASSERT(outputParticleCount() == inputParticleCount());
	OVITO_ASSERT(outputBondCount() == inputBondCount());

	size_t oldParticleCount = inputParticleCount();
	size_t newParticleCount = oldParticleCount - deleteCount;
	if(newParticleCount == oldParticleCount)
		return oldParticleCount;	// Nothing to delete.

	_outputParticleCount = newParticleCount;

	QVector<QPair<OORef<ParticlePropertyObject>, OORef<ParticlePropertyObject>>> oldToNewMap;

	// Create output particle properties.
	for(DataObject* outobj : _output.objects()) {
		OORef<ParticlePropertyObject> originalOutputProperty = dynamic_object_cast<ParticlePropertyObject>(outobj);
		if(!originalOutputProperty)
			continue;

		OVITO_ASSERT(originalOutputProperty->size() == oldParticleCount);

		// Create copy.
		OORef<ParticlePropertyObject> newProperty = cloneHelper()->cloneObject(originalOutputProperty, false);
		newProperty->resize(newParticleCount, false);

		// Replace original property with the filtered one.
		_output.replaceObject(originalOutputProperty, newProperty);

		oldToNewMap.push_back(qMakePair(originalOutputProperty, newProperty));
	}

	// Transfer and filter per-particle data elements.
	QtConcurrent::blockingMap(oldToNewMap, [&mask](const QPair<OORef<ParticlePropertyObject>, OORef<ParticlePropertyObject>>& pair) {
		pair.second->filterCopy(pair.first, mask);
	});

	// Delete dangling bonds, i.e. those that are incident on a deleted particle.
	boost::dynamic_bitset<> deletedBondsMask;
	size_t newBondCount = 0;
	for(const auto& outobj : _output.objects()) {
		BondsObject* originalBondsObject = dynamic_object_cast<BondsObject>(outobj);
		if(!originalBondsObject)
			continue;

		// Create copy of bonds object.
		OORef<BondsObject> newBondsObject = cloneHelper()->cloneObject(originalBondsObject, false);
		// Remap particle indices of stored bonds and remove dangling bonds.
		deletedBondsMask.resize(newBondsObject->storage()->size());
		newBondCount = newBondsObject->particlesDeleted(mask, deletedBondsMask);

		// Replace original bonds object with the filtered one.
		_output.replaceObject(originalBondsObject, newBondsObject);
	}
	_outputBondCount = newBondCount;

	// Update bond properties.
	for(const auto& outobj : output().objects()) {
		BondPropertyObject* originalBondPropertyObject = dynamic_object_cast<BondPropertyObject>(outobj);
		if(!originalBondPropertyObject)
			continue;
		if(originalBondPropertyObject->size() != deletedBondsMask.size())
			continue;

		// Create copy and remove elements belonging to deleted bonds.
		OORef<BondPropertyObject> newBondPropertyObject = cloneHelper()->cloneObject(originalBondPropertyObject, false);
		newBondPropertyObject->resize(newBondCount, false);
		newBondPropertyObject->filterCopy(originalBondPropertyObject, deletedBondsMask);

		// Replace original bond property object with the filtered one.
		output().replaceObject(originalBondPropertyObject, newBondPropertyObject);
	}

	return newParticleCount;
}

/******************************************************************************
* Adds a set of new bonds to the system.
******************************************************************************/
BondsObject* ParticleModifier::addBonds(BondsStorage* newBonds, BondsDisplay* bondsDisplay, const std::vector<BondProperty*>& bondProperties)
{
	OVITO_ASSERT(newBonds != nullptr);

	// Check if there is an existing bonds object coming from upstream.
	OORef<BondsObject> bondsObj = output().findObject<BondsObject>();
	if(!bondsObj) {
		OVITO_ASSERT(outputBondCount() == 0);

		// Create a new output data object.
		bondsObj = new BondsObject(dataset(), newBonds);
		if(bondsDisplay)
			bondsObj->setDisplayObject(bondsDisplay);

		// Insert output object into the pipeline.
		output().addObject(bondsObj);
		_outputBondCount = newBonds->size();

		// Insert bond properties.
		for(BondProperty* bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds->size());
			if(bprop->type() != BondProperty::UserProperty)
				outputStandardBondProperty(bprop);
			else
				outputCustomBondProperty(bprop);
		}

		return bondsObj;
	}
	else {

		// Duplicate the existing bonds object and insert the newly created bonds into it.
		OORef<BondsObject> bondsObjCopy = cloneHelper()->cloneObject(bondsObj, false);
		BondsStorage* bonds = bondsObjCopy->modifiableStorage();
		OVITO_ASSERT(bonds != nullptr);

		// This is needed to determine which bonds already exist.
		ParticleBondMap bondMap(*bonds);

		// Add bonds one by one.
		size_t originalBondCount = bonds->size();
		std::vector<int> mapping(newBonds->size());
		for(size_t bondIndex = 0; bondIndex < newBonds->size(); bondIndex++) {
			// Check if there is already a bond like this.
			const Bond& bond = (*newBonds)[bondIndex];
			auto existingBondIndex = bondMap.findBond(bond);
			if(existingBondIndex == bondMap.endOfListValue()) {
				// Create new bond.
				bonds->push_back(bond);
				mapping[bondIndex] = _outputBondCount;
				_outputBondCount++;
			}
			else {
				mapping[bondIndex] = existingBondIndex;
			}
		}

		// Replace original bonds object with the extended one.
		output().replaceObject(bondsObj, bondsObjCopy);

		// Extend existing bond property arrays.
		for(const auto& outobj : output().objects()) {
			BondPropertyObject* originalBondPropertyObject = dynamic_object_cast<BondPropertyObject>(outobj);
			if(!originalBondPropertyObject || originalBondPropertyObject->size() != originalBondCount)
				continue;

			// Create a modifiable copy.
			OORef<BondPropertyObject> newBondPropertyObject = cloneHelper()->cloneObject(originalBondPropertyObject, false);

			// Extend array.
			newBondPropertyObject->resize(outputBondCount(), true);

			// Replace bond property in pipeline flow state.
			output().replaceObject(originalBondPropertyObject, newBondPropertyObject);
		}

		// Insert new bond properties.
		for(BondProperty* bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds->size());

			OORef<BondPropertyObject> propertyObject;

			if(bprop->type() != BondProperty::UserProperty) {
				propertyObject = BondPropertyObject::findInState(output(), bprop->type());
				if(!propertyObject)
					propertyObject = outputStandardBondProperty(bprop->type(), true);
			}
			else {
				propertyObject = outputCustomBondProperty(bprop->name(), bprop->dataType(), bprop->componentCount(), bprop->stride(), true);
			}

			// Copy bond property data.
			propertyObject->modifiableStorage()->mappedCopy(*bprop, mapping);
		}

		return bondsObjCopy;
	}
}

/******************************************************************************
* Returns a vector with the input particles colors.
******************************************************************************/
std::vector<Color> ParticleModifier::inputParticleColors(TimePoint time, TimeInterval& validityInterval)
{
	std::vector<Color> colors(inputParticleCount());

	// Obtain the particle display object.
	ParticlePropertyObject* positionProperty = inputStandardProperty(ParticleProperty::PositionProperty);
	if(positionProperty) {
		for(DisplayObject* displayObj : positionProperty->displayObjects()) {
			if(ParticleDisplay* particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj)) {

				// Query particle colors from display object.
				particleDisplay->particleColors(colors,
						inputStandardProperty(ParticleProperty::ColorProperty),
						dynamic_object_cast<ParticleTypeProperty>(inputStandardProperty(ParticleProperty::ParticleTypeProperty)));

				return colors;
			}
		}
	}

	std::fill(colors.begin(), colors.end(), Color(1,1,1));
	return colors;
}

/******************************************************************************
* Returns a vector with the input bond colors.
******************************************************************************/
std::vector<Color> ParticleModifier::inputBondColors(TimePoint time, TimeInterval& validityInterval)
{
	std::vector<Color> colors(inputBondCount());

	// Obtain the bond display object.
	BondsObject* bondObj = input().findObject<BondsObject>();
	if(bondObj) {
		for(DisplayObject* displayObj : bondObj->displayObjects()) {
			if(BondsDisplay* bondsDisplay = dynamic_object_cast<BondsDisplay>(displayObj)) {

				// Obtain the particle display object.
				ParticleDisplay* particleDisplay = nullptr;
				if(ParticlePropertyObject* positionProperty = inputStandardProperty(ParticleProperty::PositionProperty)) {
					for(DisplayObject* displayObj : positionProperty->displayObjects()) {
						particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj);
						if(particleDisplay) break;
					}
				}

				// Query bond colors from display object.
				bondsDisplay->bondColors(colors,
						inputParticleCount(), bondObj,
						inputStandardBondProperty(BondProperty::ColorProperty),
						dynamic_object_cast<BondTypeProperty>(inputStandardBondProperty(BondProperty::BondTypeProperty)),
						inputStandardBondProperty(BondProperty::SelectionProperty),
						particleDisplay, inputStandardProperty(ParticleProperty::ColorProperty),
						dynamic_object_cast<ParticleTypeProperty>(inputStandardProperty(ParticleProperty::ParticleTypeProperty)));

				return colors;
			}
		}
	}

	std::fill(colors.begin(), colors.end(), Color(1,1,1));
	return colors;
}

/******************************************************************************
* Returns a vector with the input particles radii.
******************************************************************************/
std::vector<FloatType> ParticleModifier::inputParticleRadii(TimePoint time, TimeInterval& validityInterval)
{
	std::vector<FloatType> radii(inputParticleCount());

	// Obtain the particle display object.
	ParticlePropertyObject* positionProperty = inputStandardProperty(ParticleProperty::PositionProperty);
	if(positionProperty) {
		for(DisplayObject* displayObj : positionProperty->displayObjects()) {
			if(ParticleDisplay* particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj)) {

				// Query particle radii from display object.
				particleDisplay->particleRadii(radii,
						inputStandardProperty(ParticleProperty::RadiusProperty),
						dynamic_object_cast<ParticleTypeProperty>(inputStandardProperty(ParticleProperty::ParticleTypeProperty)));

				return radii;
			}
		}
	}

	std::fill(radii.begin(), radii.end(), FloatType(1));
	return radii;
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void ParticleModifier::saveToStream(ObjectSaveStream& stream)
{
	Modifier::saveToStream(stream);
	stream.beginChunk(0x01);
	// For future use...
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void ParticleModifier::loadFromStream(ObjectLoadStream& stream)
{
	Modifier::loadFromStream(stream);
	stream.expectChunk(0x01);
	// For future use...
	stream.closeChunk();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

