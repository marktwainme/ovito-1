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

#ifndef __OVITO_CREATE_BONDS_MODIFIER_H
#define __OVITO_CREATE_BONDS_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/modifier/AsynchronousParticleModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief A modifier that creates bonds between pairs of particles based on their distance.
 */
class OVITO_PARTICLES_EXPORT CreateBondsModifier : public AsynchronousParticleModifier
{
public:

	enum CutoffMode {
		UniformCutoff,		///< A single cutoff radius for all particles.
		PairCutoff,			///< Individual cutoff radius for each pair of particle types.
	};
	Q_ENUMS(CutoffMode);

	/// The container type used to store the pair-wise cutoffs.
	typedef QMap<QPair<QString,QString>, FloatType> PairCutoffsList;

private:

	/// Engine that determines the bonds between particles.
	class BondsEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		BondsEngine(const TimeInterval& validityInterval, ParticleProperty* positions, ParticleProperty* particleTypes, const SimulationCell& simCell, CutoffMode cutoffMode,
				FloatType maxCutoff, FloatType minCutoff, std::vector<std::vector<FloatType>>&& pairCutoffsSquared, ParticleProperty* moleculeIDs) :
					ComputeEngine(validityInterval),
					_positions(positions), _particleTypes(particleTypes), _simCell(simCell), _cutoffMode(cutoffMode),
					_maxCutoff(maxCutoff), _minCutoff(minCutoff), _pairCutoffsSquared(std::move(pairCutoffsSquared)), _bonds(new BondsStorage()),
					_moleculeIDs(moleculeIDs) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the generated bonds.
		BondsStorage* bonds() { return _bonds.data(); }

		/// Returns the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

	private:

		CutoffMode _cutoffMode;
		FloatType _maxCutoff;
		FloatType _minCutoff;
		std::vector<std::vector<FloatType>> _pairCutoffsSquared;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _particleTypes;
		QExplicitlySharedDataPointer<ParticleProperty> _moleculeIDs;
		QExplicitlySharedDataPointer<BondsStorage> _bonds;
		SimulationCell _simCell;
	};

public:

	/// Constructor.
	Q_INVOKABLE CreateBondsModifier(DataSet* dataset);

	/// Returns the mode of choosing the cutoff radius.
	CutoffMode cutoffMode() const { return _cutoffMode; }

	/// Sets the mode of choosing the cutoff radius.
	void setCutoffMode(CutoffMode mode) { _cutoffMode = mode; }

	/// \brief Returns the uniform cutoff radius used to determine which particles are bonded.
	/// \return The uniform cutoff radius.
	FloatType uniformCutoff() const { return _uniformCutoff; }

	/// \brief Sets the cutoff radius that is used for generating bonds.
	/// \param newCutoff The new cutoff radius.
	void setUniformCutoff(FloatType newCutoff) { _uniformCutoff = newCutoff; }

	/// \brief Returns the minimum length of bonds to create.
	FloatType minimumCutoff() const { return _minCutoff; }

	/// \brief Sets the minimum length of bonds to create.
	void setMinimumCutoff(FloatType cutoff) { _minCutoff = cutoff; }

	/// Returns the cutoff radii for pairs of particle types.
	const PairCutoffsList& pairCutoffs() const { return _pairCutoffs; }

	/// Sets the cutoff radii for pairs of particle types.
	void setPairCutoffs(const PairCutoffsList& pairCutoffs);

	/// Sets the cutoff radius for a pair of particle types.
	void setPairCutoff(const QString& typeA, const QString& typeB, FloatType cutoff);

	/// Returns the pair-wise cutoff radius for a pair of particle types.
	FloatType getPairCutoff(const QString& typeA, const QString& typeB) const;

	/// \brief Returns the display object that is responsible for rendering the bonds.
	BondsDisplay* bondsDisplay() const { return _bondsDisplay; }

	/// Returns whether bonds will only be created between atoms from the same molecule.
	bool onlyIntraMoleculeBonds() const { return _onlyIntraMoleculeBonds; }

	/// Sets whether bonds will only be created between atoms from the same molecule.
	void setOnlyIntraMoleculeBonds(bool enable) { _onlyIntraMoleculeBonds = enable; }

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) override;

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Resets the modifier's result cache.
	virtual void invalidateCachedResults() override;

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// The mode of choosing the cutoff radius.
	PropertyField<CutoffMode, int> _cutoffMode;

	/// The cutoff radius for bond generation.
	PropertyField<FloatType> _uniformCutoff;

	/// The minimum bond length.
	PropertyField<FloatType> _minCutoff;

	/// The cutoff radii for pairs of particle types.
	PairCutoffsList _pairCutoffs;

	/// If true, bonds will only be created between atoms from the same molecule.
	PropertyField<bool> _onlyIntraMoleculeBonds;

	/// The display object for rendering the bonds.
	ReferenceField<BondsDisplay> _bondsDisplay;

	/// This stores the cached results of the modifier, i.e. the list of created bonds.
	QExplicitlySharedDataPointer<BondsStorage> _bonds;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Create bonds");
	Q_CLASSINFO("ModifierCategory", "Modification");

	DECLARE_PROPERTY_FIELD(_cutoffMode);
	DECLARE_PROPERTY_FIELD(_uniformCutoff);
	DECLARE_PROPERTY_FIELD(_onlyIntraMoleculeBonds);
	DECLARE_PROPERTY_FIELD(_minCutoff);
	DECLARE_REFERENCE_FIELD(_bondsDisplay);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::CreateBondsModifier::CutoffMode);
Q_DECLARE_TYPEINFO(Ovito::Particles::CreateBondsModifier::CutoffMode, Q_PRIMITIVE_TYPE);

#endif // __OVITO_CREATE_BONDS_MODIFIER_H
