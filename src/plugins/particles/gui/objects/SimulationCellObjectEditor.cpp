///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <gui/properties/Vector3ParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/BooleanRadioButtonParameterUI.h>
#include <core/viewport/ViewportConfiguration.h>
#include "SimulationCellObjectEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(SimulationCellEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(SimulationCellObject, SimulationCellEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SimulationCellEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create rollout.
	QWidget* rollout = createRollout(QString(), rolloutParams);

	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(8);

	{
		QGroupBox* dimensionalityGroupBox = new QGroupBox(tr("Dimensionality"), rollout);
		layout1->addWidget(dimensionalityGroupBox);

		QGridLayout* layout2 = new QGridLayout(dimensionalityGroupBox);
		layout2->setContentsMargins(4,4,4,4);
		layout2->setSpacing(2);

		BooleanRadioButtonParameterUI* is2dPUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(SimulationCellObject::is2D));
		is2dPUI->buttonTrue()->setText("2D");
		is2dPUI->buttonFalse()->setText("3D");
		layout2->addWidget(is2dPUI->buttonTrue(), 0, 0);
		layout2->addWidget(is2dPUI->buttonFalse(), 0, 1);
	}

	{
		QGroupBox* pbcGroupBox = new QGroupBox(tr("Periodic boundary conditions"), rollout);
		layout1->addWidget(pbcGroupBox);

		QGridLayout* layout2 = new QGridLayout(pbcGroupBox);
		layout2->setContentsMargins(4,4,4,4);
		layout2->setSpacing(2);

		BooleanParameterUI* pbcxPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SimulationCellObject::pbcX));
		pbcxPUI->checkBox()->setText("X");
		layout2->addWidget(pbcxPUI->checkBox(), 0, 0);

		BooleanParameterUI* pbcyPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SimulationCellObject::pbcY));
		pbcyPUI->checkBox()->setText("Y");
		layout2->addWidget(pbcyPUI->checkBox(), 0, 1);

		pbczPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SimulationCellObject::pbcZ));
		pbczPUI->checkBox()->setText("Z");
		layout2->addWidget(pbczPUI->checkBox(), 0, 2);
	}

	{
		QGroupBox* sizeGroupBox = new QGroupBox(tr("Box size"), rollout);
		layout1->addWidget(sizeGroupBox);

		QGridLayout* layout2 = new QGridLayout(sizeGroupBox);
		layout2->setContentsMargins(4,4,4,4);
		layout2->setSpacing(0);
		layout2->setColumnStretch(1, 1);

		QSignalMapper* signalMapperValueChanged = new QSignalMapper(this);
		QSignalMapper* signalMapperDragStart = new QSignalMapper(this);
		QSignalMapper* signalMapperDragStop = new QSignalMapper(this);
		QSignalMapper* signalMapperDragAbort = new QSignalMapper(this);
		for(int i = 0; i < 3; i++) {
			QLineEdit* textBox = new QLineEdit(rollout);
			simCellSizeSpinners[i] = new SpinnerWidget(rollout, textBox);
			simCellSizeSpinners[i]->setMinValue(0);
			layout2->addWidget(textBox, i, 1);
			layout2->addWidget(simCellSizeSpinners[i], i, 2);

			connect(simCellSizeSpinners[i], &SpinnerWidget::spinnerValueChanged, signalMapperValueChanged, (void (QSignalMapper::*)())&QSignalMapper::map);
			connect(simCellSizeSpinners[i], &SpinnerWidget::spinnerDragStart, signalMapperDragStart, (void (QSignalMapper::*)())&QSignalMapper::map);
			connect(simCellSizeSpinners[i], &SpinnerWidget::spinnerDragStop, signalMapperDragStop, (void (QSignalMapper::*)())&QSignalMapper::map);
			connect(simCellSizeSpinners[i], &SpinnerWidget::spinnerDragAbort, signalMapperDragAbort, (void (QSignalMapper::*)())&QSignalMapper::map);

			signalMapperValueChanged->setMapping(simCellSizeSpinners[i], i);
			signalMapperDragStart->setMapping(simCellSizeSpinners[i], i);
			signalMapperDragStop->setMapping(simCellSizeSpinners[i], i);
			signalMapperDragAbort->setMapping(simCellSizeSpinners[i], i);
		}
		connect(signalMapperValueChanged, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, &SimulationCellEditor::onSizeSpinnerValueChanged);
		connect(signalMapperDragStart, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, &SimulationCellEditor::onSizeSpinnerDragStart);
		connect(signalMapperDragStop, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, &SimulationCellEditor::onSizeSpinnerDragStop);
		connect(signalMapperDragAbort, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, &SimulationCellEditor::onSizeSpinnerDragAbort);
		layout2->addWidget(new QLabel(tr("Width (X):")), 0, 0);
		layout2->addWidget(new QLabel(tr("Length (Y):")), 1, 0);
		layout2->addWidget(new QLabel(tr("Height (Z):")), 2, 0);

		connect(this, &SimulationCellEditor::contentsChanged, this, &SimulationCellEditor::updateSimulationBoxSize);
	}

	{
		QGroupBox* vectorsGroupBox = new QGroupBox(tr("Cell geometry"), rollout);
		layout1->addWidget(vectorsGroupBox);

		QVBoxLayout* sublayout = new QVBoxLayout(vectorsGroupBox);
		sublayout->setContentsMargins(4,4,4,4);
		sublayout->setSpacing(2);

		QString xyz[3] = { QStringLiteral("X: "), QStringLiteral("Y: "), QStringLiteral("Z: ") };

		{	// First cell vector.
			sublayout->addSpacing(6);
			sublayout->addWidget(new QLabel(tr("Cell vector 1:"), rollout));
			QGridLayout* layout2 = new QGridLayout();
			layout2->setContentsMargins(0,0,0,0);
			layout2->setSpacing(0);
			sublayout->addLayout(layout2);
			for(int i = 0; i < 3; i++) {
				Vector3ParameterUI* vPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(SimulationCellObject::cellVector1), i);
				layout2->addLayout(vPUI->createFieldLayout(), 0, i*2);
				layout2->setColumnStretch(i*2, 1);
				if(i != 2)
					layout2->setColumnMinimumWidth(i*2+1, 6);
			}
		}

		{	// Second cell vector.
			sublayout->addSpacing(2);
			sublayout->addWidget(new QLabel(tr("Cell vector 2:"), rollout));
			QGridLayout* layout2 = new QGridLayout();
			layout2->setContentsMargins(0,0,0,0);
			layout2->setSpacing(0);
			sublayout->addLayout(layout2);
			for(int i = 0; i < 3; i++) {
				Vector3ParameterUI* vPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(SimulationCellObject::cellVector2), i);
				layout2->addLayout(vPUI->createFieldLayout(), 0, i*2);
				layout2->setColumnStretch(i*2, 1);
				if(i != 2)
					layout2->setColumnMinimumWidth(i*2+1, 6);
			}
		}

		{	// Third cell vector.
			sublayout->addSpacing(2);
			sublayout->addWidget(new QLabel(tr("Cell vector 3:"), rollout));
			QGridLayout* layout2 = new QGridLayout();
			layout2->setContentsMargins(0,0,0,0);
			layout2->setSpacing(0);
			sublayout->addLayout(layout2);
			for(int i = 0; i < 3; i++) {
				Vector3ParameterUI* vPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(SimulationCellObject::cellVector3), i);
				zvectorPUI[i] = vPUI;
				layout2->addLayout(vPUI->createFieldLayout(), 0, i*2);
				layout2->setColumnStretch(i*2, 1);
				if(i != 2)
					layout2->setColumnMinimumWidth(i*2+1, 6);
			}
		}

		{	// Cell origin.
			sublayout->addSpacing(8);
			sublayout->addWidget(new QLabel(tr("Cell origin:"), rollout));
			QGridLayout* layout2 = new QGridLayout();
			layout2->setContentsMargins(0,0,0,0);
			layout2->setSpacing(0);
			sublayout->addLayout(layout2);
			for(int i = 0; i < 3; i++) {
				Vector3ParameterUI* vPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(SimulationCellObject::cellOrigin), i);
				if(i == 2) zoriginPUI = vPUI;
				layout2->addLayout(vPUI->createFieldLayout(), 0, i*2);
				layout2->setColumnStretch(i*2, 1);
				if(i != 2)
					layout2->setColumnMinimumWidth(i*2+1, 6);
			}
		}
	}
}

/******************************************************************************
* After the user has changed a spinner value, this method changes the
* simulation cell geometry.
******************************************************************************/
void SimulationCellEditor::changeSimulationBoxSize(int dim)
{
	OVITO_ASSERT(dim >=0 && dim < 3);

	SimulationCellObject* cell = static_object_cast<SimulationCellObject>(editObject());
	if(!cell) return;

	AffineTransformation cellTM = cell->cellMatrix();
	FloatType newSize = simCellSizeSpinners[dim]->floatValue();
	cellTM.column(3)[dim] -= 0.5 * (newSize - cellTM(dim, dim));
	cellTM(dim, dim) = newSize;
	cell->setCellMatrix(cellTM);
}

/******************************************************************************
* After the simulation cell size has changed, updates the UI controls.
******************************************************************************/
void SimulationCellEditor::updateSimulationBoxSize()
{
	SimulationCellObject* cell = static_object_cast<SimulationCellObject>(editObject());
	if(!cell) return;

	AffineTransformation cellTM = cell->cellMatrix();
	for(int dim = 0; dim < 3; dim++) {
		if(simCellSizeSpinners[dim]->isDragging() == false) {
			simCellSizeSpinners[dim]->setUnit(dataset()->unitsManager().worldUnit());
			simCellSizeSpinners[dim]->setFloatValue(cellTM(dim,dim));
		}
	}

	pbczPUI->setEnabled(!cell->is2D());
	simCellSizeSpinners[2]->setEnabled(!cell->is2D());
	zvectorPUI[0]->setEnabled(!cell->is2D());
	zvectorPUI[1]->setEnabled(!cell->is2D());
	zvectorPUI[2]->setEnabled(!cell->is2D());
	zoriginPUI->setEnabled(!cell->is2D());
}

/******************************************************************************
* Is called when a spinner's value has changed.
******************************************************************************/
void SimulationCellEditor::onSizeSpinnerValueChanged(int dim)
{
	ViewportSuspender noVPUpdate(dataset());
	if(!dataset()->undoStack().isRecording()) {
		undoableTransaction(tr("Change simulation cell size"), [this, dim]() {
			changeSimulationBoxSize(dim);
		});
	}
	else {
		dataset()->undoStack().resetCurrentCompoundOperation();
		changeSimulationBoxSize(dim);
	}
}

/******************************************************************************
* Is called when the user begins dragging a spinner interactively.
******************************************************************************/
void SimulationCellEditor::onSizeSpinnerDragStart(int dim)
{
	OVITO_ASSERT(!dataset()->undoStack().isRecording());
	dataset()->undoStack().beginCompoundOperation(tr("Change simulation cell size"));
}

/******************************************************************************
* Is called when the user stops dragging a spinner interactively.
******************************************************************************/
void SimulationCellEditor::onSizeSpinnerDragStop(int dim)
{
	OVITO_ASSERT(dataset()->undoStack().isRecording());
	dataset()->undoStack().endCompoundOperation();
}

/******************************************************************************
* Is called when the user aborts dragging a spinner interactively.
******************************************************************************/
void SimulationCellEditor::onSizeSpinnerDragAbort(int dim)
{
	OVITO_ASSERT(dataset()->undoStack().isRecording());
	dataset()->undoStack().endCompoundOperation(false);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
