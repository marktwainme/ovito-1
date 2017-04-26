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
#include <core/animation/controller/PRSTransformationController.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/animation/TimeInterval.h>
#include <core/utilities/linalg/AffineDecomposition.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(PRSTransformationController, Controller);
DEFINE_REFERENCE_FIELD(PRSTransformationController, positionController, "Position", Controller);
DEFINE_REFERENCE_FIELD(PRSTransformationController, rotationController, "Rotation", Controller);
DEFINE_REFERENCE_FIELD(PRSTransformationController, scalingController, "Scaling", Controller);
SET_PROPERTY_FIELD_LABEL(PRSTransformationController, positionController, "Position");
SET_PROPERTY_FIELD_LABEL(PRSTransformationController, rotationController, "Rotation");
SET_PROPERTY_FIELD_LABEL(PRSTransformationController, scalingController, "Scaling");
SET_PROPERTY_FIELD_UNITS(PRSTransformationController, positionController, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(PRSTransformationController, rotationController, AngleParameterUnit);
SET_PROPERTY_FIELD_UNITS(PRSTransformationController, scalingController, PercentParameterUnit);

/******************************************************************************
* Default constructor.
******************************************************************************/
PRSTransformationController::PRSTransformationController(DataSet* dataset) : Controller(dataset)
{
	INIT_PROPERTY_FIELD(positionController);
	INIT_PROPERTY_FIELD(rotationController);
	INIT_PROPERTY_FIELD(scalingController);
	setPositionController(ControllerManager::createPositionController(dataset));
	setRotationController(ControllerManager::createRotationController(dataset));
	setScalingController(ControllerManager::createScalingController(dataset));
}

/******************************************************************************
* Let the controller apply its value at a certain time to the input value.
******************************************************************************/
void PRSTransformationController::applyTransformation(TimePoint time, AffineTransformation& result, TimeInterval& validityInterval)
{
	positionController()->applyTranslation(time, result, validityInterval);
	rotationController()->applyRotation(time, result, validityInterval);
	scalingController()->applyScaling(time, result, validityInterval);
}

/******************************************************************************
* Sets the controller's value at the specified time.
******************************************************************************/
void PRSTransformationController::setTransformationValue(TimePoint time, const AffineTransformation& newValue, bool isAbsolute)
{
	AffineDecomposition decomp(newValue);
	positionController()->setPositionValue(time, decomp.translation, isAbsolute);
	rotationController()->setRotationValue(time, Rotation(decomp.rotation), isAbsolute);
	scalingController()->setScalingValue(time, decomp.scaling, isAbsolute);
}

/******************************************************************************
* Adjusts the controller's value after a scene node has gotten a new parent node.
******************************************************************************/
void PRSTransformationController::changeParent(TimePoint time, const AffineTransformation& oldParentTM, const AffineTransformation& newParentTM, SceneNode* contextNode)
{
	positionController()->changeParent(time, oldParentTM, newParentTM, contextNode);
	rotationController()->changeParent(time, oldParentTM, newParentTM, contextNode);
	scalingController()->changeParent(time, oldParentTM, newParentTM, contextNode);
}

/******************************************************************************
* Computes the largest time interval containing the given time during which the
* controller's value is constant.
******************************************************************************/
TimeInterval PRSTransformationController::validityInterval(TimePoint time)
{
	TimeInterval iv = TimeInterval::infinite();
	iv.intersect(positionController()->validityInterval(time));
	iv.intersect(rotationController()->validityInterval(time));
	iv.intersect(scalingController()->validityInterval(time));
	return iv;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
