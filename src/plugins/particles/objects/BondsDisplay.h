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

#ifndef __OVITO_BONDS_DISPLAY_H
#define __OVITO_BONDS_DISPLAY_H

#include <plugins/particles/Particles.h>
#include <core/scene/objects/DisplayObject.h>
#include <core/scene/objects/WeakVersionedObjectReference.h>
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/SceneRenderer.h>
#include "BondsObject.h"
#include "ParticlePropertyObject.h"
#include "BondPropertyObject.h"
#include "BondTypeProperty.h"
#include "SimulationCellObject.h"

namespace Ovito { namespace Particles {

/**
 * \brief A scene display object for bonds.
 */
class OVITO_PARTICLES_EXPORT BondsDisplay : public DisplayObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE BondsDisplay(DataSet* dataset);

	/// \brief Renders the associated data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the display bounding box of the data object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState) override;

	/// \brief Returns the display width of bonds.
	FloatType bondWidth() const { return _bondWidth; }

	/// \brief Sets the display width of bonds.
	void setBondWidth(FloatType newWidth) { _bondWidth = newWidth; }

	/// \brief Returns the selected shading mode for bonds.
	ArrowPrimitive::ShadingMode shadingMode() const { return _shadingMode; }

	/// \brief Sets the shading mode for bonds.
	void setShadingMode(ArrowPrimitive::ShadingMode mode) { _shadingMode = mode; }

	/// \brief Returns the selected rendering quality mode for bonds.
	ArrowPrimitive::RenderingQuality renderingQuality() const { return _renderingQuality; }

	/// \brief Sets the rendering quality mode for bonds.
	void setRenderingQuality(ArrowPrimitive::RenderingQuality quality) { _renderingQuality = quality; }

	/// Returns the display color for bonds.
	const Color& bondColor() const { return _bondColor; }

	/// Sets the display color for bonds.
	void setBondColor(const Color& color) { _bondColor = color; }

	/// Returns the display color used for selected bonds.
	Color selectionBondColor() const { return Color(1,0,0); }

	/// Returns whether bonds colors are derived from particle colors.
	bool useParticleColors() const { return _useParticleColors; }

	/// Controls whether bonds colors are derived from particle colors.
	void setUseParticleColors(bool enable) { _useParticleColors = enable; }

	/// Determines the display colors of bonds.
	void bondColors(std::vector<Color>& output, size_t particleCount, BondsObject* bondsObject,
			BondPropertyObject* bondColorProperty, BondTypeProperty* bondTypeProperty, BondPropertyObject* bondSelectionProperty,
			ParticleDisplay* particleDisplay, ParticlePropertyObject* particleColorProperty, ParticleTypeProperty* particleTypeProperty);

public:

    Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);
    Q_PROPERTY(Ovito::ArrowPrimitive::RenderingQuality renderingQuality READ renderingQuality WRITE setRenderingQuality);

protected:

	/// Controls the display width of bonds.
	PropertyField<FloatType> _bondWidth;

	/// Controls the color of the bonds.
	PropertyField<Color, QColor> _bondColor;

	/// Controls whether bonds colors are derived from particle colors.
	PropertyField<bool> _useParticleColors;

	/// Controls the shading mode for bonds.
	PropertyField<ArrowPrimitive::ShadingMode, int> _shadingMode;

	/// Controls the rendering quality mode for bonds.
	PropertyField<ArrowPrimitive::RenderingQuality, int> _renderingQuality;

	/// The buffered geometry used to render the bonds.
	std::shared_ptr<ArrowPrimitive> _buffer;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffer.
	SceneObjectCacheHelper<
		WeakVersionedOORef<BondsObject>,				// The bonds data object + revision number
		WeakVersionedOORef<ParticlePropertyObject>,		// Particle position property + revision number
		WeakVersionedOORef<ParticlePropertyObject>,		// Particle color property + revision number
		WeakVersionedOORef<ParticlePropertyObject>,		// Particle type property + revision number
		WeakVersionedOORef<BondPropertyObject>,			// Bond color property + revision number
		WeakVersionedOORef<BondPropertyObject>,			// Bond type property + revision number
		WeakVersionedOORef<BondPropertyObject>,			// Bond selection property + revision number
		WeakVersionedOORef<SimulationCellObject>,		// Simulation cell + revision number
		FloatType,										// Bond width
		Color,											// Bond color
		bool											// Use particle colors
	> _geometryCacheHelper;

	/// The bounding box that includes all bonds.
	Box3 _cachedBoundingBox;

	/// This helper structure is used to detect changes in the input data
	/// that require recomputing the bounding box.
	SceneObjectCacheHelper<
		WeakVersionedOORef<BondsObject>,				// The bonds data object + revision number
		WeakVersionedOORef<ParticlePropertyObject>,		// Particle position property + revision number
		WeakVersionedOORef<SimulationCellObject>,		// Simulation cell + revision number
		FloatType										// Bond width
	> _boundingBoxCacheHelper;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Bonds");

	DECLARE_PROPERTY_FIELD(_bondWidth);
	DECLARE_PROPERTY_FIELD(_bondColor);
	DECLARE_PROPERTY_FIELD(_useParticleColors);
	DECLARE_PROPERTY_FIELD(_shadingMode);
	DECLARE_PROPERTY_FIELD(_renderingQuality);
};

/**
 * \brief This information record is attached to the bonds by the BondsDisplay when rendering
 * them in the viewports. It facilitates the picking of bonds with the mouse.
 */
class OVITO_PARTICLES_EXPORT BondPickInfo : public ObjectPickInfo
{
public:

	/// Constructor.
	BondPickInfo(BondsObject* bondsObj, const PipelineFlowState& pipelineState) : _bondsObj(bondsObj), _pipelineState(pipelineState) {}

	/// The pipeline flow state containing the bonds.
	const PipelineFlowState& pipelineState() const { return _pipelineState; }

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(ObjectNode* objectNode, quint32 subobjectId) override;

private:

	/// The pipeline flow state containing the bonds.
	PipelineFlowState _pipelineState;

	/// The bonds data object.
	OORef<BondsObject> _bondsObj;

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace

#endif // __OVITO_BONDS_DISPLAY_H
