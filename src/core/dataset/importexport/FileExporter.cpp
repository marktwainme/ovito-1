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

#include <core/Core.h>
#include <core/plugins/PluginManager.h>
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/ProgressDisplay.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/scene/ObjectNode.h>
#include <core/animation/AnimationSettings.h>
#include "FileExporter.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Core, FileExporter, RefTarget);
DEFINE_PROPERTY_FIELD(FileExporter, _outputFilename, "OutputFile");
DEFINE_PROPERTY_FIELD(FileExporter, _exportAnimation, "ExportAnimation");
DEFINE_PROPERTY_FIELD(FileExporter, _useWildcardFilename, "UseWildcardFilename");
DEFINE_PROPERTY_FIELD(FileExporter, _wildcardFilename, "WildcardFilename");
DEFINE_PROPERTY_FIELD(FileExporter, _startFrame, "StartFrame");
DEFINE_PROPERTY_FIELD(FileExporter, _endFrame, "EndFrame");
DEFINE_PROPERTY_FIELD(FileExporter, _everyNthFrame, "EveryNthFrame");
SET_PROPERTY_FIELD_LABEL(FileExporter, _outputFilename, "Output filename");
SET_PROPERTY_FIELD_LABEL(FileExporter, _exportAnimation, "Export animation");
SET_PROPERTY_FIELD_LABEL(FileExporter, _useWildcardFilename, "Use wildcard filename");
SET_PROPERTY_FIELD_LABEL(FileExporter, _wildcardFilename, "Wildcard filename");
SET_PROPERTY_FIELD_LABEL(FileExporter, _startFrame, "Start frame");
SET_PROPERTY_FIELD_LABEL(FileExporter, _endFrame, "End frame");
SET_PROPERTY_FIELD_LABEL(FileExporter, _everyNthFrame, "Every Nth frame");

/******************************************************************************
* Constructs a new instance of the class.
******************************************************************************/
FileExporter::FileExporter(DataSet* dataset) : RefTarget(dataset),
	_exportAnimation(false),
	_useWildcardFilename(false), _startFrame(0), _endFrame(-1),
	_everyNthFrame(1)
{
	INIT_PROPERTY_FIELD(FileExporter::_outputFilename);
	INIT_PROPERTY_FIELD(FileExporter::_exportAnimation);
	INIT_PROPERTY_FIELD(FileExporter::_useWildcardFilename);
	INIT_PROPERTY_FIELD(FileExporter::_wildcardFilename);
	INIT_PROPERTY_FIELD(FileExporter::_startFrame);
	INIT_PROPERTY_FIELD(FileExporter::_endFrame);
	INIT_PROPERTY_FIELD(FileExporter::_everyNthFrame);

	// Use the entire animation interval as default export interval.
	setStartFrame(0);
	int lastFrame = dataset->animationSettings()->timeToFrame(dataset->animationSettings()->animationInterval().end());
	setEndFrame(lastFrame);
}

/******************************************************************************
* Sets the scene objects to be exported.
******************************************************************************/
void FileExporter::setOutputData(const QVector<SceneNode*>& nodes)
{
	_nodesToExport.clear();
	for(SceneNode* node : nodes)
		_nodesToExport.push_back(node);
}

/******************************************************************************
* Sets the name of the output file that should be written by this exporter.
******************************************************************************/
void FileExporter::setOutputFilename(const QString& filename)
{
	_outputFilename = filename;

	// Generate a default wildcard pattern from the filename.
	if(wildcardFilename().isEmpty()) {
		QString fn = QFileInfo(filename).fileName();
		if(!fn.contains('*')) {
			int dotIndex = fn.lastIndexOf('.');
			if(dotIndex > 0)
				setWildcardFilename(fn.left(dotIndex) + QStringLiteral(".*") + fn.mid(dotIndex));
			else
				setWildcardFilename(fn + QStringLiteral(".*"));
		}
		else
			setWildcardFilename(fn);
	}
}

/******************************************************************************
 * Exports the data of the scene nodes to one or more output files.
 *****************************************************************************/
bool FileExporter::exportNodes(AbstractProgressDisplay* progressDisplay)
{
	if(outputFilename().isEmpty())
		throwException(tr("The output filename not been set for the file exporter."));

	if(startFrame() > endFrame())
		throwException(tr("The animation interval to be exported is empty or has not been set."));

	if(outputData().empty())
		throwException(tr("There is no data to be exported."));

	// Compute the number of frames that need to be exported.
	TimePoint exportTime;
	int firstFrameNumber, numberOfFrames;
	if(exportAnimation()) {
		firstFrameNumber = startFrame();
		exportTime = dataset()->animationSettings()->frameToTime(firstFrameNumber);
		numberOfFrames = (endFrame() - startFrame() + everyNthFrame()) / everyNthFrame();
		if(numberOfFrames < 1 || everyNthFrame() < 1)
			throwException(tr("Invalid export animation range: Frame %1 to %2").arg(startFrame()).arg(endFrame()));
	}
	else {
		exportTime = dataset()->animationSettings()->time();
		firstFrameNumber = dataset()->animationSettings()->timeToFrame(exportTime);
		numberOfFrames = 1;
	}

	// Validate export settings.
	if(exportAnimation() && useWildcardFilename()) {
		if(wildcardFilename().isEmpty())
			throwException(tr("Cannot write animation frame to separate files. Wildcard pattern has not been specified."));
		if(wildcardFilename().contains(QChar('*')) == false)
			throwException(tr("Cannot write animation frames to separate files. The filename must contain the '*' wildcard character, which gets replaced by the frame number."));
	}

	if(progressDisplay) progressDisplay->setMaximum(numberOfFrames * 100);
	QDir dir = QFileInfo(outputFilename()).dir();
	QString filename = outputFilename();

	// Open output file for writing.
	if(!exportAnimation() || !useWildcardFilename()) {
		if(!openOutputFile(filename, numberOfFrames))
			return false;
	}

	try {

		// Export animation frames.
		for(int frameIndex = 0; frameIndex < numberOfFrames; frameIndex++) {
			if(progressDisplay)
				progressDisplay->setValue(frameIndex * 100);

			int frameNumber = firstFrameNumber + frameIndex * everyNthFrame();

			if(exportAnimation() && useWildcardFilename()) {
				// Generate an output filename based on the wildcard pattern.
				filename = dir.absoluteFilePath(wildcardFilename());
				filename.replace(QChar('*'), QString::number(frameNumber));

				if(!openOutputFile(filename, 1))
					return false;
			}

			if(!exportFrame(frameNumber, exportTime, filename, progressDisplay) && progressDisplay)
				progressDisplay->cancel();

			if(exportAnimation() && useWildcardFilename())
				closeOutputFile(!progressDisplay || !progressDisplay->wasCanceled());

			if(progressDisplay && progressDisplay->wasCanceled())
				break;

			// Go to next animation frame.
			exportTime += dataset()->animationSettings()->ticksPerFrame() * everyNthFrame();
		}
	}
	catch(...) {
		closeOutputFile(false);
		throw;
	}

	// Close output file.
	if(!exportAnimation() || !useWildcardFilename()) {
		closeOutputFile(!progressDisplay || !progressDisplay->wasCanceled());
	}

	return !progressDisplay || !progressDisplay->wasCanceled();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool FileExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, AbstractProgressDisplay* progressDisplay)
{
	// Jump to animation time.
	dataset()->animationSettings()->setTime(time);

	// Wait until the scene is ready.
	if(!dataset()->waitUntilSceneIsReady(tr("Preparing frame %1 for export...").arg(frameNumber), progressDisplay))
		return false;

	// Also make sure nodes to be exported are ready, in case they are not part of the scene.
	for(SceneNode* sceneNode : outputData()) {
		try {
			if(ObjectNode* objNode = dynamic_object_cast<ObjectNode>(sceneNode)) {
				if(!objNode->waitUntilReady(time, tr("Preparing frame %1 for export...").arg(frameNumber), progressDisplay))
					return false;
			}
		}
		catch(Exception& ex) {
			// Provide a local context for errors that occurred during export.
			if(ex.context() == nullptr) ex.setContext(dataset());
			throw;
		}
	}
	
	return true;
}

/******************************************************************************
* Return the list of available export services.
******************************************************************************/
QVector<OvitoObjectType*> FileExporter::availableExporters()
{
	return PluginManager::instance().listClasses(FileExporter::OOType);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
