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
#include <core/reference/RefTarget.h>
#include "Controller.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

/**
 * \brief Base class for animation keys.
 */
class OVITO_CORE_EXPORT AnimationKey : public RefTarget
{
public:

	/// Constructor.
	AnimationKey(DataSet* dataset, TimePoint time = 0) : RefTarget(dataset), _time(time) {
		INIT_PROPERTY_FIELD(time);
	}

	/// Returns the value of this animation key as a QVariant.
	virtual QVariant valueQVariant() const = 0;

	/// Sets the value of the key.
	virtual bool setValueQVariant(const QVariant& v) = 0;

private:

	/// The animation time at which the key is positioned.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(TimePoint, time, setTime);

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief Animation key class for float controllers.
 */
class OVITO_CORE_EXPORT FloatAnimationKey : public AnimationKey
{
public:

	/// The type of value stored by this animation key.
	using value_type = FloatType;

	/// The type used to initialize default values of this key.
	using nullvalue_type = FloatType;

	/// The type used for derivatives/tangents.
	using tangent_type = FloatType;

	/// Constructor.
	Q_INVOKABLE FloatAnimationKey(DataSet* dataset, TimePoint time = 0, FloatType value = 0) : AnimationKey(dataset, time), _value(value) {
		INIT_PROPERTY_FIELD(value);
	}

	/// Returns the value of this animation key as a QVariant.
	virtual QVariant valueQVariant() const override {
		return QVariant::fromValue(value());
	}

	/// Sets the value of the key.
	virtual bool setValueQVariant(const QVariant& v) override {
		if(!v.canConvert<value_type>()) return false;
		setValue(v.value<value_type>());
		return true;
	}

private:

	Q_OBJECT
	OVITO_OBJECT

	/// Stores the value of the key.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(value_type, value, setValue);
};

/**
 * \brief Animation key class for integer controllers.
 */
class OVITO_CORE_EXPORT IntegerAnimationKey : public AnimationKey
{
public:

	/// The type of value stored by this animation key.
	using value_type = int;

	/// The type used to initialize default values of this key.
	using nullvalue_type = int;

	/// The type used for derivatives/tangents.
	using tangent_type = int;

	/// Constructor.
	Q_INVOKABLE IntegerAnimationKey(DataSet* dataset, TimePoint time = 0, int value = 0) : AnimationKey(dataset, time), _value(value) {
		INIT_PROPERTY_FIELD(value);
	}

	/// Returns the value of this animation key as a QVariant.
	virtual QVariant valueQVariant() const override {
		return QVariant::fromValue(value());
	}

	/// Sets the value of the key.
	virtual bool setValueQVariant(const QVariant& v) override {
		if(!v.canConvert<value_type>()) return false;
		setValue(v.value<value_type>());
		return true;
	}

private:

	Q_OBJECT
	OVITO_OBJECT

	/// Stores the value of the key.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(value_type, value, setValue);
};

/**
 * \brief Animation key class for Vector3 controllers.
 */
class OVITO_CORE_EXPORT Vector3AnimationKey : public AnimationKey
{
public:

	/// The type of value stored by this animation key.
	using value_type = Vector3;

	/// The type used to initialize default values of this key.
	using nullvalue_type = Vector3::Zero;

	/// The type used for derivatives/tangents.
	using tangent_type = Vector3;

	/// Constructor.
	Q_INVOKABLE Vector3AnimationKey(DataSet* dataset, TimePoint time = 0, const Vector3& value = Vector3::Zero()) : AnimationKey(dataset, time), _value(value) {
		INIT_PROPERTY_FIELD(value);
	}

	/// Returns the value of this animation key as a QVariant.
	virtual QVariant valueQVariant() const override {
		return QVariant::fromValue(value());
	}

	/// Sets the value of the key.
	virtual bool setValueQVariant(const QVariant& v) override {
		if(!v.canConvert<value_type>()) return false;
		setValue(v.value<value_type>());
		return true;
	}

private:

	Q_OBJECT
	OVITO_OBJECT

	/// Stores the value of the key.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(value_type, value, setValue);
};

/**
 * \brief Animation key class for position controllers.
 */
class OVITO_CORE_EXPORT PositionAnimationKey : public AnimationKey
{
public:

	/// The type of value stored by this animation key.
	using value_type = Vector3;

	/// The type used to initialize default values of this key.
	using nullvalue_type = Vector3::Zero;

	/// The type used for derivatives/tangents.
	using tangent_type = Vector3;

	/// Constructor.
	Q_INVOKABLE PositionAnimationKey(DataSet* dataset, TimePoint time = 0, const Vector3& value = Vector3::Zero()) : AnimationKey(dataset, time), _value(value) {
		INIT_PROPERTY_FIELD(value);
	}

	/// Returns the value of this animation key as a QVariant.
	virtual QVariant valueQVariant() const override {
		return QVariant::fromValue(value());
	}

	/// Sets the value of the key.
	virtual bool setValueQVariant(const QVariant& v) override {
		if(!v.canConvert<value_type>()) return false;
		setValue(v.value<value_type>());
		return true;
	}	

private:

	Q_OBJECT
	OVITO_OBJECT

	/// Stores the value of the key.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(value_type, value, setValue);
};

/**
 * \brief Animation key class for rotation controllers.
 */
class OVITO_CORE_EXPORT RotationAnimationKey : public AnimationKey
{
public:

	/// The type of value stored by this animation key.
	using value_type = Rotation;

	/// The type used to initialize default values of this key.
	using nullvalue_type = Rotation::Identity;

	/// The type used for derivatives/tangents.
	using tangent_type = Rotation;

	/// Constructor.
	Q_INVOKABLE RotationAnimationKey(DataSet* dataset, TimePoint time = 0, const Rotation& value = Rotation::Identity()) : AnimationKey(dataset, time), _value(value) {
		INIT_PROPERTY_FIELD(value);
	}

	/// Returns the value of this animation key as a QVariant.
	virtual QVariant valueQVariant() const override {
		return QVariant::fromValue(value());
	}

	/// Sets the value of the key.
	virtual bool setValueQVariant(const QVariant& v) override {
		if(!v.canConvert<value_type>()) return false;
		setValue(v.value<value_type>());
		return true;
	}	

private:

	Q_OBJECT
	OVITO_OBJECT

	/// Stores the value of the key.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(value_type, value, setValue);
};

/**
 * \brief Animation key class for scaling controllers.
 */
class OVITO_CORE_EXPORT ScalingAnimationKey : public AnimationKey
{
public:

	/// The type of value stored by this animation key.
	using value_type = Scaling;

	/// The type used to initialize default values of this key.
	using nullvalue_type = Scaling::Identity;

	/// The type used for derivatives/tangents.
	using tangent_type = Scaling;

	/// Constructor.
	Q_INVOKABLE ScalingAnimationKey(DataSet* dataset, TimePoint time = 0, const Scaling& value = Scaling::Identity()) : AnimationKey(dataset, time), _value(value) {
		INIT_PROPERTY_FIELD(value);
	}

	/// Returns the value of this animation key as a QVariant.
	virtual QVariant valueQVariant() const override {
		return QVariant::fromValue(value());
	}

	/// Sets the value of the key.
	virtual bool setValueQVariant(const QVariant& v) override {
		if(!v.canConvert<value_type>()) return false;
		setValue(v.value<value_type>());
		return true;
	}
	
private:

	Q_OBJECT
	OVITO_OBJECT

	/// Stores the value of the key.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(value_type, value, setValue);
};

/**
 * \brief Default implementation of the value interpolator concept that does linear interpolation.
 *
 * This template class interpolates linearly between two values of arbitrary types.
 * The value 0.0 of the interpolation parameter \c t is mapped to the first value.
 * The value 1.0 of the interpolation parameter \c t is mapped to the second value.
 */
template<typename ValueType>
struct LinearValueInterpolator {
	ValueType operator()(FloatType t, const ValueType& value1, const ValueType& value2) const {
		return static_cast<ValueType>(value1 + (t * (value2 - value1)));
	}
};

/**
 * \brief Implementation of the value interpolator concept for rotations.
 *
 * This class is required because the Rotation class does not support the standard
 * addition, scalar multiplication and subtraction operators.
 */
template<>
struct LinearValueInterpolator<Rotation> {
	Rotation operator()(FloatType t, const Rotation& value1, const Rotation& value2) const {
#if 1
		return interpolate(value1, value2, t);
#else
		Rotation diff = value2 * value1.inverse();
		return Rotation(diff.axis(), diff.angle() * t) * value1;
#endif
	}

	template<typename T>
    static RotationT<T> interpolate(const RotationT<T>& rot1, const RotationT<T>& rot2, T t) {
    	OVITO_ASSERT(t >= 0 && t <= 1);

    	RotationT<T> _rot2;
    	if(rot1.axis().dot(rot2.axis()) < T(0))
    		_rot2 = RotationT<T>(-rot2.axis(), -rot2.angle(), false);
    	else
    		_rot2 = rot2;

    	// Determine interpolation type, compute extra spins, and adjust angles accordingly.
		if(rot1.axis().equals(_rot2.axis())) {
			return RotationT<T>((T(1) - t) * rot1.axis() + t * _rot2.axis(), (T(1) - t) * rot1.angle() + t * _rot2.angle());
		}
		else if(rot1.angle() != T(0)) {
			T fDiff = _rot2.angle() - rot1.angle();
			T fDiffUnit = fDiff/T(2*FLOATTYPE_PI);
			int extraSpins = (int)floor(fDiffUnit + T(0.5));
			if(extraSpins * fDiffUnit * (fDiffUnit - extraSpins) < 0)
				extraSpins = -extraSpins;

	    	QuaternionT<T> q1 = (QuaternionT<T>)rot1;
	    	QuaternionT<T> q2 = (QuaternionT<T>)_rot2;

	    	// Eliminate any non-acute angles between quaternions. This
	    	// is done to prevent potential discontinuities that are the result of
	    	// invalid intermediate value quaternions.
	    	if(q1.dot(q2) < T(0))
	    		q2 = -q2;

	    	// Clamp identity quaternions so that |w| <= 1 (avoids problems with
	    	// call to acos() in slerpExtraSpins).
	    	if(q1.w() < T(-1)) q1.w() = T(-1); else if(q1.w() > T(1)) q1.w() = T(1);
	    	if(q2.w() < T(-1)) q2.w() = T(-1); else if(q2.w() > T(1)) q2.w() = T(1);

			RotationT<T> result = RotationT<T>(slerpExtraSpins(t, q1, q2, extraSpins));
			if(result.axis().dot(interpolateAxis(t, rot1.axis(), _rot2.axis())) < T(0))
				result = RotationT<T>(-result.axis(), -result.angle(), false);
			int nrev = floor((t * _rot2.angle() + (T(1) - t) * rot1.angle() - result.angle())/T(2*FLOATTYPE_PI) + T(0.5));
			result.addRevolutions(nrev);
			return result;
		}
		else {
			return RotationT<T>(interpolateAxis(t, rot1.axis(), _rot2.axis()), (T(1) - t) * rot1.angle() + t * _rot2.angle());
		}
    }

    template<typename T>
	static inline Vector_3<T> interpolateAxis(T time, const Vector_3<T>& axis0, const Vector_3<T>& axis1) {
		// assert:  axis0 and axis1 are unit length
		// assert:  axis0.dot(axis1) >= 0
		// assert:  0 <= time <= 1

		T cos = axis0.dot(axis1);  // >= 0 by assertion
		OVITO_ASSERT(cos >= T(0));
		if(cos > T(1)) cos = T(1); // round-off error might create problems in acos call

		T angle = acos(cos);
		T invSin = T(1) / sin(angle);
		T timeAngle = time * angle;
		T coeff0 = sin(angle - timeAngle) * invSin;
		T coeff1 = sin(timeAngle) * invSin;

		return (coeff0 * axis0 + coeff1 * axis1);
	}

    template<typename T>
	static inline QuaternionT<T> slerpExtraSpins(T t, const QuaternionT<T>& p, const QuaternionT<T>& q, int iExtraSpins) {
		T fCos = p.dot(q);
		OVITO_ASSERT(fCos >= T(0));

		// Numerical round-off error could create problems in call to acos.
		if(fCos < T(-1)) fCos = T(-1);
		else if(fCos > T(1)) fCos = T(1);

		T fAngle = acos(fCos);
		T fSin = sin(fAngle);  // fSin >= 0 since fCos >= 0

		if(fSin < T(1e-3)) {
			return p;
		}
		else {
			T fPhase = T(FLOATTYPE_PI) * (T)iExtraSpins * t;
			T fInvSin = T(1) / fSin;
			T fCoeff0 = sin((T(1) - t) * fAngle - fPhase) * fInvSin;
			T fCoeff1 = sin(t * fAngle + fPhase) * fInvSin;
			return QuaternionT<T>(fCoeff0*p.x() + fCoeff1*q.x(), fCoeff0*p.y() + fCoeff1*q.y(),
			                        fCoeff0*p.z() + fCoeff1*q.z(), fCoeff0*p.w() + fCoeff1*q.w());
		}
	}
};

/**
 * \brief Implementation of the value interpolator concept for scaling values.
 *
 * This class is required because the Scaling class does not support the standard
 * addition, scalar multiplication and subtraction operators.
 */
template<>
struct LinearValueInterpolator<Scaling> {
	Scaling operator()(FloatType t, const Scaling& value1, const Scaling& value2) const {
		return Scaling::interpolate(value1, value2, t);
	}
};

/**
 * \brief Default implementation of the value interpolator concept that does smooth interpolation.
 *
 * This template class interpolates using a cubic spline between two values of arbitrary data type.
 * The value 0.0 of the interpolation parameter \c t is mapped to the first value.
 * The value 1.0 of the interpolation parameter \c t is mapped to the second value.
 */
template<typename ValueType>
struct SplineValueInterpolator {
	ValueType operator()(FloatType t, const ValueType& value1, const ValueType& value2, const ValueType& outPoint1, const ValueType& inPoint2) const {
		FloatType Ti = FloatType(1) - t;
		FloatType U2 = t * t, T2 = Ti * Ti;
		FloatType U3 = U2 * t, T3 = T2 * Ti;
		return value1 * T3 + outPoint1 * (FloatType(3) * t * T2) + inPoint2 * (FloatType(3) * U2 * Ti) + value2 * U3;
	}
};

/**
 * \brief Implementation of the smooth value interpolator concept for rotations.
 *
 * This class is required because the Rotation class does not support the standard
 * addition, scalar multiplication, and subtraction operators.
 */
template<>
struct SplineValueInterpolator<Rotation> {
	Rotation operator()(FloatType t, const Rotation& value1, const Rotation& value2, const Rotation& outPoint1, const Rotation& inPoint2) const {
		return Rotation(Rotation::interpolateQuad(value1, value2, outPoint1, inPoint2, t));
	}
};

/**
 * \brief Implementation of the smooth value interpolator concept for scaling values.
 *
 * This class is required because the Scaling class does not support the standard
 * addition, scalar multiplication, and subtraction operators.
 */
template<>
struct SplineValueInterpolator<Scaling> {
	Scaling operator()(FloatType t, const Scaling& value1, const Scaling& value2, const Scaling& outPoint1, const Scaling& inPoint2) const {
		return Scaling::interpolateQuad(value1, value2, outPoint1, inPoint2, t);
	}
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
