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

#ifndef __OVITO_OPENGL_BUFFER_H
#define __OVITO_OPENGL_BUFFER_H

#include <core/Core.h>
#include "OpenGLHelpers.h"
#include "OpenGLSceneRenderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <qopengl.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A wrapper for the QOpenGLBuffer class, which adds more features.
 */
template<typename T>
class OpenGLBuffer
{
public:

	typedef T value_type;

	/// Constructor.
	OpenGLBuffer(QOpenGLBuffer::Type type = QOpenGLBuffer::VertexBuffer) : _elementCount(0), _verticesPerElement(0), _buffer(type) {}

	/// Creates the buffer object in the OpenGL server. This function must be called with a current QOpenGLContext.
	/// The buffer will be bound to and can only be used in that context (or any other context that is shared with it).
	bool create(QOpenGLBuffer::UsagePattern usagePattern, int elementCount, int verticesPerElement = 1) {
		OVITO_ASSERT(verticesPerElement >= 1);
		OVITO_ASSERT(elementCount >= 0);
		OVITO_ASSERT(elementCount < std::numeric_limits<int>::max() / sizeof(T) / verticesPerElement);
		if(_elementCount != elementCount || _verticesPerElement != verticesPerElement) {
			_elementCount = elementCount;
			_verticesPerElement = verticesPerElement;
			if(!_buffer.isCreated()) {
				if(!_buffer.create())
					throw Exception(QStringLiteral("Failed to create OpenGL vertex buffer."));
				_buffer.setUsagePattern(usagePattern);
			}
			if(!_buffer.bind())
				throw Exception(QStringLiteral("Failed to bind OpenGL vertex buffer."));
			_buffer.allocate(sizeof(T) * _elementCount * _verticesPerElement);
            OVITO_REPORT_OPENGL_ERRORS();
			_buffer.release();
			return true;
		}
		else {
			OVITO_ASSERT(isCreated());
			return false;
		}
	}

	/// Returns true if this buffer has been created; false otherwise.
	bool isCreated() const { return _buffer.isCreated(); }

	/// Returns the number of elements stored in this buffer.
	int elementCount() const { return _elementCount; }

	/// Returns the number of vertices rendered per element.
	int verticesPerElement() const { return _verticesPerElement; }

	/// Provides access to the internal OpenGL vertex buffer object.
	QOpenGLBuffer& oglBuffer() { return _buffer; }

	/// Destroys this buffer object, including the storage being used in the OpenGL server.
	void destroy() {
		_buffer.destroy();
		_elementCount = 0;
		_verticesPerElement = 0;
	}

	/// Maps the contents of this buffer into the application's memory space and returns a pointer to it.
	T* map(QOpenGLBuffer::Access access) {
		OVITO_ASSERT(isCreated());
		if(elementCount() == 0)
			return nullptr;
		if(!_buffer.bind())
			throw Exception(QStringLiteral("Failed to bind OpenGL vertex buffer."));
		T* data = static_cast<T*>(_buffer.map(access));
		if(!data)
			throw Exception(QStringLiteral("Failed to map OpenGL vertex buffer to memory."));
        OVITO_REPORT_OPENGL_ERRORS();
		return data;
	}

	/// Unmaps the buffer after it was mapped into the application's memory space with a previous call to map().
	void unmap() {
		if(elementCount() == 0)
			return;
		if(!_buffer.unmap())
			throw Exception(QStringLiteral("Failed to unmap OpenGL vertex buffer from memory."));
		_buffer.release();
        OVITO_REPORT_OPENGL_ERRORS();
	}

	/// Fills the vertex buffer with the given data.
	template<typename U>
	void fill(const U* data) {
		OVITO_ASSERT(isCreated());
		OVITO_ASSERT(_elementCount >= 0);
		OVITO_ASSERT(_verticesPerElement >= 1);

		if(!_buffer.bind())
			throw Exception(QStringLiteral("Failed to bind OpenGL vertex buffer."));
		if(_verticesPerElement == 1 && std::is_same<T,U>::value) {
			_buffer.write(0, data, _elementCount * sizeof(T));
		}
		else {
			if(_elementCount) {
				T* bufferData = static_cast<T*>(_buffer.map(QOpenGLBuffer::WriteOnly));
				OVITO_CHECK_POINTER(bufferData);
				if(!bufferData)
					throw Exception(QStringLiteral("Failed to map OpenGL vertex buffer to memory."));
				const U* endData = data + _elementCount;
				for(; data != endData; ++data) {
					for(int i = 0; i < _verticesPerElement; i++, ++bufferData) {
						*bufferData = (T)*data;
					}
				}
				_buffer.unmap();
			}
		}
		_buffer.release();
        OVITO_REPORT_OPENGL_ERRORS();
	}

	/// Fills the buffer with a constant value.
	template<typename U>
	void fillConstant(U value) {
		OVITO_ASSERT(isCreated());
		OVITO_ASSERT(_elementCount >= 0);
		OVITO_ASSERT(_verticesPerElement >= 1);

		if(!_buffer.bind())
			throw Exception(QStringLiteral("Failed to bind OpenGL vertex buffer."));
		if(_elementCount) {
			T* bufferData = static_cast<T*>(_buffer.map(QOpenGLBuffer::WriteOnly));
			OVITO_CHECK_POINTER(bufferData);
			if(!bufferData)
				throw Exception(QStringLiteral("Failed to map OpenGL vertex buffer to memory."));
			std::fill(bufferData, bufferData + _elementCount * _verticesPerElement, (T)value);
			_buffer.unmap();
		}
		_buffer.release();
        OVITO_REPORT_OPENGL_ERRORS();
	}

	/// Binds this buffer to a vertex attribute of a vertex shader.
	void bind(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, const char* attributeName, GLenum type, int offset, int tupleSize, int stride = 0) {
		OVITO_ASSERT(isCreated());
		OVITO_ASSERT(type != GL_FLOAT || (sizeof(T) == sizeof(GLfloat)*tupleSize && stride == 0) || sizeof(T) == stride);
		OVITO_ASSERT(type != GL_INT || (sizeof(T) == sizeof(GLint)*tupleSize && stride == 0) || sizeof(T) == stride);
		if(!_buffer.bind())
			throw Exception(QStringLiteral("Failed to bind OpenGL vertex buffer."));
		if(stride == 0) stride = sizeof(T);
		OVITO_CHECK_OPENGL(shader->enableAttributeArray(attributeName));
		OVITO_CHECK_OPENGL(shader->setAttributeBuffer(attributeName, type, offset, tupleSize, stride));
		_buffer.release();
	}

	/// After rendering is done, release the binding of the buffer to a shader attribute.
	void detach(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, const char* attributeName) {
		OVITO_CHECK_OPENGL(shader->disableAttributeArray(attributeName));
	}

	/// Binds this buffer to the vertex position attribute of a vertex shader.
	void bindPositions(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, size_t byteOffset = 0) {
		OVITO_ASSERT(isCreated());
		OVITO_STATIC_ASSERT(sizeof(T) >= sizeof(GLfloat)*3);

		if(!_buffer.bind())
			throw Exception(QStringLiteral("Failed to bind OpenGL vertex positions buffer."));

		if(renderer->glformat().majorVersion() >= 3) {
			OVITO_CHECK_OPENGL(shader->enableAttributeArray("position"));
			OVITO_CHECK_OPENGL(shader->setAttributeBuffer("position", GL_FLOAT, byteOffset, 3, sizeof(T)));
		}
		else if(renderer->oldGLFunctions()) {
			// Older OpenGL implementations cannot take vertex coordinates through a custom shader attribute.
			OVITO_CHECK_OPENGL(renderer->oldGLFunctions()->glEnableClientState(GL_VERTEX_ARRAY));
			OVITO_CHECK_OPENGL(renderer->oldGLFunctions()->glVertexPointer(3, GL_FLOAT, sizeof(T), reinterpret_cast<const GLvoid*>(byteOffset)));
		}
		_buffer.release();
	}

	/// After rendering is done, release the binding of the buffer to the vertex position attribute.
	void detachPositions(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader) {
		if(renderer->glformat().majorVersion() >= 3) {
			OVITO_CHECK_OPENGL(shader->disableAttributeArray("position"));
		}
		else if(renderer->oldGLFunctions()) {
			OVITO_CHECK_OPENGL(renderer->oldGLFunctions()->glDisableClientState(GL_VERTEX_ARRAY));
		}
	}

	/// Binds this buffer to the vertex color attribute of a vertex shader.
	void bindColors(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, int components, size_t byteOffset = 0) {
		OVITO_ASSERT(isCreated());
		OVITO_ASSERT(sizeof(T) >= sizeof(GLfloat)*components);
		OVITO_ASSERT(components == 3 || components == 4);

		if(!_buffer.bind())
			throw Exception(QStringLiteral("Failed to bind OpenGL vertex color buffer."));

		if(renderer->glformat().majorVersion() >= 3) {
			OVITO_CHECK_OPENGL(shader->enableAttributeArray("color"));
			OVITO_CHECK_OPENGL(shader->setAttributeBuffer("color", GL_FLOAT, byteOffset, components, sizeof(T)));
		}
		else if(renderer->oldGLFunctions()) {
			// Older OpenGL implementations cannot take vertex colors through a custom shader attribute.
			OVITO_CHECK_OPENGL(renderer->oldGLFunctions()->glEnableClientState(GL_COLOR_ARRAY));
			OVITO_CHECK_OPENGL(renderer->oldGLFunctions()->glColorPointer(components, GL_FLOAT, sizeof(T), reinterpret_cast<const GLvoid*>(byteOffset)));
		}
		_buffer.release();
	}

	/// After rendering is done, release the binding of the buffer to the vertex color attribute.
	void detachColors(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader) {
		if(renderer->glformat().majorVersion() >= 3) {
			OVITO_CHECK_OPENGL(shader->disableAttributeArray("color"));
		}
		else if(renderer->oldGLFunctions()) {
			OVITO_CHECK_OPENGL(renderer->oldGLFunctions()->glDisableClientState(GL_COLOR_ARRAY));
		}
	}

	/// Binds this buffer to the vertex normal attribute of a vertex shader.
	void bindNormals(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, size_t byteOffset = 0) {
		OVITO_ASSERT(isCreated());
		OVITO_STATIC_ASSERT(sizeof(T) >= sizeof(GLfloat)*3);

		if(!_buffer.bind())
			throw Exception(QStringLiteral("Failed to bind OpenGL vertex normal buffer."));

		if(renderer->glformat().majorVersion() >= 3) {
			OVITO_CHECK_OPENGL(shader->enableAttributeArray("normal"));
			OVITO_CHECK_OPENGL(shader->setAttributeBuffer("normal", GL_FLOAT, byteOffset, 3, sizeof(T)));
		}
		else if(renderer->oldGLFunctions()) {
			// Older OpenGL implementations cannot take vertex normals through a custom shader attribute.
			OVITO_CHECK_OPENGL(renderer->oldGLFunctions()->glEnableClientState(GL_NORMAL_ARRAY));
			OVITO_CHECK_OPENGL(renderer->oldGLFunctions()->glNormalPointer(GL_FLOAT, sizeof(T), reinterpret_cast<const GLvoid*>(byteOffset)));
		}
		_buffer.release();
	}

	/// After rendering is done, release the binding of the buffer to the vertex normal attribute.
	void detachNormals(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader) {
		if(renderer->glformat().majorVersion() >= 3) {
			OVITO_CHECK_OPENGL(shader->disableAttributeArray("normal"));
		}
		else if(renderer->oldGLFunctions()) {
			OVITO_CHECK_OPENGL(renderer->oldGLFunctions()->glDisableClientState(GL_NORMAL_ARRAY));
		}
	}

private:

	/// The OpenGL vertex buffer.
	QOpenGLBuffer _buffer;

	/// The number of elements stored in the buffer.
	int _elementCount;

	/// The number of vertices per element.
	int _verticesPerElement;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_OPENGL_BUFFER_H
