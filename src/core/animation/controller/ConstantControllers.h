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


#include <core/Core.h>
#include "Controller.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

/**
 * \brief An animation controller with a constant float value.
 */
class OVITO_CORE_EXPORT ConstFloatController : public Controller
{
public:

	/// Constructor.
	Q_INVOKABLE ConstFloatController(DataSet* dataset) : Controller(dataset), _value(0) {
		INIT_PROPERTY_FIELD(value);
	}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeFloat; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual FloatType getFloatValue(TimePoint time, TimeInterval& validityInterval) override { return _value; }

	/// \brief Sets the controller's value at the given animation time.
	virtual void setFloatValue(TimePoint time, FloatType newValue) override { _value = newValue; }

private:

	/// Stores the constant value of the controller.
	DECLARE_PROPERTY_FIELD(FloatType, value);

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief An animation controller with a constant int value.
 */
class OVITO_CORE_EXPORT ConstIntegerController : public Controller
{
public:

	/// Constructor.
	Q_INVOKABLE ConstIntegerController(DataSet* dataset) : Controller(dataset), _value(0) {
		INIT_PROPERTY_FIELD(value);
	}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeInt; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual int getIntValue(TimePoint time, TimeInterval& validityInterval) override { return _value; }

	/// \brief Sets the controller's value at the given animation time.
	virtual void setIntValue(TimePoint time, int newValue) override { _value = newValue; }

private:

	/// Stores the constant value of the controller.
	DECLARE_PROPERTY_FIELD(int, value);

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief An animation controller with a constant Vector3 value.
 */
class OVITO_CORE_EXPORT ConstVectorController : public Controller
{
public:

	/// Constructor.
	Q_INVOKABLE ConstVectorController(DataSet* dataset) : Controller(dataset), _value(Vector3::Zero()) {
		INIT_PROPERTY_FIELD(value);
	}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeVector3; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getVector3Value(TimePoint time, Vector3& result, TimeInterval& validityInterval) override { result = _value; }

	/// \brief Sets the controller's value at the given animation time.
	virtual void setVector3Value(TimePoint time, const Vector3& newValue) override { _value = newValue; }

private:

	/// Stores the constant value of the controller.
	DECLARE_PROPERTY_FIELD(Vector3, value);

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief An animation controller with a constant position value.
 */
class OVITO_CORE_EXPORT ConstPositionController : public Controller
{
public:

	/// Constructor.
	Q_INVOKABLE ConstPositionController(DataSet* dataset) : Controller(dataset), _value(Vector3::Zero()) {
		INIT_PROPERTY_FIELD(value);
	}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypePosition; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getPositionValue(TimePoint time, Vector3& result, TimeInterval& validityInterval) override { result = _value; }

	/// \brief Sets a position controller's value at the given animation time.
	virtual void setPositionValue(TimePoint time, const Vector3& newValue, bool isAbsolute) override {
		if(isAbsolute) _value = newValue;
		else _value = newValue + _value.value();
	}

private:

	/// Stores the constant value of the controller.
	DECLARE_PROPERTY_FIELD(Vector3, value);

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief An animation controller with a constant rotation value.
 */
class OVITO_CORE_EXPORT ConstRotationController : public Controller
{
public:

	/// Constructor.
	Q_INVOKABLE ConstRotationController(DataSet* dataset) : Controller(dataset), _value(Rotation::Identity()) {
		INIT_PROPERTY_FIELD(value);
	}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeRotation; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getRotationValue(TimePoint time, Rotation& result, TimeInterval& validityInterval) override { result = _value; }

	/// \brief Sets a rotation controller's value at the given animation time.
	virtual void setRotationValue(TimePoint time, const Rotation& newValue, bool isAbsolute) override {
		if(isAbsolute) _value = newValue;
		else _value = newValue * _value.value();
	}

private:

	/// Stores the constant value of the controller.
	DECLARE_PROPERTY_FIELD(Rotation, value);

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief An animation controller with a constant scaling value.
 */
class OVITO_CORE_EXPORT ConstScalingController : public Controller
{
public:

	/// Constructor.
	Q_INVOKABLE ConstScalingController(DataSet* dataset) : Controller(dataset), _value(Scaling::Identity()) {
		INIT_PROPERTY_FIELD(value);
	}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeScaling; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getScalingValue(TimePoint time, Scaling& result, TimeInterval& validityInterval) override { result = _value; }

	/// \brief Sets a scaling controller's value at the given animation time.
	virtual void setScalingValue(TimePoint time, const Scaling& newValue, bool isAbsolute) override {
		if(isAbsolute) _value = newValue;
		else _value = newValue * _value.value();
	}

private:

	/// Stores the constant value of the controller.
	DECLARE_PROPERTY_FIELD(Scaling, value);

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
