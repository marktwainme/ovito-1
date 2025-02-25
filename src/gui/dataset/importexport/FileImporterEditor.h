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

#ifndef __OVITO_FILE_IMPORTER_EDITOR_H
#define __OVITO_FILE_IMPORTER_EDITOR_H

#include <gui/GUI.h>
#include <gui/properties/PropertiesEditor.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

/**
 * \brief Abstract base class for properties editors for FileImporter derived classes.
 */
class OVITO_GUI_EXPORT FileImporterEditor : public PropertiesEditor
{
public:

	/// Constructor.
	FileImporterEditor() {}

	/// This is called by the system when the user has selected a new file to import.
	virtual bool inspectNewFile(FileImporter* importer, const QUrl& sourceFile, QWidget* parent) { return true; }

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_FILE_IMPORTER_EDITOR_H
