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

#ifndef __OVITO_PARTICLE_TYPE_H
#define __OVITO_PARTICLE_TYPE_H

#include <plugins/particles/Particles.h>
#include <core/reference/RefTarget.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores the properties of a particle type, e.g. name, color, and radius.
 *
 * \ingroup particles_objects
 */
class OVITO_PARTICLES_EXPORT ParticleType : public RefTarget
{
public:

	/// \brief Constructs a new particle type.
	Q_INVOKABLE ParticleType(DataSet* dataset);

	/// \brief Returns the identifier of the particle type.
	/// \return The type identifier.
	int id() const { return _id; }

	/// \brief Sets the identifier of the particle type.
	/// \param identifier The new identifier.
	/// \undoable
	void setId(int identifier) { _id = identifier; }

	/// \brief Gets the types's display name.
	/// \return The human-readable name of the particle type.
	/// \sa setName()
	const QString& name() const { return _name; }

	/// \brief Sets the types's display name.
	/// \param name The new human-readable name for this particle type.
	/// \undoable
	/// \sa name()
	void setName(const QString& name) { _name = name; }

	/// \brief Returns the display color that is assigned to the particles of this type.
	/// \return The color used for particles of this type.
	/// \sa setColor()
	Color color() const { return _color; }

	/// \brief Sets the display color of this particle type.
	/// \param color The new color to be used to display particles of this type.
	/// \undoable
	void setColor(const Color& color) { _color = color; }

	/// \brief Returns the radius of the particle type.
	/// \return The radius in world units.
	FloatType radius() const { return _radius; }

	/// \brief Sets the radius of the particle type.
	/// \param newRadius The radius in world units to be used to display this kind of particle.
	/// \undoable
	void setRadius(FloatType newRadius) { _radius = newRadius; }

	/// \brief Returns whether this type is enabled or disabled.
	/// This controls, e.g., the search for this structure type by structure identification modifiers.
	bool isEnabled() const { return _enabled; }

	/// \brief Sets whether this type is enabled or disabled.
	/// This controls, e.g., the search for this structure type by structure identification modifiers.
	void setEnabled(bool enabled) { _enabled = enabled; }

	// From RefTarget class:

	/// Returns the title of this object.
	virtual QString objectTitle() override { return name(); }

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

protected:

	/// Stores the identifier of the particle type.
	PropertyField<int> _id;

	/// The name of this particle type.
	PropertyField<QString, QString, ReferenceEvent::TitleChanged> _name;

	/// Stores the color of the particle type.
	PropertyField<Color, QColor> _color;

	/// Stores the radius of the particle type.
	PropertyField<FloatType> _radius;

	/// Stores whether this type is enabled or disabled.
	/// This controls, e.g., the search for this structure type by structure identification modifiers.
	PropertyField<bool> _enabled;

private:

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_PROPERTY_FIELD(_id);
	DECLARE_PROPERTY_FIELD(_name);
	DECLARE_PROPERTY_FIELD(_color);
	DECLARE_PROPERTY_FIELD(_radius);
	DECLARE_PROPERTY_FIELD(_enabled);
};

}	// End of namespace
}	// End of namespace

#endif // __OVITO_PARTICLE_TYPE_H
