///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#ifndef __OVITO_FHI_AIMS_IMPORTER_H
#define __OVITO_FHI_AIMS_IMPORTER_H

#include <plugins/particles/Particles.h>
#include "../ParticleImporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

/**
 * \brief File parser for 'geometry.in' files of the FHI-aims code.
 */
class OVITO_PARTICLES_EXPORT FHIAimsImporter : public ParticleImporter
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE FHIAimsImporter(DataSet* dataset) : ParticleImporter(dataset) {}

	/// \brief Returns the file filter that specifies the files that can be imported by this service.
	/// \return A wild-card pattern that specifies the file types that can be handled by this import class.
	virtual QString fileFilter() override { return QStringLiteral("*"); }

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	/// \return A string that describes the file format.
	virtual QString fileFilterDescription() override { return tr("FHI-aims Geometry Files"); }

	/// \brief Checks if the given file has format that can be read by this importer.
	virtual bool checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) override;

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("FHI-aims"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FrameLoader> createFrameLoader(const Frame& frame, bool isNewlySelectedFile) override {
		return std::make_shared<FHIAimsImportTask>(dataset()->container(), frame, isNewlySelectedFile);
	}

private:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class FHIAimsImportTask : public ParticleFrameLoader
	{
	public:

		/// Constructor.
		FHIAimsImportTask(DataSetContainer* container, const FileSourceImporter::Frame& frame, bool isNewFile)
			: ParticleFrameLoader(container, frame, isNewFile) {}

	protected:

		/// Parses the given input file and stores the data in this container object.
		virtual void parseFile(CompressedTextReader& stream) override;
	};


	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_FHI_AIMS_IMPORTER_H
