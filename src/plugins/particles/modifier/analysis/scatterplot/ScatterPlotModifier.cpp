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
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/animation/AnimationSettings.h>
#include "ScatterPlotModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, ScatterPlotModifier, ParticleModifier);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _selectXAxisInRange, "SelectXAxisInRange");
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _selectionXAxisRangeStart, "SelectionXAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _selectionXAxisRangeEnd, "SelectionXAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _selectYAxisInRange, "SelectYAxisInRange");
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _selectionYAxisRangeStart, "SelectionYAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _selectionYAxisRangeEnd, "SelectionYAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _fixXAxisRange, "FixXAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _xAxisRangeStart, "XAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _xAxisRangeEnd, "XAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _fixYAxisRange, "FixYAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _yAxisRangeStart, "YAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _yAxisRangeEnd, "YAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _xAxisProperty, "XAxisProperty");
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _yAxisProperty, "YAxisProperty");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectXAxisInRange, "Select particles in x-range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectionXAxisRangeStart, "Selection x-range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectionXAxisRangeEnd, "Selection x-range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectYAxisInRange, "Select particles in y-range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectionYAxisRangeStart, "Selection y-range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectionYAxisRangeEnd, "Selection y-range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _fixXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _xAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _xAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _fixYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _yAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _yAxisRangeEnd, "Y-range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _xAxisProperty, "X-axis property");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _yAxisProperty, "Y-axis property");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ScatterPlotModifier::ScatterPlotModifier(DataSet* dataset) : ParticleModifier(dataset),
	_selectXAxisInRange(false),	_selectionXAxisRangeStart(0), _selectionXAxisRangeEnd(1),
	_selectYAxisInRange(false),	_selectionYAxisRangeStart(0), _selectionYAxisRangeEnd(1),
	_fixXAxisRange(false), _xAxisRangeStart(0),	_xAxisRangeEnd(0), _fixYAxisRange(false),
	_yAxisRangeStart(0), _yAxisRangeEnd(0)
{
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectXAxisInRange);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectionXAxisRangeStart);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectionXAxisRangeEnd);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectYAxisInRange);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectionYAxisRangeStart);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectionYAxisRangeEnd);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_fixXAxisRange);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_xAxisRangeStart);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_xAxisRangeEnd);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_fixYAxisRange);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_yAxisRangeStart);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_yAxisRangeEnd);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_xAxisProperty);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_yAxisProperty);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ScatterPlotModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	ParticlePropertyReference bestProperty;
	if(xAxisProperty().isNull() || yAxisProperty().isNull()) {
		// Select the first available particle property from the input state.
		PipelineFlowState input = pipeline->evaluatePipeline(dataset()->animationSettings()->time(), modApp, false);
		for(DataObject* o : input.objects()) {
			ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
			if(property && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
	}
	if(xAxisProperty().isNull() && !bestProperty.isNull()) {
		setXAxisProperty(bestProperty);
	}
	if(yAxisProperty().isNull() && !bestProperty.isNull()) {
		setYAxisProperty(bestProperty);
	}
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus ScatterPlotModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	// Get the source property.
	if(xAxisProperty().isNull())
		throwException(tr("Select a particle property first."));
	ParticlePropertyObject* xProperty = xAxisProperty().findInState(input());
	ParticlePropertyObject* yProperty = yAxisProperty().findInState(input());
	if(!xProperty)
		throwException(tr("The selected particle property with the name '%1' does not exist.").arg(xAxisProperty().name()));
	if(!yProperty)
		throwException(tr("The selected particle property with the name '%1' does not exist.").arg(yAxisProperty().name()));
	if(xAxisProperty().vectorComponent() >= (int)xProperty->componentCount())
		throwException(tr("The selected vector component is out of range. The particle property '%1' contains only %2 values per particle.").arg(xAxisProperty().name()).arg(xProperty->componentCount()));
	if(yAxisProperty().vectorComponent() >= (int)yProperty->componentCount())
		throwException(tr("The selected vector component is out of range. The particle property '%1' contains only %2 values per particle.").arg(yAxisProperty().name()).arg(yProperty->componentCount()));

	size_t xVecComponent = std::max(0, xAxisProperty().vectorComponent());
	size_t xVecComponentCount = xProperty->componentCount();
	size_t yVecComponent = std::max(0, yAxisProperty().vectorComponent());
	size_t yVecComponentCount = yProperty->componentCount();

	ParticleTypeProperty* typeProperty = static_object_cast<ParticleTypeProperty>(inputStandardProperty(ParticleProperty::ParticleTypeProperty));
	if(typeProperty) {
		_colorMap = typeProperty->colorMap();
		_typeData.clear();
		_typeData.resize(typeProperty->size());
		std::copy(typeProperty->constDataInt(), typeProperty->constDataInt() + typeProperty->size(), _typeData.begin());
	}
	else {
		_colorMap.clear();
		_typeData.clear();
	}

	ParticlePropertyObject* selProperty = nullptr;
	FloatType selectionXAxisRangeStart = _selectionXAxisRangeStart;
	FloatType selectionXAxisRangeEnd = _selectionXAxisRangeEnd;
	FloatType selectionYAxisRangeStart = _selectionYAxisRangeStart;
	FloatType selectionYAxisRangeEnd = _selectionYAxisRangeEnd;
	size_t numSelected = 0;
	if(_selectXAxisInRange || _selectYAxisInRange) {
		selProperty = outputStandardProperty(ParticleProperty::SelectionProperty, false);
		std::fill(selProperty->dataInt(), selProperty->dataInt() + selProperty->size(), 1);
		numSelected = selProperty->size();
	}
	if(_selectXAxisInRange) {
		if(selectionXAxisRangeStart > selectionXAxisRangeEnd)
			std::swap(selectionXAxisRangeStart, selectionXAxisRangeEnd);
	}
	if(_selectYAxisInRange) {
		if(selectionYAxisRangeStart > selectionYAxisRangeEnd)
			std::swap(selectionYAxisRangeStart, selectionYAxisRangeEnd);
	}

	double xIntervalStart = _xAxisRangeStart;
	double xIntervalEnd = _xAxisRangeEnd;
	double yIntervalStart = _yAxisRangeStart;
	double yIntervalEnd = _yAxisRangeEnd;

	_xyData.clear();
	_xyData.resize(inputParticleCount());

	// Collect X coordinates.
	if(xProperty->dataType() == qMetaTypeId<FloatType>()) {
		for(size_t i = 0; i < inputParticleCount(); i++) {
			_xyData[i].rx() = xProperty->getFloatComponent(i, xVecComponent);
		}
	}
	else if(xProperty->dataType() == qMetaTypeId<int>()) {
		for(size_t i = 0; i < inputParticleCount(); i++) {
			_xyData[i].rx() = xProperty->getIntComponent(i, xVecComponent);
		}
	}
	else throwException(tr("Particle property '%1' has an invalid data type.").arg(xAxisProperty().name()));

	// Collect Y coordinates.
	if(yProperty->dataType() == qMetaTypeId<FloatType>()) {
		for(size_t i = 0; i < inputParticleCount(); i++) {
			_xyData[i].ry() = yProperty->getFloatComponent(i, yVecComponent);
		}
	}
	else if(yProperty->dataType() == qMetaTypeId<int>()) {
		for(size_t i = 0; i < inputParticleCount(); i++) {
			_xyData[i].ry() = yProperty->getIntComponent(i, yVecComponent);
		}
	}
	else throwException(tr("Particle property '%1' has an invalid data type.").arg(yAxisProperty().name()));

	// Determine value ranges.
	if(_fixXAxisRange == false || _fixYAxisRange == false) {
		Box2 bbox;
		for(const QPointF& p : _xyData) {
			bbox.addPoint(p.x(), p.y());
		}
		if(_fixXAxisRange == false) {
			xIntervalStart = bbox.minc.x();
			xIntervalEnd = bbox.maxc.x();
		}
		if(_fixYAxisRange == false) {
			yIntervalStart = bbox.minc.y();
			yIntervalEnd = bbox.maxc.y();
		}
	}

	if(selProperty && _selectXAxisInRange) {
		OVITO_ASSERT(selProperty->size() == _xyData.size());
		int* s = selProperty->dataInt();
		int* s_end = s + selProperty->size();
		for(const QPointF& p : _xyData) {
			if(p.x() < selectionXAxisRangeStart || p.x() > selectionXAxisRangeEnd) {
				*s = 0;
				numSelected--;
			}
			++s;
		}
		selProperty->changed();
	}

	if(selProperty && _selectYAxisInRange) {
		OVITO_ASSERT(selProperty->size() == _xyData.size());
		int* s = selProperty->dataInt();
		int* s_end = s + selProperty->size();
		for(const QPointF& p : _xyData) {
			if(p.y() < selectionYAxisRangeStart || p.y() > selectionYAxisRangeEnd) {
				if(*s) {
					*s = 0;
					numSelected--;
				}
			}
			++s;
		}
		selProperty->changed();
	}

	QString statusMessage;
	if(selProperty)
		statusMessage += tr("%1 particles selected (%2%)").arg(numSelected).arg((FloatType)numSelected * 100 / std::max(1,(int)selProperty->size()), 0, 'f', 1);

	_xAxisRangeStart = xIntervalStart;
	_xAxisRangeEnd = xIntervalEnd;
	_yAxisRangeStart = yIntervalStart;
	_yAxisRangeEnd = yIntervalEnd;

	notifyDependents(ReferenceEvent::ObjectStatusChanged);

	return PipelineStatus(PipelineStatus::Success, statusMessage);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
