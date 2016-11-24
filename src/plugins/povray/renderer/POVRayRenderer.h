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

#ifndef __OVITO_POVRAY_RENDERER_H
#define __OVITO_POVRAY_RENDERER_H

#include <core/Core.h>
#include <core/rendering/noninteractive/NonInteractiveSceneRenderer.h>

#include <QTemporaryFile>

#ifdef POVRay_EXPORTS		// This is defined by CMake when building the plugin library.
#  define OVITO_POVRAY_EXPORT Q_DECL_EXPORT
#else
#  define OVITO_POVRAY_EXPORT Q_DECL_IMPORT
#endif

namespace Ovito { namespace POVRay {

/**
 * \brief A scene renderer that calls the external POV-Ray ray-tracing engine.
 */
class OVITO_POVRAY_EXPORT POVRayRenderer : public NonInteractiveSceneRenderer
{
public:

	/// Constructor.
	Q_INVOKABLE POVRayRenderer(DataSet* dataset);

	///	Prepares the renderer for rendering of the given scene.
	/// Throws an exception on error. Returns false when the operation has been aborted by the user.
	virtual bool startRender(DataSet* dataset, RenderSettings* settings) override;

	/// This method is called just before renderFrame() is called.
	/// Sets the view projection parameters, the animation frame to render,
	/// and the viewport who is being rendered.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// Renders a single animation frame into the given frame buffer.
	/// Throws an exception on error. Returns false when the operation has been aborted by the user.
	virtual bool renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, AbstractProgressDisplay* progress) override;

	/// This method is called after renderFrame() has been called.
	virtual void endFrame() override;

	///	Finishes the rendering pass. This is called after all animation frames have been rendered
	/// or when the rendering operation has been aborted.
	virtual void endRender() override;

	/// Renders the line geometry stored in the given buffer.
	virtual void renderLines(const DefaultLinePrimitive& lineBuffer) override;

	/// Renders the particles stored in the given buffer.
	virtual void renderParticles(const DefaultParticlePrimitive& particleBuffer) override;

	/// Renders the arrow elements stored in the given buffer.
	virtual void renderArrows(const DefaultArrowPrimitive& arrowBuffer) override;

	/// Renders the text stored in the given buffer.
	virtual void renderText(const DefaultTextPrimitive& textBuffer, const Point2& pos, int alignment) override;

	/// Renders the image stored in the given buffer.
	virtual void renderImage(const DefaultImagePrimitive& imageBuffer, const Point2& pos, const Vector2& size) override;

	/// Renders the triangle mesh stored in the given buffer.
	virtual void renderMesh(const DefaultMeshPrimitive& meshBuffer) override;

	/// Sets the (open) I/O device to which the renderer should write the POV-Ray scene.
	void setScriptOutputDevice(QIODevice* device) {
		_outputStream.setDevice(device);
	}

	/// Returns the path to the external POV-Ray executable.
	const QString& povrayExecutable() const { return _povrayExecutable; }

	///Sets  the path to the external POV-Ray executable.
	void setPovrayExecutable(const QString& path) { _povrayExecutable = path; }

	/// Returns the POV-Ray quality level to use for rendering (0 <= level <= 11).
	int qualityLevel() const { return _qualityLevel; }

	/// Sets the POV-Ray quality level to use for rendering (0 <= level <= 11).
	void setQualityLevel(int level) { _qualityLevel = level; }

	/// Returns whether anti-aliasing is turned on.
	bool antialiasingEnabled() const { return _enableAntialiasing; }

	/// Turns anti-aliasing on/off
	void setAntialiasingEnabled(bool enable) { _enableAntialiasing = enable; }

	/// Returns whether the POV-Ray rendering window is shown.
	bool povrayDisplayEnabled() const { return _povrayDisplayEnabled; }

	/// Turns the POV-Ray rendering window on or off.
	void setPovrayDisplayEnabled(bool enable) { _povrayDisplayEnabled = enable; }

	/// Returns whether radiosity is turned on.
	bool radiosityEnabled() const { return _enableRadiosity; }

	/// Turns radiosity on or off.
	void setRadiosityEnabled(bool enable) { _enableRadiosity = enable; }

	/// Returns the number of rays that are sent out whenever a new radiosity value has to be calculated.
	int radiosityRayCount() const { return _radiosityRayCount; }

	/// Sets the number of rays that are sent out whenever a new radiosity value has to be calculated.
	void setRadiosityRayCount(int count) { _radiosityRayCount = count; }
	
	/// Returns whether depth-of-field rendering is enabled.
	bool depthOfFieldEnabled() const { return _depthOfFieldEnabled; }

	/// Enables/disables depth-of-field rendering.
	void setDepthOfFieldEnabled(bool enabled) { _depthOfFieldEnabled = enabled; }

	/// Returns the camera's focal length, which is used for depth-of-field rendering.
	FloatType dofFocalLength() const { return _dofFocalLength; }

	/// Sets the camera's focal length, which is used for depth-of-field rendering.
	void setDofFocalLength(FloatType length) { _dofFocalLength = length; }

	/// Returns the camera's aperture, which is used for depth-of-field rendering.
	FloatType dofAperture() const { return _dofAperture; }

	/// Sets the camera's aperture, which is used for depth-of-field rendering.
	void setDofAperture(FloatType aperture) { _dofAperture = aperture; }

	/// Returns the number of sampling rays used for focal blur.
	int dofSampleCount() const { return _dofSampleCount; }

	/// Sets the number of sampling rays used for focal blur.
	void setDofSampleCount(int count) { _dofSampleCount = count; }

private:

    /// Writes a 3d vector to the output stream in POV-Ray format.
	void write(const Vector3& v) {
		_outputStream << "<" << v.x() << ", " << v.z() << ", " << v.y() << ">";
	}

    /// Writes a 3d point to the output stream in POV-Ray format.
	void write(const Point3& p) {
		_outputStream << "<" << p.x() << ", " << p.z() << ", " << p.y() << ">";
	}

    /// Writes a color to the output stream in POV-Ray format.
	void write(const Color& c) {
		_outputStream << "rgb <" << c.r() << ", " << c.g() << ", " << c.b() << ">";
	}

    /// Writes a color with alpha channel to the output stream in POV-Ray format.
	void write(const ColorA& c) {
		_outputStream << "rgbt <" << c.r() << ", " << c.g() << ", " << c.b() << ", " << std::max(FloatType(0), FloatType(1) - c.a()) << ">";
	}

    /// Writes a matrix to the output stream in POV-Ray format.
	void write(const AffineTransformation& m) {
		_outputStream << "<";
		_outputStream << m(0,0) << ", " << m(2,0) << ", " << m(1,0) << ", ";
		_outputStream << m(0,2) << ", " << m(2,2) << ", " << m(1,2) << ", ";
		_outputStream << m(0,1) << ", " << m(2,1) << ", " << m(1,1) << ", ";
		_outputStream << m(0,3) << ", " << m(2,3) << ", " << m(1,3);
		_outputStream << ">";
	}

private:

	/// List of image primitives that need to be painted over the final image.
	std::vector<std::tuple<QImage,Point2,Vector2>> _imageDrawCalls;

	/// List of text primitives that need to be painted over the final image.
	std::vector<std::tuple<QString,ColorA,QFont,Point2,int>> _textDrawCalls;

	/// The stream which the POVRay script is written to. 
	QTextStream _outputStream;

	/// The temporary file for passing the scene data to POV-Ray.
	std::unique_ptr<QTemporaryFile> _sceneFile;

	/// The temporary file for receiving the rendered image from POV-Ray. 
	std::unique_ptr<QTemporaryFile> _imageFile;

	/// The POV-Ray quality level to use for rendering (0 <= level <= 11).
	/// See POV-Ray documentation for details.
	PropertyField<int> _qualityLevel;

	/// Turns anti-aliasing on/off
	PropertyField<bool> _enableAntialiasing;

	/// Controls the AA sampling method (only 1 or 2 are valid).
	PropertyField<int> _samplingMethod;

	/// Controls the anti-aliasing threshold.
	PropertyField<FloatType> _AAThreshold;

	/// Controls the number of AA samples.
	PropertyField<int> _antialiasDepth;

	/// Turns on AA-jitter.
	PropertyField<bool> _enableJitter;

	/// Shows or supresses the POV-Ray rendering window.
	PropertyField<bool> _povrayDisplayEnabled;

	/// Turns on radiosity.
	PropertyField<bool> _enableRadiosity;

	/// Controls the number of rays that are sent out whenever a new radiosity value has to be calculated.
	PropertyField<int> _radiosityRayCount;

	/// Determines how many recursion levels are used to calculate the diffuse inter-reflection.
	PropertyField<int> _radiosityRecursionLimit;

	/// Controls the fraction of error tolerated for the radiosity calculation.
	PropertyField<FloatType> _radiosityErrorBound;

	/// Enables depth-of-field rendering.
	PropertyField<bool> _depthOfFieldEnabled;

	/// Controls the camera's focal length, which is used for depth-of-field rendering.
	PropertyField<FloatType> _dofFocalLength;

	/// Controls the camera's aperture, which is used for depth-of-field rendering.
	PropertyField<FloatType> _dofAperture;	

	/// Controls the number of sampling rays used for focal blur.
	PropertyField<int> _dofSampleCount;

	/// Path to the external POV-Ray executable.
	PropertyField<QString> _povrayExecutable;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "POV-Ray renderer");

	DECLARE_PROPERTY_FIELD(_qualityLevel);
	DECLARE_PROPERTY_FIELD(_enableAntialiasing);
	DECLARE_PROPERTY_FIELD(_samplingMethod);
	DECLARE_PROPERTY_FIELD(_AAThreshold);
	DECLARE_PROPERTY_FIELD(_antialiasDepth);
	DECLARE_PROPERTY_FIELD(_enableJitter);
	DECLARE_PROPERTY_FIELD(_povrayDisplayEnabled);
	DECLARE_PROPERTY_FIELD(_enableRadiosity);
	DECLARE_PROPERTY_FIELD(_radiosityRayCount);
	DECLARE_PROPERTY_FIELD(_radiosityRecursionLimit);
	DECLARE_PROPERTY_FIELD(_radiosityErrorBound);
	DECLARE_PROPERTY_FIELD(_depthOfFieldEnabled);
	DECLARE_PROPERTY_FIELD(_dofFocalLength);
	DECLARE_PROPERTY_FIELD(_dofAperture);
	DECLARE_PROPERTY_FIELD(_dofSampleCount);
	DECLARE_PROPERTY_FIELD(_povrayExecutable);

	friend class POVRayExporter;
};

}	// End of namespace
}	// End of namespace

#endif // __OVITO_POVRAY_RENDERER_H
