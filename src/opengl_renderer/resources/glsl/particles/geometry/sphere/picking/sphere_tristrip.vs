///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
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

// Inputs from calling program:
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 modelviewprojection_matrix;
uniform int pickingBaseID;
uniform vec3 cubeVerts[14];
uniform float radius_scalingfactor;

#if __VERSION__ >= 130

	// The particle data:
	in vec3 position;
	in float particle_radius;
	in float vertexID;
	
	// Outputs to fragment shader
	flat out vec4 particle_color_fs;
	flat out float particle_radius_squared_fs;
	flat out vec3 particle_view_pos_fs;

#else

	// The particle data:
	attribute float particle_radius;
	attribute float vertexID;

	// Output to fragment shader:
	#define particle_radius_squared_fs gl_TexCoord[1].w
	#define particle_view_pos_fs gl_TexCoord[1].xyz
	
#endif

void main()
{
#if __VERSION__ >= 130

	particle_radius_squared_fs = particle_radius * particle_radius * radius_scalingfactor * radius_scalingfactor;
	particle_view_pos_fs = vec3(modelview_matrix * vec4(position, 1));

	// Compute color from object ID.
	int objectID = pickingBaseID + int(vertexID) / 14;
	particle_color_fs = vec4(
		float(objectID & 0xFF) / 255.0, 
		float((objectID >> 8) & 0xFF) / 255.0, 
		float((objectID >> 16) & 0xFF) / 255.0, 
		float((objectID >> 24) & 0xFF) / 255.0);
		
	// Transform and project vertex.
	gl_Position = modelviewprojection_matrix * vec4(position + cubeVerts[gl_VertexID % 14] * particle_radius * radius_scalingfactor, 1);
	
#else

	particle_radius_squared_fs = particle_radius * particle_radius * radius_scalingfactor * radius_scalingfactor;
	particle_view_pos_fs = vec3(modelview_matrix * gl_Vertex);

	// Compute color from object ID.
	float objectID = pickingBaseID + floor(vertexID / 14);
	gl_FrontColor = vec4(
		floor(mod(objectID, 256.0)) / 255.0,
		floor(mod(objectID / 256.0, 256.0)) / 255.0, 
		floor(mod(objectID / 65536.0, 256.0)) / 255.0, 
		floor(mod(objectID / 16777216.0, 256.0)) / 255.0);			
		
	// Transform and project vertex.
	int cubeCorner = int(mod(vertexID+0.5, 14.0));
	gl_Position = modelviewprojection_matrix * (gl_Vertex + vec4(cubeVerts[cubeCorner] * particle_radius * radius_scalingfactor, 0));
	
#endif
}
