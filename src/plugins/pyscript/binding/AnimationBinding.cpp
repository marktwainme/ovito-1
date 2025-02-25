///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#include <plugins/pyscript/PyScript.h>
#include <core/animation/TimeInterval.h>
#include <core/animation/AnimationSettings.h>
#include <core/animation/controller/Controller.h>
#include <core/animation/controller/PRSTransformationController.h>
#include <core/animation/controller/LinearInterpolationControllers.h>
#include <core/animation/controller/SplineInterpolationControllers.h>
#include <core/animation/controller/TCBInterpolationControllers.h>
#include <core/animation/controller/LookAtController.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;
namespace py = pybind11;

PYBIND11_PLUGIN(PyScriptAnimation)
{
	py::options options;
	options.disable_function_signatures();

	py::module m("PyScriptAnimation");

	py::object TimeInterval_py = py::class_<TimeInterval>(m, "TimeInterval")
		.def(py::init<>())
		.def(py::init<TimePoint>())
		.def(py::init<TimePoint, TimePoint>())
		.def_property("start", &TimeInterval::start, &TimeInterval::setStart)
		.def_property("end", &TimeInterval::end, &TimeInterval::setEnd)
		.def_property_readonly("is_empty", &TimeInterval::isEmpty)
		.def_property_readonly("is_infinite", &TimeInterval::isInfinite)
		.def_property("duration", &TimeInterval::duration, &TimeInterval::setDuration)
		.def("set_infinite", &TimeInterval::setInfinite)
		.def("set_empty", &TimeInterval::setEmpty)
		.def("set_instant", &TimeInterval::setInstant)
		.def("contains", &TimeInterval::contains)
		.def("intersect", &TimeInterval::intersect)
		.def_static("time_to_seconds", &TimeToSeconds)
		.def_static("seconds_to_time", &TimeFromSeconds)
		.def_property_readonly_static("infinite", &TimeInterval::infinite)
		.def_property_readonly_static("empty", &TimeInterval::empty)
		.def(py::self == TimeInterval())
		.def(py::self != TimeInterval())
	;
	py::setattr(TimeInterval_py, "TimeNegativeInfinity", py::cast(TimeNegativeInfinity()));
	py::setattr(TimeInterval_py, "TimePositiveInfinity", py::cast(TimePositiveInfinity()));

	ovito_class<AnimationSettings, RefTarget>(m,
			"Stores animation-related settings of the current :py:attr:`~ovito.DataSet`. You can access "
			"an instance of this class through the dataset's :py:attr:`~ovito.DataSet.anim` attribute."
			"\n\n"
			"Animation settings comprise the animation length (number of frames) and the current animation time. "
			"For example, to step through each animation frame and perform some action::"
			"\n\n"
			"    for frame in range(0, dataset.anim.last_frame + 1):\n"
			"        dataset.anim.current_frame = frame    # Jump to the animation frame.\n"
			"        performSomething()\n"
			"\n")
		.def_property("time", &AnimationSettings::time, &AnimationSettings::setTime)
		//.def_property("animation_interval", &AnimationSettings::animationInterval, &AnimationSettings::setAnimationInterval)
		.def_property("frames_per_second", &AnimationSettings::framesPerSecond, &AnimationSettings::setFramesPerSecond,
				"Controls the playback speed of the animation. This parameter also determines the playback speed of movie files generated by OVITO."
				"\n\n"
				":Default: 10\n")
		//.def_property("ticks_per_frame", &AnimationSettings::ticksPerFrame, &AnimationSettings::setTicksPerFrame)
		.def_property("current_frame", &AnimationSettings::currentFrame, &AnimationSettings::setCurrentFrame,
				"The current animation frame. This parameter controls the position of the time slider in OVITO's main window "
				"and determines which animation frame is shown in the viewports."
				"\n\n"
				":Default: 0\n")
		.def_property("last_frame", &AnimationSettings::lastFrame, &AnimationSettings::setLastFrame,
				"The index of the last animation frame. You can change this property to set a new animation length."
				"\n\n"
				":Default: 0\n")
		.def_property("first_frame", &AnimationSettings::firstFrame, &AnimationSettings::setFirstFrame,
				"The index of the first animation frame."
				"\n\n"
				":Default: 0\n")
		//.def_property("playback_speed", &AnimationSettings::playbackSpeed, &AnimationSettings::setPlaybackSpeed)
		//.def_property_readonly("is_animating", &AnimationSettings::isAnimating)
		//.def_property("auto_key_mode", &AnimationSettings::autoKeyMode, &AnimationSettings::setAutoKeyMode)
		//.def_property_readonly("is_time_changing", &AnimationSettings::isTimeChanging)
		.def("frame_to_time", &AnimationSettings::frameToTime)
		.def("time_to_frame", &AnimationSettings::timeToFrame)
		.def("snap_time", &AnimationSettings::snapTime)
		.def("time_to_string", &AnimationSettings::timeToString)
		.def("string_to_time", &AnimationSettings::stringToTime)
		.def("jump_to_animation_start", &AnimationSettings::jumpToAnimationStart)
		.def("jump_to_animation_end", &AnimationSettings::jumpToAnimationEnd)
		.def("jump_to_next_frame", &AnimationSettings::jumpToNextFrame)
		.def("jump_to_previous_frame", &AnimationSettings::jumpToPreviousFrame)
		.def("start_animation_playback", &AnimationSettings::startAnimationPlayback)
		.def("stop_animation_playback", &AnimationSettings::stopAnimationPlayback)
	;

	py::handle Controller_py = ovito_abstract_class<Controller, RefTarget>(m)
		.def_property_readonly("type", &Controller::controllerType)
		.def_property_readonly("float_value", &Controller::currentFloatValue)
		.def_property_readonly("int_value", &Controller::currentIntValue)
		.def_property_readonly("vector3_value", &Controller::currentVector3Value)
		.def_property_readonly("color_value", &Controller::currentColorValue)
		.def("set_float_value", &Controller::setFloatValue)
		.def("set_int_value", &Controller::setIntValue)
		.def("set_vector3_value", &Controller::setVector3Value)
		.def("set_color_value", &Controller::setColorValue)
		.def("set_position_value", &Controller::setPositionValue)
		.def("set_rotation_value", &Controller::setRotationValue)
		.def("set_scaling_value", &Controller::setScalingValue)
	;

	py::enum_<Controller::ControllerType>(Controller_py, "Type")
		.value("Float", Controller::ControllerTypeFloat)
		.value("Int", Controller::ControllerTypeInt)
		.value("Vector3", Controller::ControllerTypeVector3)
		.value("Position", Controller::ControllerTypePosition)
		.value("Rotation", Controller::ControllerTypeRotation)
		.value("Scaling", Controller::ControllerTypeScaling)
		.value("Transformation", Controller::ControllerTypeTransformation)
	;

	ovito_class<PRSTransformationController, Controller>(m)
		.def_property("position", &PRSTransformationController::positionController, &PRSTransformationController::setPositionController)
		.def_property("rotation", &PRSTransformationController::rotationController, &PRSTransformationController::setRotationController)
		.def_property("scaling", &PRSTransformationController::scalingController, &PRSTransformationController::setScalingController)
	;

	ovito_abstract_class<KeyframeController, Controller>{m}
	;

	ovito_class<LinearFloatController, KeyframeController>{m}
	;

	ovito_class<LinearIntegerController, KeyframeController>{m}
	;

	ovito_class<LinearVectorController, KeyframeController>{m}
	;

	ovito_class<LinearPositionController, KeyframeController>{m}
	;

	ovito_class<LinearRotationController, KeyframeController>{m}
	;

	ovito_class<LinearScalingController, KeyframeController>{m}
	;

	ovito_class<SplinePositionController, KeyframeController>{m}
	;

	ovito_class<TCBPositionController, KeyframeController>{m}
	;

	ovito_class<LookAtController, Controller>{m}
	;

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScriptAnimation);

};
