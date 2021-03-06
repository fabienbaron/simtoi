 /*
 * This file is part of the SImulation and Modeling Tool for Optical
 * Interferometry (SIMTOI).
 *
 * SIMTOI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation version 3.
 *
 * SIMTOI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SIMTOI.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2013 Brian Kloppenborg

 *  This is the base class for disk models which come from a physically
 *  motivated density distribution (unlike those inheriting from CCylinder).
 *
 *  At present, models are rendered as a series of partially opaque concentric
 *  rings oriented along the z-direction. The rings are connected at the
 *  midplane by a triangle fan. This rendering method is suitable for disks
 *  seen edge-on, but probably not for other inclinations.
 *
 *  This base class has four parameters:
 *  	r_in : the disk model's inner radius
 *  	r_cutoff : the radius at which rendering will stop
 *  	h_cutoff : the height (z) at which rendering will stop
 *  	n_rings : the number of rings in the model
 *  The radial and height cutoffs should be defined to be sufficiently large
 *  as to not cut off meaningful density distribution. This requires some
 *  manual tweaking by the user.
 *
 */
#include "CDensityDisk.h"
#include "CShaderFactory.h"
#include "CCylinder.h"

CDensityDisk::CDensityDisk()
: 	CModel()
{
	// give this object a name
	mID = "density_disk";
	mName = "Density disk base class";

	addParameter("T_eff", 5000, 2E3, 1E6, false, 100, "T_eff", "Effective temperature (Kelvin)", 0);
	addParameter("r_in", 0.1, 0.1, 10, false, 0.1, "Inner Radius", "A cutoff radius below which the model will not exist", 2);
	addParameter("r_cutoff", 20, 0.1, 20, false, 1.0, "Radial cutoff", "Cutoff radius beyond which the model will not exist", 2);
	addParameter("h_cutoff", 5, 0.1, 10, false, 1.0, "Height cutoff", "Cutoff height beyond which the model will not exist", 2);
	addParameter("n_rings", 50, 1, 100, false, 1, "N Rings", "An integer number of rings used in the model", 0);

	// We load the default shader, but this should be replaced by something more
	// specific later.
	auto shaders = CShaderFactory::Instance();
	mShader = shaders.CreateShader("default");

	// Resize the texture, 1 element is sufficient.
	mFluxTexture.resize(1);
	mPixelTemperatures.resize(1);
}

CDensityDisk::~CDensityDisk()
{

}



void CDensityDisk::Init()
{
	// Generate the verticies and elements
	vector<vec3> vbo_data;
	vector<unsigned int> elements;
	unsigned int z_divisions = 20;
	unsigned int phi_divisions = 50;
	unsigned int r_divisions = 20;

	mRimStart = 0;
	CCylinder::GenerateRim(vbo_data, elements, z_divisions, phi_divisions);
	mRimSize = elements.size();
	// Calculate the offset in vertex indexes for the GenerateMidplane function
	// to generate correct element indices.
	unsigned int vertex_offset = vbo_data.size();
	mMidplaneStart = mRimSize;
	CCylinder::GenerateMidplane(vbo_data, elements, vertex_offset, r_divisions, phi_divisions);
	mMidplaneSize = elements.size() - mRimSize;

	// Create a new Vertex Array Object, Vertex Buffer Object, and Element Buffer
	// object to store the model's information.
	//
	// First generate the VAO, this stores all buffer information related to this object
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);
	// Generate and bind to the VBO. Upload the verticies.
	glGenBuffers(1, &mVBO); // Generate 1 buffer
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, vbo_data.size() * sizeof(vec3), &vbo_data[0], GL_STATIC_DRAW);
	// Generate and bind to the EBO. Upload the elements.
	glGenBuffers(1, &mEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(unsigned int), &elements[0], GL_STATIC_DRAW);

	CHECK_OPENGL_STATUS_ERROR(glGetError(), "Failed to create buffers");

	// Initialize the shader variables and texture following the default packing
	// scheme.
	InitShaderVariables();
	InitTexture();

	// All done. Un-bind from the VAO, VBO, and EBO to prevent it from being
	// modified by subsequent calls.
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CHECK_OPENGL_STATUS_ERROR(glGetError(), "Failed to bind back to default buffers");

	// Indicate the model is ready to use.
	mModelReady = true;
}

void CDensityDisk::preRender(double & max_flux)
{
	if (!mModelReady)
		Init();

	double temperature = float(mParams["T_eff"].getValue());
	mPixelTemperatures[0] = temperature;
	TemperatureToFlux(mPixelTemperatures, mFluxTexture, mWavelength, max_flux);
}

void CDensityDisk::Render(const glm::mat4 & view, const GLfloat & max_flux)
{
	// Look up the parameters:
	const double r_in = mParams["r_in"].getValue();
	const double r_cutoff  = mParams["r_cutoff"].getValue();
	const double h_cutoff  = mParams["h_cutoff"].getValue();
	int n_rings  = ceil(mParams["n_rings"].getValue());

	NormalizeFlux(max_flux);

	// Activate the shader
	GLuint shader_program = mShader->GetProgram();
	mShader->UseShader();

	// bind back to the VAO
	glBindVertexArray(mVAO);

	// Define the view:
	GLint uniView = glGetUniformLocation(shader_program, "view");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

	GLint uniTranslation = glGetUniformLocation(shader_program, "translation");
	glUniformMatrix4fv(uniTranslation, 1, GL_FALSE, glm::value_ptr(Translate()));

	GLint uniRotation = glGetUniformLocation(shader_program, "rotation");
	glUniformMatrix4fv(uniRotation, 1, GL_FALSE, glm::value_ptr(Rotate()));

	// Set the value for the inner radius.
	// NOTE: In order to accelerate the rendering of the midplane and ensure
	// the inner-most rim is rendered, we have elected to subtract a tiny
	// amount off of the inner radius. This is noted on the wiki.
	GLint uniInnerRadius = glGetUniformLocation(shader_program, "r_in");
	glUniform1f(uniInnerRadius, r_in - 0.01);

	// Look up the scale variable location. We use it below.
	GLint uniScale = glGetUniformLocation(shader_program, "scale");

	// bind to this object's texture
	glBindTexture(GL_TEXTURE_RECTANGLE, mFluxTextureID);
	// upload the image
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, mFluxTexture.size(), 1, 0, GL_RGBA,
			GL_FLOAT, &mFluxTexture[0]);

	// disable detph testing and backface culling, we need to render the whole thing.
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// Render each cylindrical wall:

    // Init scale variables
	double radius = 0;
	double height = h_cutoff;

    glm::mat4 scale;
    glm::mat4 r_scale;
    glm::mat4 h_scale = glm::scale(mat4(1.0f), glm::vec3(1.0, 1.0, height));

    // Render each of the concentric rings
	double dr = (r_cutoff - r_in) / (n_rings - 1);
	for(unsigned int i = 0; i < n_rings; i++)
	{
		// Scale the radius:
		radius = r_in + i * dr;
		r_scale = glm::scale(mat4(1.0f), glm::vec3(radius, radius, 1.0));
		scale = h_scale * r_scale;

		glUniformMatrix4fv(uniScale, 1, GL_FALSE, glm::value_ptr(scale));
		glDrawElements(GL_TRIANGLE_STRIP, mRimSize, GL_UNSIGNED_INT, 0);
	}

	// Render the midplane
	r_scale = glm::scale(mat4(1.0f), glm::vec3(r_cutoff, r_cutoff, 1.0));
	glUniformMatrix4fv(uniScale, 1, GL_FALSE, glm::value_ptr(scale));
	glDrawElements(GL_TRIANGLE_STRIP, mMidplaneSize, GL_UNSIGNED_INT, (void*) (mMidplaneStart * sizeof(float)));

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// bind back to the default texture.
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	CHECK_OPENGL_STATUS_ERROR(glGetError(), "Rendering failed.");
}

/// Overrides the default CModel::SetShader function.
void CDensityDisk::SetShader(CShaderPtr shader)
{
	// This mode does not accept different shaders, do nothing here.
}
