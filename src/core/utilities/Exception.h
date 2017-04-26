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

/**
 * \file
 * \brief Contains the definition of the Ovito::Exception class.
 */

#pragma once


#include <core/Core.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util)

#ifdef QT_NO_EXCEPTIONS
	#error "OVITO requires Qt exception support. It seems that Qt has been built without exceptions (the macro QT_NO_EXCEPTIONS is defined). Please turn on exception support and rebuild the Qt library."
#endif

/**
 * \brief The standard exception type used by OVITO.
 *
 * The Exception class carries a message string that describes the error that has occurred.
 * The reportError() method displays the error message to the user. A typical usage pattern is:
 *
 * \code
 *     try {
 *         ...
 *         if(error) throw Exception("The operation failed.");
 *         ...
 *     }
 *     catch(const Exception& ex) {
 *         ex.reportError();
 *     }
 * \endcode
 *
 * Internally, the Exception class stores a list of message strings. The first string always gives the most general description of the error,
 * while any additional strings may describe the error in more detail or explain the low-level origin of the error.
 * The appendDetailMessage() method can be used to add more detailed information about the error:
 *
 * \code
 *     if(errorCode != 0) {
 *         Exception ex("The operation failed.");
 *         ex.appendDetailMessage(QString("Error code is %1").arg(errorCode));
 *         throw ex;
 *     }
 * \endcode
 *
 * If an Exception is thrown by a low-level function, it might be necessary to prepend a more general message before displaying
 * the error to the user. This can be done using the prependGeneralMessage() method:
 *
 * \code
 *     try {
 *         ...
 *         if(error)
 *             throw Exception(QString("Unexpected token in line %1.").arg(line));
 *         ...
 *     }
 *     catch(Exception& ex) {
 *         ex.prependGeneralMessage("The input file has an invalid format.");
 *         ex.reportError();
 *     }
 * \endcode
 *
 */
class OVITO_CORE_EXPORT Exception : public QException
{
public:

	/// Creates an exception with a default error message.
	/// You should use the constructor taking a message string instead to construct an Exception.
	Exception(QObject* context = nullptr);

	/// Initializes the Exception object with a message string describing the error that has occurred.
	/// \param message The human-readable message describing the error, which will be displayed by showError().
	/// \param context Pointer to an optional object that provides the context for this exception or error.
	explicit Exception(const QString& message, QObject* context = nullptr);

	/// \brief Multi-message constructor that initializes the Exception object with multiple message string.
	/// \param errorMessages The list of message strings describing the error. The list should be ordered with
	///                      the most general error description first, followed by the more detailed information.
	/// \param context Pointer to an optional object that provides the context for this exception or error.
	explicit Exception(const QStringList& errorMessages, QObject* context = nullptr);

	// Default destructor.
	virtual ~Exception() = default;

	/// \brief Appends a string to the list of message that describes the error in more detail.
	/// \param message A human-readable description.
	/// \return A reference to this Exception object.
	Exception& appendDetailMessage(const QString& message);

	/// Prepends a string to the list of messages that describes the error in a more general way than the existing message strings.
	/// \param message The human-readable description of the error.
	/// \return A reference to this Exception object.
	Exception& prependGeneralMessage(const QString& message);

	/// Sets the list of error messages stored in this exception object.
	/// \param messages The new list of messages, which completely replaces any existing messages.
	void setMessages(const QStringList& messages) { this->_messages = messages; }

	/// Returns the most general message string stored in this Exception object, which describes the occurred error.
	const QString& message() const { return _messages.front(); }

	/// Returns all message strings stored in this Exception object.
	const QStringList& messages() const { return _messages; }

	/// Logs the error message(s) stored in this Exception object by printing them to the console.
	/// No modal dialog box is shown in GUI mode. 
	void logError() const;

	/// Displays the error message(s) stored in the Exception object to the user.
	///
	/// In the graphical program mode, this method will display a modal message box.
	/// In console mode, this method just prints the error messages(s) to the console.
	///
	/// Note that, unless 'blocking' is true, the reporting happens asynchronously in GUI mode. 
	/// The method returns immediately and the error messaeg is displayed to the user at a later time, 
	/// as soon as control returns to the event loop.
	void reportError(bool blocking = false) const;

	/// Returns a pointer to an object that provides the context for this exception or error.
	QObject* context() const { return _context; }

	/// Sets the context object for this exception or error.
	void setContext(QObject* context) { _context = context; }

	//////////////////////////////////////////////////////////////////////////////////////
	// The following two functions are required by the base class QtConcurrent::Exception

	// Raises this exception object.
	virtual void raise() const override { throw *this; }

	// Creates a copy of this exception object.
	virtual Exception* clone() const override { return new Exception(*this); }

private:

	/// The message strings describing the exception.
	/// This list is ordered with the most general error description coming first followed by the more detailed information.
	QStringList _messages;

	/// Pointer to an object that provides the context for this exception or error.
	QPointer<QObject> _context;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


