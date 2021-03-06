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
 *  Copyright (c) 2014 Fabien Baron
 */

#include "CShaderFactory.h"
#include "CFeature.h"
#include "CRocheLobe.h"


CRocheLobe::CRocheLobe() :
    AU(1.496e11), rsun(6.955e8), G(6.67428e-11), parsec(3.08567758e16),
    CHealpixSpheroid()
{
    mID = "roche_lobe";
    mName = "Roche Lobe";

    addParameter("T_eff_pole", 5000, 2E3, 1E6, false, 100, "T_pole", "Effective Polar temperature (kelvin)", 0);
    addParameter("von_zeipel_beta", 0.25, 0.0, 1.0, false, 0.1, "Beta", "Von Zeipel gravity darkening parameter (unitless)", 2);
    addParameter("separation", 4.0 , 0.1, 100.0, false, 0.01, "Separation", "Separation between components (mas)", 2);
    addParameter("q", 3.0 , 0.001, 100.0, false, 0.01, "Mass ratio", "M2/M1 mass ratio; M1 = this Roche lobe (unitless)", 2);
    addParameter("P", 1.0 , 0.01, 2.0, false, 0.01, "Async ratio", "Ratio self-rotation period/orbital revolution period (unitless)", 2);
    //    omega_rot = 2.0 * PI / (orbital_period * 3600. * 24.); // in Hz
}

CRocheLobe::~CRocheLobe()
{

}

shared_ptr<CModel> CRocheLobe::Create()
{
    return shared_ptr < CModel > (new CRocheLobe());
}

void CRocheLobe::ComputeRadii(const double r_pole, const double separation, const double q, const double P)
{
    // Compute the radii for the pixels and corners:
    for(unsigned int i = 0; i < pixel_radii.size(); i++)
        pixel_radii[i] = ComputeRadius(r_pole, separation, q, P, pixel_theta[i], pixel_phi[i]);

    for(unsigned int i = 0; i < corner_radii.size(); i++)
        corner_radii[i] = ComputeRadius(r_pole, separation, q, P, corner_theta[i], corner_phi[i]);
}

void CRocheLobe::ComputeGravity(const double r_pole, const double separation, const double q, const double P)
{
    // Compute the gravity vector for each pixel:
    for(unsigned int i = 0; i < gravity.size(); i++)
    {
        ComputeGravity(separation, q, P, pixel_radii[i], pixel_theta[i], pixel_phi[i], g_x[i], g_y[i], g_z[i], gravity[i]);
    }
}

void CRocheLobe::VonZeipelTemperatures(double T_eff_pole, double g_pole, double beta)
{
    for(unsigned int i = 0; i < mPixelTemperatures.size(); i++)
        mPixelTemperatures[i] = T_eff_pole * pow(gravity[i] / g_pole, beta);
}

/// Computes the tangential components and magnitude of gravity at the
/// specified (r, theta, phi) coordinates.
void CRocheLobe::ComputeGravity(const double separation, const double q, const double P, const double radius, const double theta, const double phi, double & g_x, double & g_y, double & g_z, double & g_mag)
{
    double radius1 = radius / separation;  // dimensionless
    double l = cos(phi) * sin(theta);
    double mu = sin(phi) * sin(theta);
    double x = radius1 * l;
    double y = radius1 * mu;
    double z = radius1 * cos(theta);
    double radius2 = std::sqrt( (x - 1.0) * (x - 1.0) + y * y + z * z);
    double radius1_pow3 = radius1 * radius1 * radius1;
    double radius2_pow3 = radius2 * radius2 * radius2;

    // gx, gy, gz are the cartesian coordinates of the gravity vector
    // we need them in cartesian form to compute the vertices for limb-darkening

    g_x = - x / radius1_pow3 + q * (1.0 - x) / radius2_pow3 + P * P * (1 + q) * x - q;
    g_y = - y / radius1_pow3 - q * y / radius2_pow3 + P * P * (1 + q) * y;
    g_z = - z / radius1_pow3 - q * z / radius2_pow3;
    g_mag = std::sqrt(g_x * g_x + g_y * g_y + g_z * g_z);
}


void CRocheLobe::ComputePotential(double & pot, double & dpot, const double radius, const double theta, const double phi, const double separation, const double q, const double P)
{
    // This is only valid for circular, aligned, asynchronous rotation, and will be replaced by Sepinsky 2007
    // theta in radians: co-latitude -- vector
    // phi in radians: phi phi=0 points toward companion -- vector

    const double radius1 = radius / separation;  // dimensionless
    const double l = cos(phi) * sin(theta);
    const double mu = sin(phi) * sin(theta);
    const double x = radius1 * l;
    const double y = radius1 * mu;
    const double z = radius1 * cos(theta);
    //const double xc =  q / (1. + q);
    const double radius2 = std::sqrt( (x - 1.0) * (x - 1.0) + y * y + z * z);

    pot = - 1.0 / radius1 - q / radius2  + q * x - 0.5 * (q + 1.0) * P * P
        * (x * x + y * y );

    dpot =  1.0 / (radius1 * radius1) + q / (radius2 * radius2 *radius2) * (radius1 - l)
        + q * l - (q + 1.0) * P * P * radius1 * ( l * l + mu * mu);

}


double CRocheLobe::ComputeRadius(const double r_pole, const double separation, const double q, const double P, const double theta, const double phi)
{
    // in this function we compute the roche radius based on masses/ distance / orbital_period, for each (theta, phi)
    const double epsilon = 1;
    register int i;
    double pot_surface, pot, dpot;
    double newton_step;

    ComputePotential(pot_surface, dpot, r_pole, 0.0, 0.0, separation, q, P ); // potential at the equator

    double radius = 1.22 * r_pole; // initial guess for the radius, TBD: improve !

    bool converged = false;
    for(int i=0; i<8;i++)//while(converged == FALSE)
    {
        ComputePotential(pot, dpot, radius, theta, phi, separation, q, P );
        newton_step = (pot - pot_surface) / dpot; // newton step
        radius = radius* (1. - newton_step);

        if (fabs(newton_step) < epsilon)
            converged = true;
    }
    return radius;
}


void CRocheLobe::preRender(double & max_flux)
{
    if (!mModelReady)
        Init();

    // See if the user change the tesselation
    const unsigned int n_sides = pow(2, mParams["n_side_power"].getValue());
    if(mParams["n_side_power"].isDirty())
    {
        Init();
    }

    const double r_pole = mParams["r_pole"].getValue();
    const double T_eff_pole = mParams["T_eff_pole"].getValue();
    const double von_zeipel_beta = mParams["von_zeipel_beta"].getValue();
    const double separation = mParams["separation"].getValue();
    const double q = mParams["q"].getValue();
    const double P = mParams["P"].getValue();

    //    if(mParams["r_pole"].isDirty() || mParams["omega_rot"].isDirty())
    ComputeRadii(r_pole, separation, q, P);

    double g_pole, tempx, tempy, tempz;
    ComputeGravity(separation, q, P, r_pole, 0.0, 0.0, tempx, tempy, tempz, g_pole);
    ComputeGravity(r_pole, separation, q, P);

    bool feature_dirty = false;
    for(auto feature: mFeatures)
        feature_dirty |= feature->isDirty();

    if(feature_dirty || mParams["T_eff_pole"].isDirty() || mParams["von_zeipel_beta"].isDirty())
        VonZeipelTemperatures(T_eff_pole, g_pole, von_zeipel_beta);

    for(auto feature: mFeatures)
        feature->apply(this);


    TemperatureToFlux(mPixelTemperatures, mFluxTexture, mWavelength, max_flux);

    GenerateVBO(n_pixels, n_sides, mVBOData);

}

void CRocheLobe::Render(const glm::mat4 & view, const GLfloat & max_flux)
{
    const unsigned int n_sides = pow(2, mParams["n_side_power"].getValue());

    NormalizeFlux(max_flux);

    mat4 scale = glm::scale(mat4(1.0f), glm::vec3(1, 1, 1));

    // Activate the shader
    GLuint shader_program = mShader->GetProgram();
    mShader->UseShader();

    // bind back to the VAO
    glBindVertexArray(mVAO);

    // Define the view:
    GLint uniView = glGetUniformLocation(shader_program, "view");
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

    GLint uniTranslation = glGetUniformLocation(shader_program, "translation");
    glUniformMatrix4fv(uniTranslation, 1, GL_FALSE,
            glm::value_ptr(Translate()));

    GLint uniRotation = glGetUniformLocation(shader_program, "rotation");
    glUniformMatrix4fv(uniRotation, 1, GL_FALSE, glm::value_ptr(Rotate()));

    GLint uniScale = glGetUniformLocation(shader_program, "scale");
    glUniformMatrix4fv(uniScale, 1, GL_FALSE, glm::value_ptr(scale));

    // Bind to the texture, upload it.
    glBindTexture(GL_TEXTURE_RECTANGLE, mFluxTextureID);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, 12 * n_sides, n_sides, 0, GL_RGBA,
            GL_FLOAT, &mFluxTexture[0]);

    // Upload the VBO data:
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, mVBOData.size() * sizeof(vec3), &mVBOData[0],
            GL_DYNAMIC_DRAW);

    // render
    glDrawElements(GL_TRIANGLES, mElements.size(), GL_UNSIGNED_INT, 0);

    glBindTexture(GL_TEXTURE_RECTANGLE, 0);

    // unbind from the Vertex Array Object, Vertex Buffer Object, and Element buffer object.
    glBindVertexArray(0);

    CHECK_OPENGL_STATUS_ERROR(glGetError(), "Rendering failed");
}

/// Computes the geometry of the spherical Healpix surface
void CRocheLobe::GenerateModel(vector<vec3> & vbo_data,
        vector<unsigned int> & elements)
{
    const double r_pole = mParams["r_pole"].getValue();
    const double T_eff_pole = mParams["T_eff_pole"].getValue();
    const double von_zeipel_beta = mParams["von_zeipel_beta"].getValue();
    const unsigned int n_sides = pow(2, mParams["n_side_power"].getValue());
    const double separation = mParams["separation"].getValue();
    const double q = mParams["q"].getValue();
    const double P = mParams["P"].getValue();

    // Generate a unit Healpix sphere
    GenerateHealpixSphere(n_pixels, n_sides);

    // recomputing the sphereoid is very expensive. Make use of the dirty flags
    // to only compute that which is absolutely necessary.
    ComputeRadii(r_pole, separation, q, P);
    double g_pole, tempx, tempy, tempz;
    ComputeGravity(separation, q, P, r_pole, 0.0, 0.0, tempx, tempy, tempz, g_pole);
    ComputeGravity(r_pole, separation, q, P);
    VonZeipelTemperatures(T_eff_pole, g_pole, von_zeipel_beta);

    for(auto feature: mFeatures)
        feature->apply(this);

    // Find the maximum temperature
    double max_temperature = 0;
    for(unsigned int i = 0; i < mPixelTemperatures.size(); i++)
    {
        if(mPixelTemperatures[i] > max_temperature)
            max_temperature = mPixelTemperatures[i];
    }

    // Convert temperatures to fluxes.
    TemperatureToFlux(mPixelTemperatures, mFluxTexture, mWavelength, max_temperature);

    GenerateVBO(n_pixels, n_sides, vbo_data);

    // Create the lookup indicies.
    GenerateHealpixVBOIndicies(n_pixels, elements);

}
