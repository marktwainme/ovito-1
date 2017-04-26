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


#include <plugins/particles/Particles.h>
#include <plugins/particles/import/InputColumnMapping.h>
#include <plugins/particles/import/ParticleImporter.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

/**
 * \brief File parser for the text-based XYZ file format.
 */
class OVITO_PARTICLES_EXPORT XYZImporter : public ParticleImporter
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE XYZImporter(DataSet* dataset) : ParticleImporter(dataset), _autoRescaleCoordinates(true) {
		INIT_PROPERTY_FIELD(autoRescaleCoordinates);	
	}

	/// \brief Returns the file filter that specifies the files that can be imported by this service.
	/// \return A wild-card pattern that specifies the file types that can be handled by this import class.
	virtual QString fileFilter() override { return QStringLiteral("*"); }

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	/// \return A string that describes the file format.
	virtual QString fileFilterDescription() override { return tr("XYZ Files"); }

	/// \brief Checks if the given file has format that can be read by this importer.
	virtual bool checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) override;

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("XYZ File"); }

	/// \brief Returns the user-defined mapping between data columns in the input file and
	///        the internal particle properties.
	const InputColumnMapping& columnMapping() const { return _columnMapping; }

	/// \brief Sets the user-defined mapping between data columns in the input file and
	///        the internal particle properties.
	void setColumnMapping(const InputColumnMapping& mapping);

	/// Guesses the mapping of input file columns to internal particle properties.
	static bool mapVariableToProperty(InputColumnMapping &columnMapping, int column, QString name, int dataType, int vec);

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FrameLoader> createFrameLoader(const Frame& frame, bool isNewlySelectedFile) override {
		return std::make_shared<XYZImportTask>(dataset()->container(), frame, isNewlySelectedFile, columnMapping(), autoRescaleCoordinates());
	}

	/// Inspects the header of the given file and returns the number of file columns.
	InputColumnMapping inspectFileHeader(const Frame& frame);

public:

	Q_PROPERTY(Ovito::Particles::InputColumnMapping columnMapping READ columnMapping WRITE setColumnMapping);

private:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class XYZImportTask : public ParticleFrameLoader
	{
	public:

		/// Normal constructor.
		XYZImportTask(DataSetContainer* container, const FileSourceImporter::Frame& frame, bool isNewFile, const InputColumnMapping& columnMapping, bool autoRescaleCoordinates)
		  : ParticleFrameLoader(container, frame, isNewFile), _parseFileHeaderOnly(false), _columnMapping(columnMapping), _autoRescaleCoordinates(autoRescaleCoordinates) {}

		/// Constructor used when reading only the file header information.
		XYZImportTask(DataSetContainer* container, const FileSourceImporter::Frame& frame)
		  : ParticleFrameLoader(container, frame, true), _parseFileHeaderOnly(true) {}

		/// Returns the file column mapping used to load the file.
		const InputColumnMapping& columnMapping() const { return _columnMapping; }

	protected:

		/// Parses the given input file and stores the data in this container object.
		virtual void parseFile(CompressedTextReader& stream) override;

	private:

		bool _parseFileHeaderOnly;
		bool _autoRescaleCoordinates;
		InputColumnMapping _columnMapping;
	};

protected:

	/// \brief Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// \brief Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// \brief Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) override;

	/// \brief Scans the given input file to find all contained simulation frames.
	virtual void scanFileForTimesteps(PromiseBase& promise, QVector<FileSourceImporter::Frame>& frames, const QUrl& sourceUrl, CompressedTextReader& stream) override;

private:

	/// Stores the user-defined mapping between data columns in the input file and
	/// the internal particle properties.
	InputColumnMapping _columnMapping;

	/// Controls the automatic detection of reduced atom coordinates in the input file.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, autoRescaleCoordinates, setAutoRescaleCoordinates);

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


