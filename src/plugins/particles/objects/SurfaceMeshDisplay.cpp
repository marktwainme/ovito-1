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
#include <core/rendering/SceneRenderer.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/animation/controller/Controller.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/util/CapPolygonTessellator.h>
#include "SurfaceMeshDisplay.h"
#include "SurfaceMesh.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SurfaceMeshDisplay, AsynchronousDisplayObject);
DEFINE_FLAGS_PROPERTY_FIELD(SurfaceMeshDisplay, surfaceColor, "SurfaceColor", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(SurfaceMeshDisplay, capColor, "CapColor", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(SurfaceMeshDisplay, showCap, "ShowCap", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(SurfaceMeshDisplay, smoothShading, "SmoothShading");
DEFINE_PROPERTY_FIELD(SurfaceMeshDisplay, reverseOrientation, "ReverseOrientation");
DEFINE_REFERENCE_FIELD(SurfaceMeshDisplay, surfaceTransparencyController, "SurfaceTransparency", Controller);
DEFINE_REFERENCE_FIELD(SurfaceMeshDisplay, capTransparencyController, "CapTransparency", Controller);
SET_PROPERTY_FIELD_LABEL(SurfaceMeshDisplay, surfaceColor, "Surface color");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshDisplay, capColor, "Cap color");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshDisplay, showCap, "Show cap polygons");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshDisplay, smoothShading, "Smooth shading");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshDisplay, surfaceTransparencyController, "Surface transparency");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshDisplay, capTransparencyController, "Cap transparency");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshDisplay, reverseOrientation, "Inside out");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SurfaceMeshDisplay, surfaceTransparencyController, PercentParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SurfaceMeshDisplay, capTransparencyController, PercentParameterUnit, 0, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
SurfaceMeshDisplay::SurfaceMeshDisplay(DataSet* dataset) : AsynchronousDisplayObject(dataset),
	_surfaceColor(1, 1, 1), _capColor(0.8, 0.8, 1.0), _showCap(true), _smoothShading(true), _trimeshUpdate(true), _reverseOrientation(false)
{
	INIT_PROPERTY_FIELD(surfaceColor);
	INIT_PROPERTY_FIELD(capColor);
	INIT_PROPERTY_FIELD(showCap);
	INIT_PROPERTY_FIELD(smoothShading);
	INIT_PROPERTY_FIELD(surfaceTransparencyController);
	INIT_PROPERTY_FIELD(capTransparencyController);
	INIT_PROPERTY_FIELD(reverseOrientation);

	setSurfaceTransparencyController(ControllerManager::createFloatController(dataset));
	setCapTransparencyController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Computes the bounding box of the displayed data.
******************************************************************************/
Box3 SurfaceMeshDisplay::boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState)
{
	// We'll use the entire simulation cell as bounding box for the mesh.
	if(SimulationCellObject* cellObject = flowState.findObject<SimulationCellObject>())
		return Box3(Point3(0,0,0), Point3(1,1,1)).transformed(cellObject->cellMatrix());
	else
		return Box3();
}

/******************************************************************************
* Creates a computation engine that will prepare the data to be displayed.
******************************************************************************/
std::shared_ptr<AsynchronousTask> SurfaceMeshDisplay::createEngine(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState)
{
	// Get the simulation cell.
	SimulationCellObject* cellObject = flowState.findObject<SimulationCellObject>();

	// Get the surface mesh.
	SurfaceMesh* surfaceMeshObj = dynamic_object_cast<SurfaceMesh>(dataObject);

	// Check if input is available.
	if(cellObject && surfaceMeshObj) {
		// Check if the input has changed.
		if(_preparationCacheHelper.updateState(dataObject, cellObject->data(), reverseOrientation())) {
			// Create compute engine.
			return std::make_shared<PrepareSurfaceEngine>(surfaceMeshObj->storage(), cellObject->data(), surfaceMeshObj->isCompletelySolid(), reverseOrientation(), surfaceMeshObj->cuttingPlanes());
		}
	}
	else {
		_surfaceMesh.clear();
		_capPolygonsMesh.clear();
		_trimeshUpdate = true;
	}

	return std::shared_ptr<AsynchronousTask>();
}

/******************************************************************************
* Computes the results and stores them in this object for later retrieval.
******************************************************************************/
void SurfaceMeshDisplay::PrepareSurfaceEngine::perform()
{
	setProgressText(tr("Preparing surface mesh for display"));

	if(!buildSurfaceMesh(*_inputMesh, _simCell, _reverseOrientation, _cuttingPlanes, _surfaceMesh, this))
		throw Exception(tr("Failed to generate non-periodic version of surface mesh for display. Simulation cell might be too small."));

	if(isCanceled())
		return;

	buildCapMesh(*_inputMesh, _simCell, _isCompletelySolid, _reverseOrientation, _cuttingPlanes, _capPolygonsMesh, this);
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the display object.
******************************************************************************/
void SurfaceMeshDisplay::transferComputationResults(AsynchronousTask* engine)
{
	if(engine) {
		_surfaceMesh = static_cast<PrepareSurfaceEngine*>(engine)->surfaceMesh();
		_capPolygonsMesh = static_cast<PrepareSurfaceEngine*>(engine)->capPolygonsMesh();
		_trimeshUpdate = true;
	}
	else {
		// Reset cache when compute task has been canceled.
		_preparationCacheHelper.updateState(nullptr, SimulationCell(), false);
	}
}

/******************************************************************************
* Lets the display object render the data object.
******************************************************************************/
void SurfaceMeshDisplay::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode)
{
	// Check if geometry preparation was successful.
	// If not, reset triangle mesh.
	if(status().type() == PipelineStatus::Error && _surfaceMesh.faceCount() != 0) {
		_surfaceMesh.clear();
		_capPolygonsMesh.clear();
		_trimeshUpdate = true;
	}

	// Get the rendering colors for the surface and cap meshes.
	FloatType transp_surface = 0;
	FloatType transp_cap = 0;
	TimeInterval iv;
	if(surfaceTransparencyController()) transp_surface = surfaceTransparencyController()->getFloatValue(time, iv);
	if(capTransparencyController()) transp_cap = capTransparencyController()->getFloatValue(time, iv);
	ColorA color_surface(surfaceColor(), FloatType(1) - transp_surface);
	ColorA color_cap(capColor(), FloatType(1) - transp_cap);

	// Do we have to re-create the render primitives from scratch?
	bool recreateSurfaceBuffer = !_surfaceBuffer || !_surfaceBuffer->isValid(renderer);
	bool recreateCapBuffer = showCap() && (!_capBuffer || !_capBuffer->isValid(renderer));

	// Do we have to update the render primitives?
	bool updateContents = _geometryCacheHelper.updateState(color_surface, color_cap, smoothShading())
					|| recreateSurfaceBuffer || recreateCapBuffer || _trimeshUpdate;

	// Re-create the render primitives if necessary.
	if(recreateSurfaceBuffer)
		_surfaceBuffer = renderer->createMeshPrimitive();
	if(recreateCapBuffer && _showCap)
		_capBuffer = renderer->createMeshPrimitive();

	// Update render primitives.
	if(updateContents) {

		// Assign smoothing group to faces to interpolate normals.
		const quint32 smoothingGroup = smoothShading() ? 1 : 0;
		for(auto& face : _surfaceMesh.faces())
			face.setSmoothingGroups(smoothingGroup);

		_surfaceBuffer->setMesh(_surfaceMesh, color_surface);
		if(showCap())
			_capBuffer->setMesh(_capPolygonsMesh, color_cap);

		// Reset update flag.
		_trimeshUpdate = false;
	}

	// Handle picking of triangles.
	renderer->beginPickObject(contextNode);
	_surfaceBuffer->render(renderer);
	if(showCap())
		_capBuffer->render(renderer);
	else
		_capBuffer.reset();
	renderer->endPickObject();
}

/******************************************************************************
* Generates the final triangle mesh, which will be rendered.
******************************************************************************/
bool SurfaceMeshDisplay::buildSurfaceMesh(const HalfEdgeMesh<>& input, const SimulationCell& cell, bool reverseOrientation, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseBase* progress)
{
	if(cell.is2D())
		throw Exception(tr("Cannot generate surface triangle mesh when simulation cell is two-dimensional."));

	// Convert half-edge mesh to triangle mesh.
	input.convertToTriMesh(output);

	// Flip orientation of mesh faces if requested.
	if(reverseOrientation)
		output.flipFaces();

	// Check for early abortion.
	if(progress && progress->isCanceled())
		return false;

	// Convert vertex positions to reduced coordinates.
	for(Point3& p : output.vertices()) {
		p = cell.absoluteToReduced(p);
		OVITO_ASSERT(std::isfinite(p.x()) && std::isfinite(p.y()) && std::isfinite(p.z()));
	}

	// Wrap mesh at periodic boundaries.
	for(size_t dim = 0; dim < 3; dim++) {
		if(cell.pbcFlags()[dim] == false) continue;

		if(progress && progress->isCanceled())
			return false;

		// Make sure all vertices are located inside the periodic box.
		for(Point3& p : output.vertices()) {
			OVITO_ASSERT(std::isfinite(p[dim]));
			p[dim] -= floor(p[dim]);
			OVITO_ASSERT(p[dim] >= FloatType(0) && p[dim] <= FloatType(1));
		}

		// Split triangle faces at periodic boundaries.
		int oldFaceCount = output.faceCount();
		int oldVertexCount = output.vertexCount();
		std::vector<Point3> newVertices;
		std::map<std::pair<int,int>,std::pair<int,int>> newVertexLookupMap;
		for(int findex = 0; findex < oldFaceCount; findex++) {
			if(!splitFace(output, output.face(findex), oldVertexCount, newVertices, newVertexLookupMap, cell, dim))
				return false;
		}

		// Insert newly created vertices into mesh.
		output.setVertexCount(oldVertexCount + (int)newVertices.size());
		std::copy(newVertices.cbegin(), newVertices.cend(), output.vertices().begin() + oldVertexCount);
	}

	// Check for early abortion.
	if(progress && progress->isCanceled())
		return false;

	// Convert vertex positions back from reduced coordinates to absolute coordinates.
	AffineTransformation cellMatrix = cell.matrix();
	for(Point3& p : output.vertices())
		p = cellMatrix * p;

	// Clip mesh at cutting planes.
	for(const Plane3& plane : cuttingPlanes) {
		if(progress && progress->isCanceled())
			return false;

		output.clipAtPlane(plane);
	}

	output.invalidateVertices();
	output.invalidateFaces();

	return true;
}

/******************************************************************************
* Splits a triangle face at a periodic boundary.
******************************************************************************/
bool SurfaceMeshDisplay::splitFace(TriMesh& output, TriMeshFace& face, int oldVertexCount, std::vector<Point3>& newVertices,
		std::map<std::pair<int,int>,std::pair<int,int>>& newVertexLookupMap, const SimulationCell& cell, size_t dim)
{
	OVITO_ASSERT(face.vertex(0) != face.vertex(1));
	OVITO_ASSERT(face.vertex(1) != face.vertex(2));
	OVITO_ASSERT(face.vertex(2) != face.vertex(0));

	FloatType z[3];
	for(int v = 0; v < 3; v++)
		z[v] = output.vertex(face.vertex(v))[dim];
	FloatType zd[3] = { z[1] - z[0], z[2] - z[1], z[0] - z[2] };

	OVITO_ASSERT(z[1] - z[0] == -(z[0] - z[1]));
	OVITO_ASSERT(z[2] - z[1] == -(z[1] - z[2]));
	OVITO_ASSERT(z[0] - z[2] == -(z[2] - z[0]));

	if(std::abs(zd[0]) < FloatType(0.5) && std::abs(zd[1]) < FloatType(0.5) && std::abs(zd[2]) < FloatType(0.5))
		return true;	// Face is not crossing the periodic boundary.

	// Create four new vertices (or use existing ones created during splitting of adjacent faces).
	int properEdge = -1;
	int newVertexIndices[3][2];
	for(int i = 0; i < 3; i++) {
		if(std::abs(zd[i]) < FloatType(0.5)) {
			if(properEdge != -1)
				return false;		// The simulation box may be too small or invalid.
			properEdge = i;
			continue;
		}
		int vi1 = face.vertex(i);
		int vi2 = face.vertex((i+1)%3);
		int oi1, oi2;
		if(zd[i] <= FloatType(-0.5)) {
			std::swap(vi1, vi2);
			oi1 = 1; oi2 = 0;
		}
		else {
			oi1 = 0; oi2 = 1;
		}
		auto entry = newVertexLookupMap.find(std::make_pair(vi1, vi2));
		if(entry != newVertexLookupMap.end()) {
			newVertexIndices[i][oi1] = entry->second.first;
			newVertexIndices[i][oi2] = entry->second.second;
		}
		else {
			Vector3 delta = output.vertex(vi2) - output.vertex(vi1);
			delta[dim] -= FloatType(1);
			for(size_t d = dim + 1; d < 3; d++) {
				if(cell.pbcFlags()[d])
					delta[d] -= floor(delta[d] + FloatType(0.5));
			}
			FloatType t;
			if(delta[dim] != 0)
				t = output.vertex(vi1)[dim] / (-delta[dim]);
			else
				t = FloatType(0.5);
			OVITO_ASSERT(std::isfinite(t));
			Point3 p = delta * t + output.vertex(vi1);
			newVertexIndices[i][oi1] = oldVertexCount + (int)newVertices.size();
			newVertexIndices[i][oi2] = oldVertexCount + (int)newVertices.size() + 1;
			newVertexLookupMap.insert(std::make_pair(std::pair<int,int>(vi1, vi2), std::pair<int,int>(newVertexIndices[i][oi1], newVertexIndices[i][oi2])));
			newVertices.push_back(p);
			p[dim] += FloatType(1);
			newVertices.push_back(p);
		}
	}
	OVITO_ASSERT(properEdge != -1);

	// Build output triangles.
	int originalVertices[3] = { face.vertex(0), face.vertex(1), face.vertex(2) };
	face.setVertices(originalVertices[properEdge], originalVertices[(properEdge+1)%3], newVertexIndices[(properEdge+2)%3][1]);

	output.setFaceCount(output.faceCount() + 2);
	TriMeshFace& newFace1 = output.face(output.faceCount() - 2);
	TriMeshFace& newFace2 = output.face(output.faceCount() - 1);
	newFace1.setVertices(originalVertices[(properEdge+1)%3], newVertexIndices[(properEdge+1)%3][0], newVertexIndices[(properEdge+2)%3][1]);
	newFace2.setVertices(newVertexIndices[(properEdge+1)%3][1], originalVertices[(properEdge+2)%3], newVertexIndices[(properEdge+2)%3][0]);

	return true;
}

/******************************************************************************
* Generates the triangle mesh for the PBC caps.
******************************************************************************/
void SurfaceMeshDisplay::buildCapMesh(const HalfEdgeMesh<>& input, const SimulationCell& cell, bool isCompletelySolid, bool reverseOrientation, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseBase* promise)
{
	// Convert vertex positions to reduced coordinates.
	std::vector<Point3> reducedPos(input.vertexCount());
	auto inputVertex = input.vertices().begin();
	for(Point3& p : reducedPos)
		p = cell.absoluteToReduced((*inputVertex++)->pos());

	int isBoxCornerInside3DRegion = -1;

	// Create caps for each periodic boundary.
	for(size_t dim = 0; dim < 3; dim++) {
		if(cell.pbcFlags()[dim] == false) continue;

		if(promise && promise->isCanceled())
			return;

		// Make sure all vertices are located inside the periodic box.
		for(Point3& p : reducedPos) {
			FloatType& c = p[dim];
			OVITO_ASSERT(std::isfinite(c));
			if(FloatType s = floor(c)) c -= s;
			OVITO_ASSERT(std::isfinite(c));
		}

		// Reset 'visited' flag for all faces.
		input.clearFaceFlag(1);

		/// The list of clipped contours.
		std::vector<std::vector<Point2>> openContours;
		std::vector<std::vector<Point2>> closedContours;

		// Find a first edge that crosses the boundary.
		for(HalfEdgeMesh<>::Vertex* vert : input.vertices()) {
			if(promise && promise->isCanceled())
				return;
			for(HalfEdgeMesh<>::Edge* edge = vert->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
				// Skip faces that have already been visited.
				if(edge->face()->testFlag(1)) continue;

				const Point3& v1 = reducedPos[edge->vertex1()->index()];
				const Point3& v2 = reducedPos[edge->vertex2()->index()];
				if(v2[dim] - v1[dim] >= FloatType(0.5)) {
					std::vector<Point2> contour = traceContour(edge, reducedPos, cell, dim);
					if(contour.empty()) throw Exception(tr("Surface mesh is not a proper manifold."));
					clipContour(contour, std::array<bool,2>{{ cell.pbcFlags()[(dim+1)%3], cell.pbcFlags()[(dim+2)%3] }}, openContours, closedContours);
				}
			}
		}

		if(reverseOrientation) {
			for(auto& contour : openContours)
				std::reverse(std::begin(contour), std::end(contour));
		}

		// Feed contours into tessellator to create triangles.
		CapPolygonTessellator tessellator(output, dim);
		tessellator.beginPolygon();
		for(const auto& contour : closedContours) {
			if(promise && promise->isCanceled())
				return;
			tessellator.beginContour();
			for(const Point2& p : contour) {
				tessellator.vertex(p);
			}
			tessellator.endContour();
		}

		// Build the outer contour.
		if(!openContours.empty()) {
			boost::dynamic_bitset<> visitedContours(openContours.size());
			for(auto c1 = openContours.begin(); c1 != openContours.end(); ++c1) {
				if(promise && promise->isCanceled())
					return;
				if(!visitedContours.test(c1 - openContours.begin())) {
					tessellator.beginContour();
					auto currentContour = c1;
					do {
						for(const Point2& p : *currentContour) {
							tessellator.vertex(p);
						}
						visitedContours.set(currentContour - openContours.begin());

						FloatType exitSide = 0;
						if(currentContour->back().x() == 0) exitSide = currentContour->back().y();
						else if(currentContour->back().y() == 1) exitSide = currentContour->back().x() + FloatType(1);
						else if(currentContour->back().x() == 1) exitSide = FloatType(3) - currentContour->back().y();
						else if(currentContour->back().y() == 0) exitSide = FloatType(4) - currentContour->back().x();
						if(exitSide >= FloatType(4)) exitSide = 0;

						// Find the next contour.
						FloatType entrySide;
						FloatType closestDist = FLOATTYPE_MAX;
						for(auto c = openContours.begin(); c != openContours.end(); ++c) {
							FloatType pos = 0;
							if(c->front().x() == 0) pos = c->front().y();
							else if(c->front().y() == 1) pos = c->front().x() + FloatType(1);
							else if(c->front().x() == 1) pos = FloatType(3) - c->front().y();
							else if(c->front().y() == 0) pos = FloatType(4) - c->front().x();
							if(pos >= FloatType(4)) pos = 0;
							FloatType dist = exitSide - pos;
							if(dist < 0) dist += FloatType(4);
							if(dist < closestDist) {
								closestDist = dist;
								currentContour = c;
								entrySide = pos;
							}
						}
						int exitCorner = (int)floor(exitSide);
						int entryCorner = (int)floor(entrySide);
						OVITO_ASSERT(exitCorner >= 0 && exitCorner < 4);
						OVITO_ASSERT(entryCorner >= 0 && entryCorner < 4);
						if(exitCorner != entryCorner || exitSide < entrySide) {
							for(int corner = exitCorner; ;) {
								switch(corner) {
								case 0: tessellator.vertex(Point2(0,0)); break;
								case 1: tessellator.vertex(Point2(0,1)); break;
								case 2: tessellator.vertex(Point2(1,1)); break;
								case 3: tessellator.vertex(Point2(1,0)); break;
								}
								corner = (corner + 3) % 4;
								if(corner == entryCorner) break;
							}
						}
					}
					while(!visitedContours.test(currentContour - openContours.begin()));
					tessellator.endContour();
				}
			}
		}
		else {
			if(isBoxCornerInside3DRegion == -1) {
				if(closedContours.empty())
					isBoxCornerInside3DRegion = isCornerInside3DRegion(input, reducedPos, cell.pbcFlags(), isCompletelySolid);
				else
					isBoxCornerInside3DRegion = isCornerInside2DRegion(closedContours);
				if(reverseOrientation)
					isBoxCornerInside3DRegion = !isBoxCornerInside3DRegion;
			}
			if(isBoxCornerInside3DRegion) {
				tessellator.beginContour();
				tessellator.vertex(Point2(0,0));
				tessellator.vertex(Point2(1,0));
				tessellator.vertex(Point2(1,1));
				tessellator.vertex(Point2(0,1));
				tessellator.endContour();
			}
		}

		tessellator.endPolygon();
	}

	// Check for early abortion.
	if(promise && promise->isCanceled())
		return;

	// Convert vertex positions back from reduced coordinates to absolute coordinates.
	AffineTransformation cellMatrix = cell.matrix();
	for(Point3& p : output.vertices())
		p = cellMatrix * p;

	// Clip mesh at cutting planes.
	for(const Plane3& plane : cuttingPlanes) {
		if(promise && promise->isCanceled())
			return;
		output.clipAtPlane(plane);
	}
}

/******************************************************************************
* Traces the closed contour of the surface-boundary intersection.
******************************************************************************/
std::vector<Point2> SurfaceMeshDisplay::traceContour(HalfEdgeMesh<>::Edge* firstEdge, const std::vector<Point3>& reducedPos, const SimulationCell& cell, size_t dim)
{
	size_t dim1 = (dim + 1) % 3;
	size_t dim2 = (dim + 2) % 3;
	std::vector<Point2> contour;
	HalfEdgeMesh<>::Edge* edge = firstEdge;
	do {
		OVITO_ASSERT(edge->face() != nullptr);
		OVITO_ASSERT(!edge->face()->testFlag(1));

		// Mark face as visited.
		edge->face()->setFlag(1);

		// Compute intersection point.
		const Point3& v1 = reducedPos[edge->vertex1()->index()];
		const Point3& v2 = reducedPos[edge->vertex2()->index()];
		Vector3 delta = v2 - v1;
		OVITO_ASSERT(delta[dim] >= FloatType(0.5));

		delta[dim] -= FloatType(1);
		if(cell.pbcFlags()[dim1]) {
			FloatType& c = delta[dim1];
			if(FloatType s = floor(c + FloatType(0.5)))
				c -= s;
		}
		if(cell.pbcFlags()[dim2]) {
			FloatType& c = delta[dim2];
			if(FloatType s = floor(c + FloatType(0.5)))
				c -= s;
		}
		if(std::abs(delta[dim]) > FloatType(1e-9f)) {
			FloatType t = v1[dim] / delta[dim];
			FloatType x = v1[dim1] - delta[dim1] * t;
			FloatType y = v1[dim2] - delta[dim2] * t;
			OVITO_ASSERT(std::isfinite(x) && std::isfinite(y));
			if(contour.empty() || std::abs(x - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x,y});
			}
		}
		else {
			FloatType x1 = v1[dim1];
			FloatType y1 = v1[dim2];
			FloatType x2 = v1[dim1] + delta[dim1];
			FloatType y2 = v1[dim2] + delta[dim2];
			if(contour.empty() || std::abs(x1 - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y1 - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x1,y1});
			}
			else if(contour.empty() || std::abs(x2 - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y2 - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x2,y2});
			}
		}

		// Find the face edge that crosses the boundary in the reverse direction.
		for(;;) {
			edge = edge->nextFaceEdge();
			const Point3& v1 = reducedPos[edge->vertex1()->index()];
			const Point3& v2 = reducedPos[edge->vertex2()->index()];
			if(v2[dim] - v1[dim] <= FloatType(-0.5))
				break;
		}

		edge = edge->oppositeEdge();
		if(!edge) {
			// Mesh is not closed (not a proper manifold).
			contour.clear();
			break;
		}
	}
	while(edge != firstEdge);

	return contour;
}

/******************************************************************************
* Clips a 2d contour at a periodic boundary.
******************************************************************************/
void SurfaceMeshDisplay::clipContour(std::vector<Point2>& input, std::array<bool,2> pbcFlags, std::vector<std::vector<Point2>>& openContours, std::vector<std::vector<Point2>>& closedContours)
{
	if(!pbcFlags[0] && !pbcFlags[1]) {
		closedContours.push_back(std::move(input));
		return;
	}

	// Ensure all coordinates are within the primary image.
	if(pbcFlags[0]) {
		for(auto& v : input) {
			OVITO_ASSERT(std::isfinite(v.x()));
			if(FloatType s = floor(v.x())) v.x() -= s;
		}
	}
	if(pbcFlags[1]) {
		for(auto& v : input) {
			OVITO_ASSERT(std::isfinite(v.y()));
			if(FloatType s = floor(v.y())) v.y() -= s;
		}
	}

	std::vector<std::vector<Point2>> contours;
	contours.push_back({});

	auto v1 = input.cend() - 1;
	for(auto v2 = input.cbegin(); v2 != input.cend(); v1 = v2, ++v2) {
		contours.back().push_back(*v1);

		Vector2 delta = (*v2) - (*v1);
		if(std::abs(delta.x()) < FloatType(0.5) && std::abs(delta.y()) < FloatType(0.5))
			continue;

		FloatType t[2] = { 2, 2 };
		Vector2I crossDir(0, 0);
		for(size_t dim = 0; dim < 2; dim++) {
			if(pbcFlags[dim]) {
				if(delta[dim] >= FloatType(0.5)) {
					delta[dim] -= FloatType(1);
					if(std::abs(delta[dim]) > FLOATTYPE_EPSILON)
						t[dim] = std::min((*v1)[dim] / -delta[dim], FloatType(1));
					else
						t[dim] = FloatType(0.5);
					crossDir[dim] = -1;
					OVITO_ASSERT(t[dim] >= 0 && t[dim] <= 1);
				}
				else if(delta[dim] <= FloatType(-0.5)) {
					delta[dim] += FloatType(1);
					if(std::abs(delta[dim]) > FLOATTYPE_EPSILON)
						t[dim] = std::max((FloatType(1) - (*v1)[dim]) / delta[dim], FloatType(0));
					else
						t[dim] = FloatType(0.5);
					crossDir[dim] = +1;
					OVITO_ASSERT(t[dim] >= 0 && t[dim] <= 1);
				}
			}
		}

		Point2 base = *v1;
		if(t[0] < t[1]) {
			OVITO_ASSERT(t[0] <= 1);
			computeContourIntersection(0, t[0], base, delta, crossDir[0], contours);
			if(crossDir[1] != 0) {
				OVITO_ASSERT(t[1] <= 1);
				computeContourIntersection(1, t[1], base, delta, crossDir[1], contours);
			}
		}
		else if(t[1] < t[0]) {
			OVITO_ASSERT(t[1] <= 1);
			computeContourIntersection(1, t[1], base, delta, crossDir[1], contours);
			if(crossDir[0] != 0) {
				OVITO_ASSERT(t[0] <= 1);
				computeContourIntersection(0, t[0], base, delta, crossDir[0], contours);
			}
		}
	}

	if(contours.size() == 1) {
		closedContours.push_back(std::move(contours.back()));
	}
	else {
		auto& firstSegment = contours.front();
		auto& lastSegment = contours.back();
		firstSegment.insert(firstSegment.begin(), lastSegment.begin(), lastSegment.end());
		contours.pop_back();
		for(auto& contour : contours) {
			bool isDegenerate = std::all_of(contour.begin(), contour.end(), [&contour](const Point2& p) { 
				return p.equals(contour.front()); 
			});
			if(!isDegenerate)
				openContours.push_back(std::move(contour));
		}
	}
}

/******************************************************************************
* Computes the intersection point of a 2d contour segment crossing a
* periodic boundary.
******************************************************************************/
void SurfaceMeshDisplay::computeContourIntersection(size_t dim, FloatType t, Point2& base, Vector2& delta, int crossDir, std::vector<std::vector<Point2>>& contours)
{
	OVITO_ASSERT(std::isfinite(t));
	Point2 intersection = base + t * delta;
	intersection[dim] = (crossDir == -1) ? 0 : 1;
	contours.back().push_back(intersection);
	intersection[dim] = (crossDir == +1) ? 0 : 1;
	contours.push_back({intersection});
	base = intersection;
	delta *= (FloatType(1) - t);
}

/******************************************************************************
* Determines if the 2D box corner (0,0) is inside the closed region described
* by the 2d polygon.
*
* 2D version of the algorithm:
*
* J. Andreas Baerentzen and Henrik Aanaes
* Signed Distance Computation Using the Angle Weighted Pseudonormal
* IEEE Transactions on Visualization and Computer Graphics 11 (2005), Page 243
******************************************************************************/
bool SurfaceMeshDisplay::isCornerInside2DRegion(const std::vector<std::vector<Point2>>& contours)
{
	OVITO_ASSERT(!contours.empty());
	bool isInside = true;

	// Determine which vertex is closest to the test point.
	std::vector<Point2>::const_iterator closestVertex = contours.front().end();
	FloatType closestDistanceSq = FLOATTYPE_MAX;
	for(const auto& contour : contours) {
		auto v1 = contour.end() - 1;
		for(auto v2 = contour.begin(); v2 != contour.end(); v1 = v2++) {
			Vector2 r = (*v1) - Point2::Origin();
			FloatType distanceSq = r.squaredLength();
			if(distanceSq < closestDistanceSq) {
				closestDistanceSq = distanceSq;
				closestVertex = v1;

				// Compute pseuso-normal at vertex.
				auto v0 = (v1 == contour.begin()) ? (contour.end() - 1) : (v1 - 1);
				Vector2 edgeDir = (*v2) - (*v0);
				Vector2 normal(edgeDir.y(), -edgeDir.x());
				isInside = (normal.dot(r) > 0);
			}

			// Check if any edge is closer to the test point.
			Vector2 edgeDir = (*v2) - (*v1);
			FloatType edgeLength = edgeDir.length();
			if(edgeLength <= FLOATTYPE_EPSILON) continue;
			edgeDir /= edgeLength;
			FloatType d = -edgeDir.dot(r);
			if(d <= 0 || d >= edgeLength) continue;
			Vector2 c = r + edgeDir * d;
			distanceSq = c.squaredLength();
			if(distanceSq < closestDistanceSq) {
				closestDistanceSq = distanceSq;

				// Compute normal at edge.
				Vector2 normal(edgeDir.y(), -edgeDir.x());
				isInside = (normal.dot(c) > 0);
			}
		}
	}

	return isInside;
}

/******************************************************************************
* Determines if the 3D box corner (0,0,0) is inside the region described by
* the half-edge polyhedron.
*
* Algorithm:
*
* J. Andreas Baerentzen and Henrik Aanaes
* Signed Distance Computation Using the Angle Weighted Pseudonormal
* IEEE Transactions on Visualization and Computer Graphics 11 (2005), Page 243
******************************************************************************/
bool SurfaceMeshDisplay::isCornerInside3DRegion(const HalfEdgeMesh<>& mesh, const std::vector<Point3>& reducedPos, const std::array<bool,3> pbcFlags, bool isCompletelySolid)
{
	// Determine which vertex is closest to the test point.
	FloatType closestDistanceSq = FLOATTYPE_MAX;
	HalfEdgeMesh<>::Vertex* closestVertex = nullptr;
	Vector3 closestNormal, closestVector;
	for(HalfEdgeMesh<>::Vertex* v : mesh.vertices()) {
		if(v->edges() == nullptr) continue;
		Vector3 r = reducedPos[v->index()] - Point3::Origin();
		for(size_t k = 0; k < 3; k++) {
			if(pbcFlags[k]) {
				if(FloatType s = floor(r[k] + FloatType(0.5)))
					r[k] -= s;
			}
		}
		FloatType distSq = r.squaredLength();
		if(distSq < closestDistanceSq) {
			closestDistanceSq = distSq;
			closestVertex = v;
			closestVector = r;
		}
	}

	if(!closestVertex)
		return isCompletelySolid;

	// Check if any edge is closer to the test point than the closest vertex.
	for(HalfEdgeMesh<>::Vertex* v : mesh.vertices()) {
		for(HalfEdgeMesh<>::Edge* edge = v->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
			OVITO_ASSERT_MSG(edge->oppositeEdge() != nullptr, "SurfaceMeshDisplay::isCornerInside3DRegion", "Surface mesh is not fully closed. This should not happen.");
			const Point3& p1 = reducedPos[edge->vertex1()->index()];
			const Point3& p2 = reducedPos[edge->vertex2()->index()];
			Vector3 edgeDir = p2 - p1;
			Vector3 r = p1 - Point3::Origin();
			for(size_t k = 0; k < 3; k++) {
				if(pbcFlags[k]) {
					if(FloatType s = floor(r[k] + FloatType(0.5)))
						r[k] -= s;
					if(FloatType s = floor(edgeDir[k] + FloatType(0.5)))
						edgeDir[k] -= s;
				}
			}
			FloatType edgeLength = edgeDir.length();
			if(edgeLength <= FLOATTYPE_EPSILON) continue;
			edgeDir /= edgeLength;
			FloatType d = -edgeDir.dot(r);
			if(d <= 0 || d >= edgeLength) continue;
			Vector3 c = r + edgeDir * d;
			FloatType distSq = c.squaredLength();
			if(distSq < closestDistanceSq) {
				closestDistanceSq = distSq;
				closestVertex = nullptr;
				closestVector = c;
				Vector3 e1 = reducedPos[edge->nextFaceEdge()->vertex2()->index()] - p1;
				Vector3 e2 = reducedPos[edge->oppositeEdge()->nextFaceEdge()->vertex2()->index()] - p1;
				for(size_t k = 0; k < 3; k++) {
					if(pbcFlags[k]) {
						if(FloatType s = floor(e1[k] + FloatType(0.5)))
							e1[k] -= s;
						if(FloatType s = floor(e2[k] + FloatType(0.5)))
							e2[k] -= s;
					}
				}
				closestNormal = edgeDir.cross(e1).normalized() + e2.cross(edgeDir).normalized();
			}
		}
	}

	// Check if any facet is closer to the test point than the closest vertex and the closest edge.
	HalfEdgeMesh<>::Face* closestFace = nullptr;
	for(HalfEdgeMesh<>::Face* face : mesh.faces()) {
		HalfEdgeMesh<>::Edge* edge1 = face->edges();
		HalfEdgeMesh<>::Edge* edge2 = edge1->nextFaceEdge();
		const Point3& p1 = reducedPos[edge1->vertex1()->index()];
		const Point3& p2 = reducedPos[edge1->vertex2()->index()];
		const Point3& p3 = reducedPos[edge2->vertex2()->index()];
		Vector3 edgeVectors[3];
		edgeVectors[0] = p2 - p1;
		edgeVectors[1] = p3 - p2;
		Vector3 r = p1 - Point3::Origin();
		for(size_t k = 0; k < 3; k++) {
			if(pbcFlags[k]) {
				if(FloatType s = floor(r[k] + FloatType(0.5)))
					r[k] -= s;
				if(FloatType s = floor(edgeVectors[0][k] + FloatType(0.5)))
					edgeVectors[0][k] -= s;
				if(FloatType s = floor(edgeVectors[1][k] + FloatType(0.5)))
					edgeVectors[1][k] -= s;
			}
		}
		edgeVectors[2] = -edgeVectors[1] - edgeVectors[0];

		Vector3 normal = edgeVectors[0].cross(edgeVectors[1]);
		bool isInsideTriangle = true;
		Vector3 vertexVector = r;
		for(size_t v = 0; v < 3; v++) {
			if(vertexVector.dot(normal.cross(edgeVectors[v])) >= 0.0) {
				isInsideTriangle = false;
				break;
			}
			vertexVector += edgeVectors[v];
		}
		if(isInsideTriangle) {
			FloatType normalLengthSq = normal.squaredLength();
			if(std::abs(normalLengthSq) <= FLOATTYPE_EPSILON) continue;
			normal /= sqrt(normalLengthSq);
			FloatType planeDist = normal.dot(r);
			if(planeDist * planeDist < closestDistanceSq) {
				closestDistanceSq = planeDist * planeDist;
				closestVector = normal * planeDist;
				closestVertex = nullptr;
				closestNormal = normal;
			}
		}
	}

	// If a vertex is closest, we still have to compute the local pseudo-normal at the vertex.
	if(closestVertex != nullptr) {
		HalfEdgeMesh<>::Edge* edge = closestVertex->edges();
		OVITO_ASSERT(edge != nullptr);
		closestNormal.setZero();
		Vector3 edge1v = reducedPos[edge->vertex2()->index()] - reducedPos[closestVertex->index()];
		for(size_t k = 0; k < 3; k++) {
			if(pbcFlags[k]) {
				if(FloatType s = floor(edge1v[k] + FloatType(0.5)))
					edge1v[k] -= s;
			}
		}
		edge1v.normalizeSafely();
		do {
			HalfEdgeMesh<>::Edge* nextEdge = edge->oppositeEdge()->nextFaceEdge();
			OVITO_ASSERT(nextEdge->vertex1() == closestVertex);
			Vector3 edge2v = reducedPos[nextEdge->vertex2()->index()] - reducedPos[closestVertex->index()];
			for(size_t k = 0; k < 3; k++) {
				if(pbcFlags[k]) {
					if(FloatType s = floor(edge2v[k] + FloatType(0.5)))
						edge2v[k] -= s;
				}
			}
			edge2v.normalizeSafely();
			FloatType angle = acos(edge1v.dot(edge2v));
			Vector3 normal = edge2v.cross(edge1v);
			if(normal != Vector3::Zero())
				closestNormal += normal.normalized() * angle;
			edge = nextEdge;
			edge1v = edge2v;
		}
		while(edge != closestVertex->edges());
	}

	return closestNormal.dot(closestVector) > 0;
}

}	// End of namespace
}	// End of namespace
