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

#include <plugins/particles/Particles.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/ParticleDisplay.h>
#include <plugins/particles/objects/VectorDisplay.h>
#include <plugins/particles/objects/SimulationCellDisplay.h>
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/particles/objects/SurfaceMeshDisplay.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/objects/BondPropertyObject.h>
#include <plugins/particles/objects/BondTypeProperty.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <core/utilities/io/CompressedTextWriter.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace boost::python;
using namespace PyScript;

template<class PropertyClass, bool ReadOnly>
dict PropertyObject__array_interface__(PropertyClass& p)
{
	dict ai;
	if(p.componentCount() == 1) {
		ai["shape"] = boost::python::make_tuple(p.size());
		if(p.stride() != p.dataTypeSize())
			ai["strides"] = boost::python::make_tuple(p.stride());
	}
	else if(p.componentCount() > 1) {
		ai["shape"] = boost::python::make_tuple(p.size(), p.componentCount());
		ai["strides"] = boost::python::make_tuple(p.stride(), p.dataTypeSize());
	}
	else throw Exception("Cannot access empty property from Python.");
	if(p.dataType() == qMetaTypeId<int>()) {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = str("<i") + str(sizeof(int));
#else
		ai["typestr"] = str(">i") + str(sizeof(int));
#endif
	}
	else if(p.dataType() == qMetaTypeId<FloatType>()) {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = str("<f") + str(sizeof(FloatType));
#else
		ai["typestr"] = str(">f") + str(sizeof(FloatType));
#endif
	}
	else throw Exception("Cannot access property of this data type from Python.");
	if(ReadOnly) {
		ai["data"] = boost::python::make_tuple(reinterpret_cast<std::intptr_t>(p.constData()), true);
	}
	else {
		ai["data"] = boost::python::make_tuple(reinterpret_cast<std::intptr_t>(p.data()), false);
	}
	ai["version"] = 3;
	return ai;
}

dict BondsObject__array_interface__(const BondsObject& p)
{
	dict ai;
	ai["shape"] = boost::python::make_tuple(p.storage()->size(), 2);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	ai["typestr"] = str("<u") + str(sizeof(unsigned int));
#else
	ai["typestr"] = str(">u") + str(sizeof(unsigned int));
#endif
	const unsigned int* data;
	if(!p.storage()->empty()) {
		data = &p.storage()->front().index1;
	}
	else {
		static const unsigned int null_data = 0;
		data = &null_data;
	}
	ai["data"] = boost::python::make_tuple(reinterpret_cast<std::intptr_t>(data), true);
	ai["strides"] = boost::python::make_tuple(sizeof(Bond), sizeof(unsigned int));
	ai["version"] = 3;
	return ai;
}

dict BondsObject__pbc_vectors(const BondsObject& p)
{
	dict ai;
	ai["shape"] = boost::python::make_tuple(p.storage()->size(), 3);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	ai["typestr"] = str("<i") + str(sizeof(int8_t));
#else
	ai["typestr"] = str(">i") + str(sizeof(int8_t));
#endif
	const int8_t* data;
	if(!p.storage()->empty()) {
		data = &p.storage()->front().pbcShift.x();
	}
	else {
		static const int8_t null_data = 0;
		data = &null_data;
	}
	ai["data"] = boost::python::make_tuple(reinterpret_cast<std::intptr_t>(data), true);
	ai["strides"] = boost::python::make_tuple(sizeof(Bond), sizeof(int8_t));
	ai["version"] = 3;
	return ai;
}

BOOST_PYTHON_MODULE(Particles)
{
	docstring_options docoptions(true, false, false);

	class_<ParticlePropertyReference>("ParticlePropertyReference", init<ParticleProperty::Type, optional<int>>())
		.def(init<const QString&, optional<int>>())
		.add_property("type", &ParticlePropertyReference::type, &ParticlePropertyReference::setType)
		.add_property("name", make_function(&ParticlePropertyReference::name, return_value_policy<copy_const_reference>()))
		.add_property("component", &ParticlePropertyReference::vectorComponent, &ParticlePropertyReference::setVectorComponent)
		.add_property("isNull", &ParticlePropertyReference::isNull)
		.def(self == other<ParticlePropertyReference>())
		.def("findInState", make_function(&ParticlePropertyReference::findInState, return_value_policy<ovito_object_reference>()))
		.def("__str__", &ParticlePropertyReference::nameWithComponent)
	;

	// Implement Python to ParticlePropertyReference conversion.
	auto convertible_ParticlePropertyReference = [](PyObject* obj_ptr) -> void* {
		if(obj_ptr == Py_None) return obj_ptr;
		if(extract<QString>(obj_ptr).check()) return obj_ptr;
		if(extract<ParticleProperty::Type>(obj_ptr).check()) return obj_ptr;
		return nullptr;
	};
	auto construct_ParticlePropertyReference = [](PyObject* obj_ptr, boost::python::converter::rvalue_from_python_stage1_data* data) {
		void* storage = ((boost::python::converter::rvalue_from_python_storage<ParticlePropertyReference>*)data)->storage.bytes;
		if(obj_ptr == Py_None) {
			new (storage) ParticlePropertyReference();
			data->convertible = storage;
			return;
		}
		extract<ParticleProperty::Type> enumExtract(obj_ptr);
		if(enumExtract.check()) {
			ParticleProperty::Type ptype = enumExtract();
			if(ptype == ParticleProperty::UserProperty)
				throw Exception("User-defined particle property without a name is not acceptable.");
			new (storage) ParticlePropertyReference(ptype);
			data->convertible = storage;
			return;
		}
		QStringList parts = extract<QString>(obj_ptr)().split(QChar('.'));
		if(parts.length() > 2)
			throw Exception("Too many dots in particle property name string.");
		else if(parts.length() == 0 || parts[0].isEmpty())
			throw Exception("Particle property name string is empty.");
		// Determine property type.
		QString name = parts[0];
		ParticleProperty::Type type = ParticleProperty::standardPropertyList().value(name, ParticleProperty::UserProperty);

		// Determine vector component.
		int component = -1;
		if(parts.length() == 2) {
			// First try to convert component to integer.
			bool ok;
			component = parts[1].toInt(&ok);
			if(!ok) {
				if(type != ParticleProperty::UserProperty) {
					// Perhaps the standard property's component name was used instead of an integer.
					const QString componentName = parts[1].toUpper();
					QStringList standardNames = ParticleProperty::standardPropertyComponentNames(type);
					component = standardNames.indexOf(componentName);
					if(component < 0)
						throw Exception(QString("Component name '%1' is not defined for particle property '%2'. Possible components are: %3").arg(parts[1]).arg(parts[0]).arg(standardNames.join(',')));
				}
				else {
					// Assume user-defined properties cannot be vectors.
					component = -1;
					name = parts.join(QChar('.'));
				}
			}
		}

		// Construct object.
		if(type == Particles::ParticleProperty::UserProperty)
			new (storage) ParticlePropertyReference(name, component);
		else
			new (storage) ParticlePropertyReference(type, component);
		data->convertible = storage;
	};
	converter::registry::push_back(convertible_ParticlePropertyReference, construct_ParticlePropertyReference, boost::python::type_id<ParticlePropertyReference>());

	class_<BondPropertyReference>("BondPropertyReference", init<BondProperty::Type, optional<int>>())
		.def(init<const QString&, optional<int>>())
		.add_property("type", &BondPropertyReference::type, &BondPropertyReference::setType)
		.add_property("name", make_function(&BondPropertyReference::name, return_value_policy<copy_const_reference>()))
		.add_property("component", &BondPropertyReference::vectorComponent, &BondPropertyReference::setVectorComponent)
		.add_property("isNull", &BondPropertyReference::isNull)
		.def(self == other<BondPropertyReference>())
		.def("findInState", make_function(&BondPropertyReference::findInState, return_value_policy<ovito_object_reference>()))
		.def("__str__", &BondPropertyReference::nameWithComponent)
	;

	// Implement Python to BondPropertyReference conversion.
	auto convertible_BondPropertyReference = [](PyObject* obj_ptr) -> void* {
		if(obj_ptr == Py_None) return obj_ptr;
		if(extract<QString>(obj_ptr).check()) return obj_ptr;
		if(extract<BondProperty::Type>(obj_ptr).check()) return obj_ptr;
		return nullptr;
	};
	auto construct_BondPropertyReference = [](PyObject* obj_ptr, boost::python::converter::rvalue_from_python_stage1_data* data) {
		void* storage = ((boost::python::converter::rvalue_from_python_storage<BondPropertyReference>*)data)->storage.bytes;
		if(obj_ptr == Py_None) {
			new (storage) BondPropertyReference();
			data->convertible = storage;
			return;
		}
		extract<BondProperty::Type> enumExtract(obj_ptr);
		if(enumExtract.check()) {
			BondProperty::Type ptype = enumExtract();
			if(ptype == BondProperty::UserProperty)
				throw Exception("User-defined bond property without a name is not acceptable.");
			new (storage) BondPropertyReference(ptype);
			data->convertible = storage;
			return;
		}
		QStringList parts = extract<QString>(obj_ptr)().split(QChar('.'));
		if(parts.length() > 2)
			throw Exception("Too many dots in bond property name string.");
		else if(parts.length() == 0 || parts[0].isEmpty())
			throw Exception("Particle bond name string is empty.");
		// Determine property type.
		QString name = parts[0];
		BondProperty::Type type = BondProperty::standardPropertyList().value(name, BondProperty::UserProperty);

		// Determine vector component.
		int component = -1;
		if(parts.length() == 2) {
			// First try to convert component to integer.
			bool ok;
			component = parts[1].toInt(&ok);
			if(!ok) {
				if(type != BondProperty::UserProperty) {
					// Perhaps the component name was used instead of an integer.
					const QString componentName = parts[1].toUpper();
					QStringList standardNames = BondProperty::standardPropertyComponentNames(type);
					component = standardNames.indexOf(componentName);
					if(component < 0)
						throw Exception(QString("Component name '%1' is not defined for bond property '%2'. Possible components are: %3").arg(parts[1]).arg(parts[0]).arg(standardNames.join(',')));
				}
				else {
					// Assume user-defined properties cannot be vectors.
					component = -1;
					name = parts.join(QChar('.'));
				}
			}
		}

		// Construct object.
		if(type == Particles::BondProperty::UserProperty)
			new (storage) BondPropertyReference(name, component);
		else
			new (storage) BondPropertyReference(type, component);
		data->convertible = storage;
	};
	converter::registry::push_back(convertible_BondPropertyReference, construct_BondPropertyReference, boost::python::type_id<BondPropertyReference>());

	{
		scope s = ovito_abstract_class<ParticlePropertyObject, DataObject>(
				":Base class: :py:class:`ovito.data.DataObject`\n\n"
				"A data object that stores the per-particle values of a particle property. "
				"\n\n"
				"The list of properties associated with a particle dataset can be access via the "
				":py:attr:`DataCollection.particle_properties` dictionary. The :py:attr:`.size` of a particle "
				"property is always equal to the number of particles in the dataset. The per-particle data "
				"of a property can be accessed as a NumPy array through the :py:attr:`.array` attribute. "
				"\n\n"
				"If you want to modify the property values, you have to use the :py:attr:`.marray` (*modifiable array*) "
				"attribute instead, which provides read/write access to the underlying per-particle data. "
				"After you are done modifying the property values, you should call :py:meth:`.changed` to inform "
				"the system that it needs to update any state that depends on the data. "
				,
				// Python class name:
				"ParticleProperty")
			.def("createUserProperty", &ParticlePropertyObject::createUserProperty)
			.def("createStandardProperty", &ParticlePropertyObject::createStandardProperty)
			.def("findInState", make_function((ParticlePropertyObject* (*)(const PipelineFlowState&, ParticleProperty::Type))&ParticlePropertyObject::findInState, return_value_policy<ovito_object_reference>()))
			.def("findInState", make_function((ParticlePropertyObject* (*)(const PipelineFlowState&, const QString&))&ParticlePropertyObject::findInState, return_value_policy<ovito_object_reference>()))
			.staticmethod("createUserProperty")
			.staticmethod("createStandardProperty")
			.staticmethod("findInState")
			.def("changed", &ParticlePropertyObject::changed,
					"Informs the particle property object that its internal data has changed. "
					"This function must be called after each direct modification of the per-particle data "
					"through the :py:attr:`.marray` attribute.\n\n"
					"Calling this method on an input particle property is necessary to invalidate data caches down the modification "
					"pipeline. Forgetting to call this method may result in an incomplete re-evaluation of the modification pipeline. "
					"See :py:attr:`.marray` for more information.")
			.def("nameWithComponent", &ParticlePropertyObject::nameWithComponent)
			.add_property("name", make_function(&ParticlePropertyObject::name, return_value_policy<copy_const_reference>()), &ParticlePropertyObject::setName,
					"The human-readable name of this particle property.")
			.add_property("__len__", &ParticlePropertyObject::size)
			.add_property("size", &ParticlePropertyObject::size, &ParticlePropertyObject::resize,
					"The number of particles.")
			.add_property("type", &ParticlePropertyObject::type, &ParticlePropertyObject::setType,
					".. _particle-types-list:"
					"\n\n"
					"The type of the particle property (user-defined or one of the standard types).\n"
					"One of the following constants:"
					"\n\n"
					"======================================================= =================================================== ========== ==================================\n"
					"Type constant                                           Property name                                       Data type  Component names\n"
					"======================================================= =================================================== ========== ==================================\n"
					"``ParticleProperty.Type.User``                          (a user-defined property with a non-standard name)  int/float  \n"
					"``ParticleProperty.Type.ParticleType``                  :guilabel:`Particle Type`                           int        \n"
					"``ParticleProperty.Type.Position``                      :guilabel:`Position`                                float      X, Y, Z\n"
					"``ParticleProperty.Type.Selection``                     :guilabel:`Selection`                               int        \n"
					"``ParticleProperty.Type.Color``                         :guilabel:`Color`                                   float      R, G, B\n"
					"``ParticleProperty.Type.Displacement``                  :guilabel:`Displacement`                            float      X, Y, Z\n"
					"``ParticleProperty.Type.DisplacementMagnitude``         :guilabel:`Displacement Magnitude`                  float      \n"
					"``ParticleProperty.Type.PotentialEnergy``               :guilabel:`Potential Energy`                        float      \n"
					"``ParticleProperty.Type.KineticEnergy``                 :guilabel:`Kinetic Energy`                          float      \n"
					"``ParticleProperty.Type.TotalEnergy``                   :guilabel:`Total Energy`                            float      \n"
					"``ParticleProperty.Type.Velocity``                      :guilabel:`Velocity`                                float      X, Y, Z\n"
					"``ParticleProperty.Type.Radius``                        :guilabel:`Radius`                                  float      \n"
					"``ParticleProperty.Type.Cluster``                       :guilabel:`Cluster`                                 int        \n"
					"``ParticleProperty.Type.Coordination``                  :guilabel:`Coordination`                            int        \n"
					"``ParticleProperty.Type.StructureType``                 :guilabel:`Structure Type`                          int        \n"
					"``ParticleProperty.Type.Identifier``                    :guilabel:`Particle Identifier`                     int        \n"
					"``ParticleProperty.Type.StressTensor``                  :guilabel:`Stress Tensor`                           float      XX, YY, ZZ, XY, XZ, YZ\n"
					"``ParticleProperty.Type.StrainTensor``                  :guilabel:`Strain Tensor`                           float      XX, YY, ZZ, XY, XZ, YZ\n"
					"``ParticleProperty.Type.DeformationGradient``           :guilabel:`Deformation Gradient`                    float      11, 21, 31, 12, 22, 32, 13, 23, 33\n"
					"``ParticleProperty.Type.Orientation``                   :guilabel:`Orientation`                             float      X, Y, Z, W\n"
					"``ParticleProperty.Type.Force``                         :guilabel:`Force`                                   float      X, Y, Z\n"
					"``ParticleProperty.Type.Mass``                          :guilabel:`Mass`                                    float      \n"
					"``ParticleProperty.Type.Charge``                        :guilabel:`Charge`                                  float      \n"
					"``ParticleProperty.Type.PeriodicImage``                 :guilabel:`Periodic Image`                          int        X, Y, Z\n"
					"``ParticleProperty.Type.Transparency``                  :guilabel:`Transparency`                            float      \n"
					"``ParticleProperty.Type.DipoleOrientation``             :guilabel:`Dipole Orientation`                      float      X, Y, Z\n"
					"``ParticleProperty.Type.DipoleMagnitude``               :guilabel:`Dipole Magnitude`                        float      \n"
					"``ParticleProperty.Type.AngularVelocity``               :guilabel:`Angular Velocity`                        float      X, Y, Z\n"
					"``ParticleProperty.Type.AngularMomentum``               :guilabel:`Angular Momentum`                        float      X, Y, Z\n"
					"``ParticleProperty.Type.Torque``                        :guilabel:`Torque`                                  float      X, Y, Z\n"
					"``ParticleProperty.Type.Spin``                          :guilabel:`Spin`                                    float      \n"
					"``ParticleProperty.Type.CentroSymmetry``                :guilabel:`Centrosymmetry`                          float      \n"
					"``ParticleProperty.Type.VelocityMagnitude``             :guilabel:`Velocity Magnitude`                      float      \n"
					"``ParticleProperty.Type.Molecule``                      :guilabel:`Molecule Identifier`                     int        \n"
					"``ParticleProperty.Type.AsphericalShape``               :guilabel:`Aspherical Shape`                        float      X, Y, Z\n"
					"``ParticleProperty.Type.VectorColor``                   :guilabel:`Vector Color`                            float      R, G, B\n"
					"``ParticleProperty.Type.ElasticStrainTensor``           :guilabel:`Elastic Strain`                          float      XX, YY, ZZ, XY, XZ, YZ\n"
					"``ParticleProperty.Type.ElasticDeformationGradient``    :guilabel:`Elastic Deformation Gradient`            float      XX, YX, ZX, XY, YY, ZY, XZ, YZ, ZZ\n"
					"``ParticleProperty.Type.Rotation``                      :guilabel:`Rotation`                                float      X, Y, Z, W\n"
					"``ParticleProperty.Type.StretchTensor``                 :guilabel:`Stretch Tensor`                          float      XX, YY, ZZ, XY, XZ, YZ\n"
					"``ParticleProperty.Type.MoleculeType``                  :guilabel:`Molecule Type`                           int        \n"
					"======================================================= =================================================== ========== ==================================\n"
					)
			.add_property("dataType", &ParticlePropertyObject::dataType)
			.add_property("dataTypeSize", &ParticlePropertyObject::dataTypeSize)
			.add_property("stride", &ParticlePropertyObject::stride)
			.add_property("components", &ParticlePropertyObject::componentCount,
					"The number of vector components (if this is a vector particle property); otherwise 1 (= scalar property).")
			.add_property("__array_interface__", &PropertyObject__array_interface__<ParticlePropertyObject,true>)
			.add_property("__mutable_array_interface__", &PropertyObject__array_interface__<ParticlePropertyObject,false>)
		;

		enum_<ParticleProperty::Type>("Type")
			.value("User", ParticleProperty::UserProperty)
			.value("ParticleType", ParticleProperty::ParticleTypeProperty)
			.value("Position", ParticleProperty::PositionProperty)
			.value("Selection", ParticleProperty::SelectionProperty)
			.value("Color", ParticleProperty::ColorProperty)
			.value("Displacement", ParticleProperty::DisplacementProperty)
			.value("DisplacementMagnitude", ParticleProperty::DisplacementMagnitudeProperty)
			.value("PotentialEnergy", ParticleProperty::PotentialEnergyProperty)
			.value("KineticEnergy", ParticleProperty::KineticEnergyProperty)
			.value("TotalEnergy", ParticleProperty::TotalEnergyProperty)
			.value("Velocity", ParticleProperty::VelocityProperty)
			.value("Radius", ParticleProperty::RadiusProperty)
			.value("Cluster", ParticleProperty::ClusterProperty)
			.value("Coordination", ParticleProperty::CoordinationProperty)
			.value("StructureType", ParticleProperty::StructureTypeProperty)
			.value("Identifier", ParticleProperty::IdentifierProperty)
			.value("StressTensor", ParticleProperty::StressTensorProperty)
			.value("StrainTensor", ParticleProperty::StrainTensorProperty)
			.value("DeformationGradient", ParticleProperty::DeformationGradientProperty)
			.value("Orientation", ParticleProperty::OrientationProperty)
			.value("Force", ParticleProperty::ForceProperty)
			.value("Mass", ParticleProperty::MassProperty)
			.value("Charge", ParticleProperty::ChargeProperty)
			.value("PeriodicImage", ParticleProperty::PeriodicImageProperty)
			.value("Transparency", ParticleProperty::TransparencyProperty)
			.value("DipoleOrientation", ParticleProperty::DipoleOrientationProperty)
			.value("DipoleMagnitude", ParticleProperty::DipoleMagnitudeProperty)
			.value("AngularVelocity", ParticleProperty::AngularVelocityProperty)
			.value("AngularMomentum", ParticleProperty::AngularMomentumProperty)
			.value("Torque", ParticleProperty::TorqueProperty)
			.value("Spin", ParticleProperty::SpinProperty)
			.value("CentroSymmetry", ParticleProperty::CentroSymmetryProperty)
			.value("VelocityMagnitude", ParticleProperty::VelocityMagnitudeProperty)
			.value("Molecule", ParticleProperty::MoleculeProperty)
			.value("AsphericalShape", ParticleProperty::AsphericalShapeProperty)
			.value("VectorColor", ParticleProperty::VectorColorProperty)
			.value("ElasticStrainTensor", ParticleProperty::ElasticStrainTensorProperty)
			.value("ElasticDeformationGradient", ParticleProperty::ElasticDeformationGradientProperty)
			.value("Rotation", ParticleProperty::RotationProperty)
			.value("StretchTensor", ParticleProperty::StretchTensorProperty)
			.value("MoleculeType", ParticleProperty::MoleculeTypeProperty)
		;
	}

	ovito_abstract_class<ParticleTypeProperty, ParticlePropertyObject>(
			":Base class: :py:class:`ovito.data.ParticleProperty`\n\n"
			"This is a specialization of the :py:class:`ParticleProperty` class, which holds a list of :py:class:`ParticleType` instances in addition "
			"to the per-particle type values. "
			"\n\n"
			"OVITO encodes the types of particles (chemical and also others) as integer values starting at 1. "
			"Like for any other particle property, the numeric type of each particle can be accessed as a NumPy array through the :py:attr:`~ParticleProperty.array` attribute "
			"of the base class, or modified through the mutable :py:attr:`~ParticleProperty.marray` NumPy interface:: "
			"\n\n"
		    "    >>> type_property = node.source.particle_properties.particle_type\n"
			"    >>> print(type_property.array)\n"
			"    [1 3 2 ..., 2 1 2]\n"
			"\n\n"
			"In addition to these per-particle type values, the :py:class:`!ParticleTypeProperty` class keeps the :py:attr:`.type_list`, which "
			"contains all defined particle types including their names, IDs, display color and radius. "
			"Each defined type is represented by an :py:attr:`ParticleType` instance and has a unique integer ID, a human-readable name (e.g. the chemical symbol) "
			"and a display color and radius:: "
			"\n\n"
			"    >>> for t in type_property.type_list:\n"
			"    ...     print(t.id, t.name, t.color, t.radius)\n"
			"    ... \n"
			"    1 N (0.188 0.313 0.972) 0.74\n"
			"    2 C (0.564 0.564 0.564) 0.77\n"
			"    3 O (1 0.050 0.050) 0.74\n"
			"    4 S (0.97 0.97 0.97) 0.0\n"
			"\n\n"
			"Each particle type has a unique numeric ID (typically starting at 1). Note that, in this particular example, types were stored in order of ascending ID in the "
			":py:attr:`.type_list`. This may not always be the case. To quickly look up the :py:class:`ParticleType` and its name for a given ID, "
			"the :py:meth:`.get_type_by_id` method is available:: "
			"\n\n"
			"    >>> for t in type_property.array:\n"
			"    ...     print(type_property.get_type_by_id(t).name)\n"
			"    ... \n"
			"    N\n"
			"    O\n"
			"    C\n"
			"\n\n"
			"Conversely, the :py:attr:`ParticleType` and its numeric ID can be looked by name using the :py:meth:`.get_type_by_name` method. "
			"For example, to count the number of oxygen atoms in a system:"
			"\n\n"
			"    >>> O_type_id = type_property.get_type_by_name('O').id\n"
			"    >>> numpy.count_nonzero(type_property.array == O_type_id)\n"
			"    957\n"
			"\n\n"
			"Note that particles may be associated with multiple kinds of types in OVITO. This includes, for example, the chemical type and the structural type. "
			"Thus, several type classifications of particles can co-exist, each being represented by a separate instance of the :py:class:`!ParticleTypeProperty` class and a separate :py:attr:`.type_list`. "
			"For example, while the ``'Particle Type'`` property stores the chemical type of "
			"atoms (e.g. C, H, Fe, ...), the ``'Structure Type'`` property stores the structural type computed for each atom (e.g. FCC, BCC, ...). ")
		.def("addParticleType", &ParticleTypeProperty::addParticleType)
		.def("insertParticleType", &ParticleTypeProperty::insertParticleType)
		.def("_get_type_by_id", make_function((ParticleType* (ParticleTypeProperty::*)(int) const)&ParticleTypeProperty::particleType, return_value_policy<ovito_object_reference>()))
		.def("_get_type_by_name", make_function((ParticleType* (ParticleTypeProperty::*)(const QString&) const)&ParticleTypeProperty::particleType, return_value_policy<ovito_object_reference>()))
		.def("removeParticleType", &ParticleTypeProperty::removeParticleType)
		.def("clearParticleTypes", &ParticleTypeProperty::clearParticleTypes)
		.add_property("particleTypes", make_function(&ParticleTypeProperty::particleTypes, return_internal_reference<>()))
		.def("getDefaultParticleColorFromId", &ParticleTypeProperty::getDefaultParticleColorFromId)
		.def("getDefaultParticleColor", &ParticleTypeProperty::getDefaultParticleColor)
		.def("setDefaultParticleColor", &ParticleTypeProperty::setDefaultParticleColor)
		.def("getDefaultParticleRadius", &ParticleTypeProperty::getDefaultParticleRadius)
		.def("setDefaultParticleRadius", &ParticleTypeProperty::setDefaultParticleRadius)
		.staticmethod("getDefaultParticleColorFromId")
		.staticmethod("getDefaultParticleColor")
		.staticmethod("getDefaultParticleRadius")
		.staticmethod("setDefaultParticleColor")
		.staticmethod("setDefaultParticleRadius")
	;

	ovito_class<SimulationCellObject, DataObject>(
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"Stores the shape and the boundary conditions of the simulation cell."
			"\n\n"
			"Each instance of this class is associated with a corresponding :py:class:`~ovito.vis.SimulationCellDisplay` "
			"that controls the visual appearance of the simulation cell. It can be accessed through "
			"the :py:attr:`~DataObject.display` attribute of the :py:class:`!SimulationCell` object, which is defined by the :py:class:`~DataObject` base class."
			"\n\n"
			"The simulation cell of a particle dataset can be accessed via the :py:attr:`DataCollection.cell` property."
			"\n\n"
			"Example:\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell.py\n",
			// Python class name:
			"SimulationCell")
		.add_property("pbc_x", &SimulationCellObject::pbcX, &SimulationCellObject::setPbcX)
		.add_property("pbc_y", &SimulationCellObject::pbcY, &SimulationCellObject::setPbcY)
		.add_property("pbc_z", &SimulationCellObject::pbcZ, &SimulationCellObject::setPbcZ)
		.add_property("is2D", &SimulationCellObject::is2D, &SimulationCellObject::set2D,
				"Specifies whether the system is two-dimensional (true) or three-dimensional (false). "
				"For two-dimensional systems the PBC flag in the third direction (z) and the third cell vector are ignored. "
				"\n\n"
				":Default: ``false``\n")
		.add_property("cellMatrix", &SimulationCellObject::cellMatrix, &SimulationCellObject::setCellMatrix)
		.add_property("vector1", make_function(&SimulationCellObject::edgeVector1, return_value_policy<copy_const_reference>()), &SimulationCellObject::setEdgeVector1)
		.add_property("vector2", make_function(&SimulationCellObject::edgeVector2, return_value_policy<copy_const_reference>()), &SimulationCellObject::setEdgeVector2)
		.add_property("vector3", make_function(&SimulationCellObject::edgeVector3, return_value_policy<copy_const_reference>()), &SimulationCellObject::setEdgeVector3)
		.add_property("origin", make_function(&SimulationCellObject::origin, return_value_policy<copy_const_reference>()), &SimulationCellObject::setOrigin)
		.add_property("volume", &SimulationCellObject::volume3D,
				"Returns the volume of the three-dimensional simulation cell.\n"
				"It is the absolute value of the determinant of the cell matrix.")
		.add_property("volume2D", &SimulationCellObject::volume2D,
				"Returns the volume of the two-dimensional simulation cell (see :py:attr:`.is2D`).\n")
	;

	{
		scope s = ovito_class<BondsObject, DataObject>(
				":Base class: :py:class:`ovito.data.DataObject`\n\n"
				"This data object stores a list of bonds between pairs of particles. "
				"Typically bonds are loaded from a simulation file or are created using the :py:class:`~.ovito.modifiers.CreateBondsModifier` in the modification pipeline."
				"\n\n"
				"The following example shows how to access the bond list create by a :py:class:`~.ovito.modifiers.CreateBondsModifier`:\n"
				"\n"
				".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
				"   :lines: 1-14\n"
				"\n"
				"OVITO represents each bond as two half-bonds, one pointing from a particle *A* to a particle *B*, and "
				"the other half-bond pointing back from *B* to *A*. Thus, for a given number of bonds, you will find twice as many half-bonds in the :py:class:`!Bonds` object. \n"
				"The :py:attr:`.array` attribute returns a (read-only) NumPy array that contains the list of half-bonds, which are "
				"defined by pairs of particle indices (the first one specifying the particle the half-bond is pointing away from)."
				"\n\n"
				"Furthermore, every :py:class:`!Bonds` object is associated with a :py:class:`~ovito.vis.BondsDisplay` instance, "
				"which controls the visual appearance of the bonds. It can be accessed through the :py:attr:`~DataObject.display` attribute:\n"
				"\n"
				".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
				"   :lines: 16-\n",
				// Python class name:
				"Bonds")
			.add_property("__array_interface__", &BondsObject__array_interface__)
			.add_property("_pbc_vectors", &BondsObject__pbc_vectors)
			.def("clear", &BondsObject::clear,
					"Removes all stored bonds.")
			.def("addBond", &BondsObject::addBond)
			.add_property("size", &BondsObject::size)
		;

		class_<ParticleBondMap, boost::shared_ptr<ParticleBondMap>, boost::noncopyable>("ParticleBondMap", no_init)
			.def("__init__", make_constructor(lambda_address([](BondsObject* bonds) { return boost::make_shared<ParticleBondMap>(*bonds->storage()); }),
					default_call_policies(), (arg("bonds"))))
			.def("firstBondOfParticle", &ParticleBondMap::firstBondOfParticle)
			.def("nextBondOfParticle", &ParticleBondMap::nextBondOfParticle)
			.add_property("endOfListValue", &ParticleBondMap::endOfListValue)
		;
	}

	ovito_class<ParticleType, RefTarget>(
			"Stores the properties of a particle type or atom type."
			"\n\n"
			"The list of particle types is stored in the :py:class:`~ovito.data.ParticleTypeProperty` class.")
		.add_property("id", &ParticleType::id, &ParticleType::setId,
				"The identifier of the particle type.")
		.add_property("color", &ParticleType::color, &ParticleType::setColor,
				"The display color to use for particles of this type.")
		.add_property("radius", &ParticleType::radius, &ParticleType::setRadius,
				"The display radius to use for particles of this type.")
		.add_property("name", make_function(&ParticleType::name, return_value_policy<copy_const_reference>()), &ParticleType::setName,
				"The display name of this particle type.")
	;

	class_<QVector<ParticleType*>, boost::noncopyable>("QVector<ParticleType*>", no_init)
		.def(QVector_OO_readonly_indexing_suite<ParticleType>())
	;
	python_to_container_conversion<QVector<ParticleType*>>();

	{
		scope s = ovito_class<ParticleDisplay, DisplayObject>(
				":Base class: :py:class:`ovito.vis.Display`\n\n"
				"This object controls the visual appearance of particles."
				"\n\n"
				"An instance of this class is attached to the ``Position`` :py:class:`~ovito.data.ParticleProperty` "
				"and can be accessed via its :py:attr:`~ovito.data.DataObject.display` property. "
				"\n\n"
				"For example, the following script demonstrates how to change the display shape of particles to a square:"
				"\n\n"
				".. literalinclude:: ../example_snippets/particle_display.py\n")
			.add_property("radius", &ParticleDisplay::defaultParticleRadius, &ParticleDisplay::setDefaultParticleRadius,
					"The standard display radius of particles. "
					"This value is only used if no per-particle or per-type radii have been set. "
					"A per-type radius can be set via :py:attr:`ovito.data.ParticleType.radius`. "
					"An individual display radius can be assigned to particles by creating a ``Radius`` "
					":py:class:`~ovito.data.ParticleProperty`, e.g. using the :py:class:`~ovito.modifiers.ComputePropertyModifier`. "
					"\n\n"
					":Default: 1.2\n")
			.add_property("default_color", &ParticleDisplay::defaultParticleColor)
			.add_property("selection_color", &ParticleDisplay::selectionParticleColor)
			.add_property("rendering_quality", &ParticleDisplay::renderingQuality, &ParticleDisplay::setRenderingQuality)
			.add_property("shape", &ParticleDisplay::particleShape, &ParticleDisplay::setParticleShape,
					"The display shape of particles.\n"
					"Possible values are:"
					"\n\n"
					"   * ``ParticleDisplay.Shape.Sphere`` (default) \n"
					"   * ``ParticleDisplay.Shape.Box``\n"
					"   * ``ParticleDisplay.Shape.Circle``\n"
					"   * ``ParticleDisplay.Shape.Square``\n"
					"   * ``ParticleDisplay.Shape.Cylinder``\n"
					"   * ``ParticleDisplay.Shape.Spherocylinder``\n"
					"\n")
			;

		enum_<ParticleDisplay::ParticleShape>("Shape")
			.value("Sphere", ParticleDisplay::Sphere)
			.value("Box", ParticleDisplay::Box)
			.value("Circle", ParticleDisplay::Circle)
			.value("Square", ParticleDisplay::Square)
			.value("Cylinder", ParticleDisplay::Cylinder)
			.value("Spherocylinder", ParticleDisplay::Spherocylinder)
		;
	}

	{
		scope s = ovito_class<VectorDisplay, DisplayObject>(
				":Base class: :py:class:`ovito.vis.Display`\n\n"
				"Controls the visual appearance of vectors (arrows)."
				"\n\n"
				"An instance of this class is attached to particle properties "
				"like for example the ``Displacement`` property, which represent vector quantities. "
				"It can be accessed via the :py:attr:`~ovito.data.DataObject.display` property of the :py:class:`~ovito.data.ParticleProperty` class. "
				"\n\n"
				"For example, the following script demonstrates how to change the display color of force vectors loaded from an input file:"
				"\n\n"
				".. literalinclude:: ../example_snippets/vector_display.py\n")
			.add_property("shading", &VectorDisplay::shadingMode, &VectorDisplay::setShadingMode,
					"The shading style used for the arrows.\n"
					"Possible values:"
					"\n\n"
					"   * ``VectorDisplay.Shading.Normal`` (default) \n"
					"   * ``VectorDisplay.Shading.Flat``\n"
					"\n")
			.add_property("renderingQuality", &VectorDisplay::renderingQuality, &VectorDisplay::setRenderingQuality)
			.add_property("reverse", &VectorDisplay::reverseArrowDirection, &VectorDisplay::setReverseArrowDirection,
					"Boolean flag controlling the reversal of arrow directions."
					"\n\n"
					":Default: ``False``\n")
			.add_property("alignment", &VectorDisplay::arrowPosition, &VectorDisplay::setArrowPosition,
					"Controls the positioning of arrows with respect to the particles.\n"
					"Possible values:"
					"\n\n"
					"   * ``VectorDisplay.Alignment.Base`` (default) \n"
					"   * ``VectorDisplay.Alignment.Center``\n"
					"   * ``VectorDisplay.Alignment.Head``\n"
					"\n")
			.add_property("color", make_function(&VectorDisplay::arrowColor, return_value_policy<copy_const_reference>()), &VectorDisplay::setArrowColor,
					"The display color of arrows."
					"\n\n"
					":Default: ``(1.0, 1.0, 0.0)``\n")
			.add_property("width", &VectorDisplay::arrowWidth, &VectorDisplay::setArrowWidth,
					"Controls the width of arrows (in natural length units)."
					"\n\n"
					":Default: 0.5\n")
			.add_property("scaling", &VectorDisplay::scalingFactor, &VectorDisplay::setScalingFactor,
					"The uniform scaling factor applied to vectors."
					"\n\n"
					":Default: 1.0\n")
		;

		enum_<VectorDisplay::ArrowPosition>("Alignment")
			.value("Base", VectorDisplay::Base)
			.value("Center", VectorDisplay::Center)
			.value("Head", VectorDisplay::Head)
		;
	}

	ovito_class<SimulationCellDisplay, DisplayObject>(
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of :py:class:`~ovito.data.SimulationCell` objects."
			"The following script demonstrates how to change the line width of the simulation cell:"
			"\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell_display.py\n")
		.add_property("line_width", &SimulationCellDisplay::simulationCellLineWidth, &SimulationCellDisplay::setSimulationCellLineWidth,
				"The width of the simulation cell line (in simulation units of length)."
				"\n\n"
				":Default: 0.14% of the simulation box diameter\n")
		.add_property("render_cell", &SimulationCellDisplay::renderSimulationCell, &SimulationCellDisplay::setRenderSimulationCell,
				"Boolean flag controlling the cell's visibility in rendered images. "
				"If ``False``, the cell will only be visible in the interactive viewports. "
				"\n\n"
				":Default: ``True``\n")
		.add_property("rendering_color", &SimulationCellDisplay::simulationCellRenderingColor, &SimulationCellDisplay::setSimulationCellRenderingColor,
				"The line color used when rendering the cell."
				"\n\n"
				":Default: ``(0, 0, 0)``\n")
	;

	ovito_class<SurfaceMeshDisplay, DisplayObject>(
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of a surface mesh computed by the :py:class:`~ovito.modifiers.ConstructSurfaceModifier`.")
		.add_property("surface_color", make_function(&SurfaceMeshDisplay::surfaceColor, return_value_policy<copy_const_reference>()), &SurfaceMeshDisplay::setSurfaceColor,
				"The display color of the surface mesh."
				"\n\n"
				":Default: ``(1.0, 1.0, 1.0)``\n")
		.add_property("cap_color", make_function(&SurfaceMeshDisplay::capColor, return_value_policy<copy_const_reference>()), &SurfaceMeshDisplay::setCapColor,
				"The display color of the cap polygons at periodic boundaries."
				"\n\n"
				":Default: ``(0.8, 0.8, 1.0)``\n")
		.add_property("show_cap", &SurfaceMeshDisplay::showCap, &SurfaceMeshDisplay::setShowCap,
				"Controls the visibility of cap polygons, which are created at the intersection of the surface mesh with periodic box boundaries."
				"\n\n"
				":Default: ``True``\n")
		.add_property("surface_transparency", &SurfaceMeshDisplay::surfaceTransparency, &SurfaceMeshDisplay::setSurfaceTransparency,
				"The level of transparency of the displayed surface. Valid range is 0.0 -- 1.0."
				"\n\n"
				":Default: 0.0\n")
		.add_property("cap_transparency", &SurfaceMeshDisplay::capTransparency, &SurfaceMeshDisplay::setCapTransparency,
				"The level of transparency of the displayed cap polygons. Valid range is 0.0 -- 1.0."
				"\n\n"
				":Default: 0.0\n")
		.add_property("smooth_shading", &SurfaceMeshDisplay::smoothShading, &SurfaceMeshDisplay::setSmoothShading,
				"Enables smooth shading of the triangulated surface mesh."
				"\n\n"
				":Default: ``True``\n")
		.add_property("reverse_orientation", &SurfaceMeshDisplay::reverseOrientation, &SurfaceMeshDisplay::setReverseOrientation,
				"Flips the orientation of the surface. This affects the generation of cap polygons."
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_class<BondsDisplay, DisplayObject>(
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of particle bonds. An instance of this class is attached to every :py:class:`~ovito.data.Bonds` data object.")
		.add_property("width", &BondsDisplay::bondWidth, &BondsDisplay::setBondWidth,
				"The display width of bonds (in natural length units)."
				"\n\n"
				":Default: 0.4\n")
		.add_property("color", make_function(&BondsDisplay::bondColor, return_value_policy<copy_const_reference>()), &BondsDisplay::setBondColor,
				"The display color of bonds. Used only if :py:attr:`.use_particle_colors` == False."
				"\n\n"
				":Default: ``(0.6, 0.6, 0.6)``\n")
		.add_property("shading", &BondsDisplay::shadingMode, &BondsDisplay::setShadingMode,
				"The shading style used for bonds.\n"
				"Possible values:"
				"\n\n"
				"   * ``BondsDisplay.Shading.Normal`` (default) \n"
				"   * ``BondsDisplay.Shading.Flat``\n"
				"\n")
		.add_property("renderingQuality", &BondsDisplay::renderingQuality, &BondsDisplay::setRenderingQuality)
		.add_property("use_particle_colors", &BondsDisplay::useParticleColors, &BondsDisplay::setUseParticleColors,
				"If ``True``, bonds are assigned the same color as the particles they are adjacent to."
				"\n\n"
				":Default: ``True``\n")
	;

	ovito_class<SurfaceMesh, DataObject>(
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"This data object stores the surface mesh computed by a :py:class:`~ovito.modifiers.ConstructSurfaceModifier`. "
			"\n\n"
			"Currently, no direct script access to the vertices and faces of the mesh is possible. But you can export the mesh to a VTK text file, "
			"which can be further processed by external tools such as ParaView. "
			"\n\n"
			"The visual appearance of the surface mesh within Ovito is controlled by its attached :py:class:`~ovito.vis.SurfaceMeshDisplay` instance, which is "
			"accessible through the :py:attr:`~DataObject.display` attribute of the :py:class:`DataObject` base class or through the :py:attr:`~ovito.modifiers.ConstructSurfaceModifier.mesh_display` attribute "
			"of the :py:class:`~ovito.modifiers.ConstructSurfaceModifier` that created the surface mesh."
			"\n\n"
			"Example:\n\n"
			".. literalinclude:: ../example_snippets/surface_mesh.py"
		)
		.add_property("isCompletelySolid", &SurfaceMesh::isCompletelySolid, &SurfaceMesh::setCompletelySolid)
		.def("export_vtk", lambda_address([](SurfaceMesh& mesh, const QString& filename, SimulationCellObject* simCellObj) {
			if(!simCellObj)
				throw Exception("A simulation cell is required to generate non-periodic mesh for export.");
			TriMesh output;
			if(!SurfaceMeshDisplay::buildSurfaceMesh(*mesh.storage(), simCellObj->data(), false, mesh.cuttingPlanes(), output))
				throw Exception("Failed to generate non-periodic mesh for export. Simulation cell might be too small.");
			QFile file(filename);
			CompressedTextWriter writer(file);
			output.saveToVTK(writer);
		}),
		"export_vtk(filename, cell)"
		"\n\n"
		"Writes the surface mesh to a VTK file, which is a simple text-based format and which can be opened with the software ParaView. "
		"The method takes the output filename and a :py:class:`~ovito.data.SimulationCell` object as input. The simulation cell information "
		"is needed by the method to generate a non-periodic version of the mesh, which is truncated at the periodic boundaries "
		"of the simulation cell (if it has any).")
		.def("export_cap_vtk", lambda_address([](SurfaceMesh& mesh, const QString& filename, SimulationCellObject* simCellObj) {
			if(!simCellObj)
				throw Exception("A simulation cell is required to generate cap mesh for export.");
			TriMesh output;
			SurfaceMeshDisplay::buildCapMesh(*mesh.storage(), simCellObj->data(), mesh.isCompletelySolid(), false, mesh.cuttingPlanes(), output);
			QFile file(filename);
			CompressedTextWriter writer(file);
			output.saveToVTK(writer);
		}),
		"export_cap_vtk(filename, cell)"
		"\n\n"
		"If the surface mesh has been generated from a :py:class:`~ovito.data.SimulationCell` with periodic boundary conditions, then this "
		"method computes the cap polygons from the intersection of the surface mesh with the periodic cell boundaries. "
		"The cap polygons are written to a VTK file, which is a simple text-based format and which can be opened with the software ParaView.")
	;

	{
		scope s = class_<CutoffNeighborFinder>("CutoffNeighborFinder", init<>())
			.def("prepare", lambda_address([](CutoffNeighborFinder& finder, FloatType cutoff, ParticlePropertyObject& positions, SimulationCellObject& cell) {
				finder.prepare(cutoff, positions.storage(), cell.data());
			}))
		;

		class_<CutoffNeighborFinder::Query>("Query", init<const CutoffNeighborFinder&, size_t>())
			.def("next", &CutoffNeighborFinder::Query::next)
			.add_property("atEnd", &CutoffNeighborFinder::Query::atEnd)
			.add_property("index", &CutoffNeighborFinder::Query::current)
			.add_property("distance_squared", &CutoffNeighborFinder::Query::distanceSquared)
			.add_property("distance", lambda_address([](const CutoffNeighborFinder::Query& query) -> FloatType { return sqrt(query.distanceSquared()); }))
			.add_property("delta", lambda_address([](const CutoffNeighborFinder::Query& query) { return make_tuple(query.delta().x(), query.delta().y(), query.delta().z()); }))
			.add_property("pbc_shift", lambda_address([](const CutoffNeighborFinder::Query& query) { return make_tuple(query.pbcShift().x(), query.pbcShift().y(), query.pbcShift().z()); }))
		;
	}

	{
		scope s = class_<NearestNeighborFinder>("NearestNeighborFinder", init<size_t>())
			.def("prepare", lambda_address([](NearestNeighborFinder& finder, ParticlePropertyObject& positions, SimulationCellObject& cell) {
				finder.prepare(positions.storage(), cell.data());
			}))
		;

		typedef NearestNeighborFinder::Query<30> NearestNeighborQuery;

		class_<NearestNeighborFinder::Neighbor>("Neighbor", no_init)
			.def_readonly("index", &NearestNeighborFinder::Neighbor::index)
			.def_readonly("distance_squared", &NearestNeighborFinder::Neighbor::distanceSq)
			.add_property("distance", lambda_address([](const NearestNeighborFinder::Neighbor& n) -> FloatType { return sqrt(n.distanceSq); }))
			.add_property("delta", lambda_address([](const NearestNeighborFinder::Neighbor& n) { return make_tuple(n.delta.x(), n.delta.y(), n.delta.z()); }))
		;

		class_<NearestNeighborQuery>("Query", init<const NearestNeighborFinder&>())
			.def("findNeighbors", static_cast<void (NearestNeighborQuery::*)(size_t)>(&NearestNeighborQuery::findNeighbors))
			.add_property("count", lambda_address([](const NearestNeighborQuery& q) -> int { return q.results().size(); }))
			.def("__getitem__", make_function(lambda_address([](const NearestNeighborQuery& q, int index) -> const NearestNeighborFinder::Neighbor& { return q.results()[index]; }),
					return_internal_reference<>()))
		;
	}

	{
		scope s = ovito_abstract_class<BondPropertyObject, DataObject>(
				":Base class: :py:class:`ovito.data.DataObject`\n\n"
				"This data object stores the values of a certain bond property. A bond property is a quantity associated with every bond in a system. "
				"Bond properties work similar to particle properties (see :py:class:`ParticleProperty` class)."
				"\n\n"
				"All bond properties associated with the bonds in a system are stored in the :py:attr:`DataCollection.bond_properties` dictionary of the :py:class:`DataCollection` container. "
				"Bond properties are either read from the external simulation file or can be newly generated by OVITO's modifiers, the "
				":py:class:`~ovito.modifiers.ComputeBondLengthsModifier` being one example. "
				"\n\n"
				"The topological definition of bonds, i.e. the connectivity of particles, is stored separately from the bond properties in the :py:class:`Bonds` data object. "
				"The :py:class:`Bonds` can be accessed through the :py:attr:`DataCollection.bonds` field. "
				"\n\n"
				"Note that OVITO internally works with half-bonds, i.e., every full bond is represented as two half-bonds, one pointing "
				"from particle A to particle B and the other from B to A. Each half-bond is associated with its own property value, "
				"and the :py:attr:`.size` of a bond property array is always twice as large as the number of full bonds "
				"(see :py:attr:`DataCollection.number_of_half_bonds` and :py:attr:`DataCollection.number_of_full_bonds`). "
				"Typically, however, the property values of a half-bond and its reverse bond are identical. "
				"\n\n"
				"Similar to particle properties, it is possible to associate user-defined properties with bonds. OVITO also knows a set of standard "
				"bond properties (see the :py:attr:`.type` attribute below), which control the visual appearance of bonds. For example, "
				"it is possible to assign the ``Color`` property to bonds, giving one control over the rendering color of each individual (half-)bond. "
				"The color values stored in this property array will be used by OVITO to render the bonds. If not present, OVITO will fall back to the "
				"default behavior, which is determined by the :py:class:`ovito.vis.BondsDisplay` associated with the :py:class:`Bonds` object. ",
				// Python class name:
				"BondProperty")
			.def("createUserProperty", &BondPropertyObject::createUserProperty)
			.def("createStandardProperty", &BondPropertyObject::createStandardProperty)
			.def("findInState", make_function((BondPropertyObject* (*)(const PipelineFlowState&, BondProperty::Type))&BondPropertyObject::findInState, return_value_policy<ovito_object_reference>()))
			.def("findInState", make_function((BondPropertyObject* (*)(const PipelineFlowState&, const QString&))&BondPropertyObject::findInState, return_value_policy<ovito_object_reference>()))
			.staticmethod("createUserProperty")
			.staticmethod("createStandardProperty")
			.staticmethod("findInState")
			.def("changed", &BondPropertyObject::changed,
					"Informs the bond property object that its stored data has changed. "
					"This function must be called after each direct modification of the per-bond data "
					"through the :py:attr:`.marray` attribute.\n\n"
					"Calling this method on an input bond property is necessary to invalidate data caches down the modification "
					"pipeline. Forgetting to call this method may result in an incomplete re-evaluation of the modification pipeline. "
					"See :py:attr:`.marray` for more information.")
			.def("nameWithComponent", &BondPropertyObject::nameWithComponent)
			.add_property("name", make_function(&BondPropertyObject::name, return_value_policy<copy_const_reference>()), &BondPropertyObject::setName,
					"The human-readable name of the bond property.")
			.add_property("__len__", &BondPropertyObject::size)
			.add_property("size", &BondPropertyObject::size, &BondPropertyObject::resize,
					"The number of stored property values, which is always equal to the number of half-bonds.")
			.add_property("type", &BondPropertyObject::type, &BondPropertyObject::setType,
					".. _bond-types-list:"
					"\n\n"
					"The type of the bond property (user-defined or one of the standard types).\n"
					"One of the following constants:"
					"\n\n"
					"======================================================= =================================================== ==========\n"
					"Type constant                                           Property name                                       Data type \n"
					"======================================================= =================================================== ==========\n"
					"``BondProperty.Type.User``                              (a user-defined property with a non-standard name)  int/float \n"
					"``BondProperty.Type.BondType``                          :guilabel:`Bond Type`                               int       \n"
					"``BondProperty.Type.Selection``                         :guilabel:`Selection`                               int       \n"
					"``BondProperty.Type.Color``                             :guilabel:`Color`                                   float     \n"
					"``BondProperty.Type.Length``                            :guilabel:`Length`                                  float     \n"
					"======================================================= =================================================== ==========\n"
					)
			.add_property("dataType", &BondPropertyObject::dataType)
			.add_property("dataTypeSize", &BondPropertyObject::dataTypeSize)
			.add_property("stride", &BondPropertyObject::stride)
			.add_property("components", &BondPropertyObject::componentCount,
					"The number of vector components (if this is a vector bond property); otherwise 1 (= scalar property).")
			.add_property("__array_interface__", &PropertyObject__array_interface__<BondPropertyObject,true>)
			.add_property("__mutable_array_interface__", &PropertyObject__array_interface__<BondPropertyObject,false>)
		;

		enum_<BondProperty::Type>("Type")
			.value("User", BondProperty::UserProperty)
			.value("BondType", BondProperty::BondTypeProperty)
			.value("Selection", BondProperty::SelectionProperty)
			.value("Color", BondProperty::ColorProperty)
			.value("Length", BondProperty::LengthProperty)
		;
	}

	ovito_abstract_class<BondTypeProperty, BondPropertyObject>(
			":Base class: :py:class:`ovito.data.BondProperty`\n\n"
			"A special :py:class:`BondProperty` that stores a list of :py:class:`BondType` instances in addition "
			"to the per-bond values. "
			"\n\n"
			"The bond property ``Bond Type`` is represented by an instance of this class. In addition to the regular per-bond "
			"data (consisting of an integer per half-bond, indicating its type ID), this class holds the list of defined bond types. These are "
			":py:class:`BondType` instances, which store the ID, name, and color of each bond type.")
		.def("addBondType", &BondTypeProperty::addBondType)
		.def("insertBondType", &BondTypeProperty::insertBondType)
		.def("bondType", make_function((BondType* (BondTypeProperty::*)(int) const)&BondTypeProperty::bondType, return_value_policy<ovito_object_reference>()))
		.def("bondType", make_function((BondType* (BondTypeProperty::*)(const QString&) const)&BondTypeProperty::bondType, return_value_policy<ovito_object_reference>()))
		.def("removeBondType", &BondTypeProperty::removeBondType)
		.def("clearBondTypes", &BondTypeProperty::clearBondTypes)
		.add_property("bondTypes", make_function(&BondTypeProperty::bondTypes, return_internal_reference<>()))
	;

	ovito_class<BondType, RefTarget>(
			"Stores the properties of a bond type."
			"\n\n"
			"The list of bond types is stored in the :py:class:`~ovito.data.BondTypeProperty` class.")
		.add_property("id", &BondType::id, &BondType::setId,
				"The identifier of the bond type.")
		.add_property("color", &BondType::color, &BondType::setColor,
				"The display color to use for bonds of this type.")
		.add_property("name", make_function(&BondType::name, return_value_policy<copy_const_reference>()), &BondType::setName,
				"The display name of this bond type.")
	;

	class_<QVector<BondType*>, boost::noncopyable>("QVector<BondType*>", no_init)
		.def(QVector_OO_readonly_indexing_suite<BondType>())
	;
	python_to_container_conversion<QVector<BondType*>>();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(Particles);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
