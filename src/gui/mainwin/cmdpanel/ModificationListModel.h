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

#ifndef __OVITO_MODIFICATION_LIST_MODEL_H
#define __OVITO_MODIFICATION_LIST_MODEL_H

#include <gui/GUI.h>
#include <core/reference/RefTarget.h>
#include <core/reference/RefTargetListener.h>
#include <core/scene/pipeline/ModifierApplication.h>
#include <core/scene/SceneNode.h>
#include "ModificationListItem.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * This Qt model class is used to populate the QListView widget.
 */
class ModificationListModel : public QAbstractListModel
{
	Q_OBJECT

public:

	/// Constructor.
	ModificationListModel(DataSetContainer& datasetContainer, QObject* parent);

	/// Returns the number of list items.
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override { return _items.size(); }

	/// Returns the data associated with a list entry.
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	/// Changes the data associated with a list entry.
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	/// Returns the flags for an item.
	virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

	/// Discards all list items.
	void clear() {
		if(_items.empty()) return;
		beginRemoveRows(QModelIndex(), 0, _items.size() - 1);
		_items.clear();
		_selectedNodes.clear();
		endRemoveRows();
		_needListUpdate = false;
	}

	/// Returns the associated selection model.
	QItemSelectionModel* selectionModel() const { return _selectionModel; }

	/// Returns the currently selected item in the modification list.
	ModificationListItem* selectedItem() const;

	/// Returns an item from the list model.
	ModificationListItem* item(int index) const {
		OVITO_ASSERT(index >= 0 && index < _items.size());
		return _items[index];
	}

	/// Populates the model with the given list items.
	void setItems(const QList<OORef<ModificationListItem>>& newItems, const QList<OORef<ModificationListItem>>& newHiddenItems);

	/// Returns the type of drag and drop operations supported by the model.
	Qt::DropActions supportedDropActions() const override {
	    return Qt::MoveAction;
	}

	/// Returns the list of allowed MIME types.
	QStringList mimeTypes() const override;

	/// Returns an object that contains serialized items of data corresponding to the list of indexes specified.
	QMimeData* mimeData(const QModelIndexList& indexes) const override;

	/// Returns true if the model can accept a drop of the data.
	bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;

	/// Handles the data supplied by a drag and drop operation that ended with the given action.
	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

	/// Returns true if the list model is currently in a valid state.
	bool isUpToDate() const { return !_needListUpdate; }

	/// The the current modification stack contains a hidden pipeline object at the top, this function returns it.
	PipelineObject* hiddenPipelineObject();

	/// The list of currently selected ObjectNode instances.
	const QVector<ObjectNode*>& selectedNodes() const { return _selectedNodes.targets(); }

	/// Returns the container of the dataset being edited.
	DataSetContainer& datasetContainer() { return _datasetContainer; }

    /// Inserts the given modifiers into the modification pipeline of the selected scene nodes.
	void applyModifiers(const QVector<OORef<Modifier>>& modifiers);

	/// Sets the item in the modification list that should be selected on the next list update.
	void setNextToSelectObject(RefTarget* obj) { _nextToSelectObject = obj; }

Q_SIGNALS:

	/// This signal is emitted if a new list item has been selected, or if the currently
	/// selected item has changed.
	void selectedItemChanged();

public Q_SLOTS:

	/// Rebuilds the complete list immediately.
	void refreshList();

	/// Updates the appearance of a single list item.
	void refreshItem(ModificationListItem* item);

	/// Rebuilds the list of modification items as soon as possible.
	void requestUpdate() {
		if(_needListUpdate) return;	// Update is already pending.
		_needListUpdate = true;
		// Invoke actual refresh function at some later time.
		QMetaObject::invokeMethod(this, "refreshList", Qt::QueuedConnection);
	}

private Q_SLOTS:

	/// Is called by the system when the animated status icon changed.
	void iconAnimationFrameChanged();

	/// Handles notification events generated by the selected object nodes.
	void onNodeEvent(RefTarget* source, ReferenceEvent* event);

private:

	/// List of visible items in the model.
	QList<OORef<ModificationListItem>> _items;

	/// List of hidden items in the model.
	QList<OORef<ModificationListItem>> _hiddenItems;

	/// Holds references to the currently selected ObjectNode instances.
	VectorRefTargetListener<ObjectNode> _selectedNodes;

	/// The item in the modification list that should be selected on the next list update.
	RefTarget* _nextToSelectObject;

	/// The selection model of the list view widget.
	QItemSelectionModel* _selectionModel;

	/// Indicates that the list of items needs to be updated.
	bool _needListUpdate;

	/// Status icons
	QPixmap _statusInfoIcon;
	QPixmap _statusWarningIcon;
	QPixmap _statusErrorIcon;
	QPixmap _statusNoneIcon;
	QMovie _statusPendingIcon;

	/// The font used for section headers.
	QFont _sectionHeaderFont;

	/// The container of the dataset being edited.
	DataSetContainer& _datasetContainer;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_MODIFICATION_LIST_MODEL_H
