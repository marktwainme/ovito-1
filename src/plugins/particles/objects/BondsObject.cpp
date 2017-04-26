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
#include "BondsObject.h"
#include "BondsDisplay.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(BondsObject, DataObject);

/******************************************************************************
* Default constructor.
******************************************************************************/
BondsObject::BondsObject(DataSet* dataset, BondsStorage* storage) : DataObjectWithSharedStorage(dataset, storage ? storage : new BondsStorage())
{
	// Attach a display object.
	addDisplayObject(new BondsDisplay(dataset));
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void BondsObject::saveToStream(ObjectSaveStream& stream)
{
	DataObjectWithSharedStorage::saveToStream(stream);

	stream.beginChunk(0x01);
	storage()->saveToStream(stream, !saveWithScene());
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void BondsObject::loadFromStream(ObjectLoadStream& stream)
{
	DataObjectWithSharedStorage::loadFromStream(stream);

	stream.expectChunk(0x01);
	modifiableStorage()->loadFromStream(stream);
	stream.closeChunk();
}

/******************************************************************************
* Remaps the bonds after some of the particles have been deleted.
* Dangling bonds are removed too.
******************************************************************************/
size_t BondsObject::particlesDeleted(const boost::dynamic_bitset<>& deletedParticlesMask, boost::dynamic_bitset<>& deletedBondsMask)
{
	// Build map that maps old particle indices to new indices.
	std::vector<size_t> indexMap(deletedParticlesMask.size());
	auto index = indexMap.begin();
	size_t oldParticleCount = deletedParticlesMask.size();
	size_t newParticleCount = 0;
	for(size_t i = 0; i < deletedParticlesMask.size(); i++)
		*index++ = deletedParticlesMask.test(i) ? std::numeric_limits<size_t>::max() : newParticleCount++;

	OVITO_ASSERT(deletedBondsMask.empty() || deletedBondsMask.size() == modifiableStorage()->size());

	auto result = modifiableStorage()->begin();
	auto bond = modifiableStorage()->begin();
	auto last = modifiableStorage()->end();
	size_t bondIndex = 0;
	for(; bond != last; ++bond, ++bondIndex) {
		// Remove invalid bonds.
		if(bond->index1 >= oldParticleCount || bond->index2 >= oldParticleCount) {
			if(!deletedBondsMask.empty())
				deletedBondsMask.set(bondIndex);
			continue;
		}

		// Remove dangling bonds whose particles have gone.
		if(deletedParticlesMask.test(bond->index1) || deletedParticlesMask.test(bond->index2)) {
			if(!deletedBondsMask.empty())
				deletedBondsMask.set(bondIndex);
			continue;
		}

		// Keep bond and remap particle indices.
		result->pbcShift = bond->pbcShift;
		result->index1 = indexMap[bond->index1];
		result->index2 = indexMap[bond->index2];
		++result;
		if(!deletedBondsMask.empty())
			deletedBondsMask.reset(bondIndex);
	}
	modifiableStorage()->erase(result, last);
	changed();
	return modifiableStorage()->size();
}

}	// End of namespace
}	// End of namespace
