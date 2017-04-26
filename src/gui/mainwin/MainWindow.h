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


#include <gui/GUI.h>
#include <gui/dataset/GuiDataSetContainer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/**
 * \brief The main window of the application.
 *
 * Note that is is possible to open multiple main windows per
 * application instance to edit multiple datasets simultaneously.
 */
class OVITO_GUI_EXPORT MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:

	/// The pages of the command panel.
	enum CommandPanelPage {
		MODIFY_PAGE		= 0,
		RENDER_PAGE		= 1,
		OVERLAY_PAGE	= 2,
		UTILITIES_PAGE	= 3
	};

public:

	/// Constructor of the main window class.
	MainWindow();

	/// Destructor.
	virtual ~MainWindow();

	/// Returns the main toolbar of the window.
	QToolBar* mainToolbar() const { return _mainToolbar; }
	
	/// Returns the status bar of the main window.
	QStatusBar* statusBar() const { return _statusBar; }

	/// Returns the frame buffer window showing the rendered image.
	FrameBufferWindow* frameBufferWindow() const { return _frameBufferWindow; }

	/// Returns the recommended size for this window.
	virtual QSize sizeHint() const override { return QSize(1024,768); }
	
	/// \brief Loads the layout of the docked widgets from the settings store.
	void restoreLayout();

	/// \brief Saves the layout of the docked widgets to the settings store.
	void saveLayout();

	/// \brief Immediately repaints all viewports that are flagged for an update.
	void processViewportUpdates();

	/// Returns the container that keeps a reference to the current dataset.
	GuiDataSetContainer& datasetContainer() { return _datasetContainer; }

	/// Returns the window's action manager.
	ActionManager* actionManager() const { return _actionManager; }

	/// Returns the window's viewport input manager.
	ViewportInputManager* viewportInputManager() const { return _viewportInputManager; }

	/// Returns the widget that numerically displays the transformation.
	CoordinateDisplayWidget* coordinateDisplay() const { return _coordinateDisplay; }

	/// Returns the container widget for viewports.
	QWidget* viewportsPanel() const { return _viewportsPanel; }

	/// Returns the layout manager for the status bar area of the main window.
	QHBoxLayout* statusBarLayout() const { return _statusBarLayout; }

	/// \brief Shows the online manual and opens the given help page.
	static void openHelpTopic(const QString& page);

	/// Returns the master OpenGL context managed by this window, which is used to render the viewports.
	/// If sharing of OpenGL contexts between viewports is disabled, then this function returns the GL context
	/// of the first viewport in this window.
	QOpenGLContext* getOpenGLContext();

	/// Returns the page of the command panel that is currently visible.
	CommandPanelPage currentCommandPanelPage() const;

	/// Sets the page of the command panel that is currently visible.
	void setCurrentCommandPanelPage(CommandPanelPage page);

	/// Sets the file path associated with this window and updates the window's title.
	void setWindowFilePath(const QString& filePath);

	/// Returns the main window in which the given dataset is opened.
	static MainWindow* fromDataset(DataSet* dataset);

protected:

	/// Is called when the user closes the window.
	virtual void closeEvent(QCloseEvent* event) override;

	/// Is called when the window receives an event.
	virtual bool event(QEvent *event) override;

private:

	/// Creates the main menu.
	void createMainMenu();

	/// Creates the main toolbar.
	void createMainToolbar();

	/// Creates a dock panel.
	void createDockPanel(const QString& caption, const QString& objectName, Qt::DockWidgetArea dockArea, Qt::DockWidgetAreas allowedAreas, QWidget* contents);

private:

	/// The upper main toolbar.
	QToolBar* _mainToolbar;
	
	/// The internal status bar widget.
	QStatusBar* _statusBar;

	/// The frame buffer window showing the rendered image.
	FrameBufferWindow* _frameBufferWindow;

	/// The command panel.
	CommandPanel* _commandPanel;

	/// Container that keeps a reference to the current dataset.
	GuiDataSetContainer _datasetContainer;

	/// The associated GUI action manager.
	ActionManager* _actionManager;

	/// The associated viewport input manager.
	ViewportInputManager* _viewportInputManager;

	/// The container widget for viewports.
	QWidget* _viewportsPanel;

	/// The widget that numerically displays the object transformation.
	CoordinateDisplayWidget* _coordinateDisplay;

	/// The layout manager for the status bar area of the main window.
	QHBoxLayout* _statusBarLayout;

	/// The OpenGL context used for rendering the viewports.
	QPointer<QOpenGLContext> _glcontext;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


