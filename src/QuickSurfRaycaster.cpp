/*
 *	QuickSurfRaycaster.cpp
 *
 *	Copyright (C) 2016 by Universitaet Stuttgart (VISUS).
 *	All rights reserved
 */

#include "stdafx.h"

//#define _USE_MATH_DEFINES 1

#include "QuickSurfRaycaster.h"
#include "mmcore/CoreInstance.h"
#include <gl/GLU.h>

#include "mmcore/param/IntParam.h"
#include "mmcore/param/FloatParam.h"
#include "mmcore/param/StringParam.h"

#include "vislib/StringTokeniser.h"

#include <channel_descriptor.h>
#include <driver_functions.h>

using namespace megamol;
using namespace megamol::core;
using namespace megamol::core::moldyn;
using namespace megamol::protein_cuda;
using namespace megamol::protein_calls;

/*
 *	QuickSurfRaycaster::QuickSurfRaycaster
 */
QuickSurfRaycaster::QuickSurfRaycaster(void) : Renderer3DModule(),
	particleDataSlot("getData", "Connects the surface renderer with the particle data storage"),
	qualityParam("quicksurf::quality", "Quality"),
	radscaleParam("quicksurf::radscale", "Radius scale"),
	gridspacingParam("quicksurf::gridspacing", "Grid spacing"),
	isovalParam("quicksurf::isoval", "Isovalue"),
	selectedIsovals("quicksurf::selectedIsovals", "Semicolon seperated list of normalized isovalues we want to ray cast the isoplanes from"),
	scalingFactor("quicksurf::scalingFactor", "Scaling factor for the density values and particle radii"),
	concFactorParam("quicksurf::concentrationFactor", "Scaling factor for particle radii based on their concentration"),
	setCUDAGLDevice(true),
	firstTransfer(true),
	particlesSize(0)
{
	this->particleDataSlot.SetCompatibleCall<MultiParticleDataCallDescription>();
	this->particleDataSlot.SetCompatibleCall<MolecularDataCallDescription>();
	this->MakeSlotAvailable(&this->particleDataSlot);

	this->qualityParam.SetParameter(new param::IntParam(1, 0, 4));
	this->MakeSlotAvailable(&this->qualityParam);

	this->radscaleParam.SetParameter(new param::FloatParam(1.0f, 0.0f));
	this->MakeSlotAvailable(&this->radscaleParam);

	//this->gridspacingParam.SetParameter(new param::FloatParam(0.01953125f, 0.0f));
	this->gridspacingParam.SetParameter(new param::FloatParam(0.2f, 0.0f));
	this->MakeSlotAvailable(&this->gridspacingParam);

	this->isovalParam.SetParameter(new param::FloatParam(1.0f, 0.0f));
	this->MakeSlotAvailable(&this->isovalParam);

	this->selectedIsovals.SetParameter(new param::StringParam("0.1,0.9"));
	this->MakeSlotAvailable(&this->selectedIsovals);
	this->selectedIsovals.ForceSetDirty(); // necessary for initial update
	isoVals.push_back(0.1f);
	isoVals.push_back(0.9f);

	this->scalingFactor.SetParameter(new param::FloatParam(1.0f, 0.0f));
	this->MakeSlotAvailable(&this->scalingFactor);

	this->concFactorParam.SetParameter(new param::FloatParam(0.5f, 0.0f));
	this->MakeSlotAvailable(&this->concFactorParam);

	lastViewport.Set(0, 0);

	volumeExtent = make_cudaExtent(0, 0, 0);

	cudaqsurf = nullptr;
	cudaImage = nullptr;
	volumeArray = nullptr;
	particles = nullptr;
	texHandle = 0;

	curTime = 0;
}

/*
 *	QuickSurfRaycaster::~QuickSurfRaycaster
 */
QuickSurfRaycaster::~QuickSurfRaycaster(void) {
	if (cudaqsurf) {
		CUDAQuickSurfAlternative *cqs = (CUDAQuickSurfAlternative *)cudaqsurf;
		delete cqs;
		cqs = nullptr;
		cudaqsurf = nullptr;
	}
	
	this->Release();
}

/*
 *	QuickSurfRaycaster::release
 */
void QuickSurfRaycaster::release(void) {

}

bool QuickSurfRaycaster::calcVolume(float3 bbMin, float3 bbMax, float* positions, int quality, float radscale, float gridspacing,
	float isoval, float minConcentration, float maxConcentration, bool useCol, int timestep) {

	float x = bbMax.x - bbMin.x;
	float y = bbMax.y - bbMin.y;
	float z = bbMax.z - bbMin.z;

	int numVoxels[3];
	numVoxels[0] = (int)ceil(x / gridspacing);
	numVoxels[1] = (int)ceil(y / gridspacing);
	numVoxels[2] = (int)ceil(z / gridspacing);

	x = (numVoxels[0] - 1) * gridspacing;
	y = (numVoxels[1] - 1) * gridspacing;
	z = (numVoxels[2] - 1) * gridspacing;

	printf("vox %i %i %i \n", numVoxels[0], numVoxels[1], numVoxels[2]);

	volumeExtent = make_cudaExtent(numVoxels[0], numVoxels[1], numVoxels[2]);
	volumeExtentSmall = make_cudaExtent(numVoxels[0], numVoxels[1], numVoxels[2]);

	float gausslim = 2.0f;
	switch (quality) { // TODO adjust values
	case 3: gausslim = 4.0f; break;
	case 2: gausslim = 3.0f; break;
	case 1: gausslim = 2.5f; break;
	case 0:
	default: gausslim = 2.0f; break;
	}

	float origin[3] = { bbMin.x, bbMin.y, bbMin.z };

	if (cudaqsurf == NULL) {
		cudaqsurf = new CUDAQuickSurfAlternative();
	}

	CUDAQuickSurfAlternative *cqs = (CUDAQuickSurfAlternative*)cudaqsurf;

	int result = -1;
	result = cqs->calc_map((long)particleCnt, positions, colorTable.data(), 1, 
		origin, numVoxels, maxConcentration, radscale, gridspacing, 
		isoval, gausslim, false, timestep, 20);

	checkCudaErrors(cudaDeviceSynchronize());

	volumeExtent = make_cudaExtent(cqs->getMapSizeX(), cqs->getMapSizeY(), cqs->getMapSizeZ());

	// make the initial plane more beautiful
	if (volumeExtent.depth > volumeExtentSmall.depth)
		volumeExtentSmall.depth = volumeExtentSmall.depth + 1;

	return (result == 0);
}

/*
 *	QuickSurfRaycaster::create
 */
bool QuickSurfRaycaster::create(void) {
	return initOpenGL();
}

/*
 *	QuickSurfRaycaster::GetCapabilities
 */
bool QuickSurfRaycaster::GetCapabilities(Call& call) {
	view::AbstractCallRender3D *cr3d = dynamic_cast<view::AbstractCallRender3D *>(&call);
	if (cr3d == NULL) return false;

	cr3d->SetCapabilities(view::AbstractCallRender3D::CAP_RENDER
		| view::AbstractCallRender3D::CAP_LIGHTING
		| view::AbstractCallRender3D::CAP_ANIMATION);

	return true;
}

/*
 *	QuickSurfRaycaster::GetExtents
 */
bool QuickSurfRaycaster::GetExtents(Call& call) {
	view::AbstractCallRender3D *cr3d = dynamic_cast<view::AbstractCallRender3D *>(&call);
	if (cr3d == NULL) return false;

	MultiParticleDataCall *mpdc = this->particleDataSlot.CallAs<MultiParticleDataCall>();
	MolecularDataCall *mdc = this->particleDataSlot.CallAs<MolecularDataCall>();

	if (mpdc == NULL && mdc == NULL) return false;

	// MultiParticleDataCall in use
	if (mpdc != NULL) {
		if (!(*mpdc)(1)) return false;

		float scale;
		if (!vislib::math::IsEqual(mpdc->AccessBoundingBoxes().ObjectSpaceBBox().LongestEdge(), 0.0f)) {
			scale = 2.0f / mpdc->AccessBoundingBoxes().ObjectSpaceBBox().LongestEdge();
		}
		else {
			scale = 1.0f;
		}
		cr3d->AccessBoundingBoxes() = mpdc->AccessBoundingBoxes();
		cr3d->AccessBoundingBoxes().MakeScaledWorld(scale);
		cr3d->SetTimeFramesCount(mpdc->FrameCount());
	} // MolecularDataCall in use
	else if (mdc != NULL) {
		if (!(*mdc)(1)) return false;

		float scale;
		if (!vislib::math::IsEqual(mdc->AccessBoundingBoxes().ObjectSpaceBBox().LongestEdge(), 0.0f)) {
			scale = 2.0f / mdc->AccessBoundingBoxes().ObjectSpaceBBox().LongestEdge();
		}
		else {
			scale = 1.0f;
		}
		cr3d->AccessBoundingBoxes() = mdc->AccessBoundingBoxes();
		cr3d->AccessBoundingBoxes().MakeScaledWorld(scale);
		cr3d->SetTimeFramesCount(mdc->FrameCount());
	}

	return true;
}

/*
 *	QuickSurfRaycaster::initCuda
 */
bool QuickSurfRaycaster::initCuda(view::CallRender3D& cr3d) {
	
	// set cuda device
	if (setCUDAGLDevice) {
#ifdef _WIN32
		if (cr3d.IsGpuAffinity()) {
			HGPUNV gpuId = cr3d.GpuAffinity<HGPUNV>();
			int devId;
			cudaWGLGetDevice(&devId, gpuId);
			cudaGLSetGLDevice(devId);
		}
		else {
			cudaGLSetGLDevice(cudaUtilGetMaxGflopsDeviceId());
		}
#else
		cudaGLSetGLDevice(cudaUtilGetMaxGflopsDeviceId());
#endif
		cudaError err = cudaGetLastError();
		if (err != 0) {
			printf("cudaGLSetGLDevice: %s\n", cudaGetErrorString(err));
			return false;
		}
		setCUDAGLDevice = false;
	}

	return true;
}

/*
 *	QuickSurfRaycaster::initPixelBuffer
 */
bool QuickSurfRaycaster::initPixelBuffer(view::CallRender3D& cr3d) {

	auto viewport = cr3d.GetViewport().GetSize();

	if (lastViewport == viewport) {
		return true;
	} else {
		lastViewport = viewport;
	}

	if (!texHandle) {
		glGenTextures(1, &texHandle);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, texHandle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, viewport.GetWidth(), viewport.GetHeight(), 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (cudaImage) {
		checkCudaErrors(cudaFreeHost(cudaImage));
		cudaImage = NULL;
	}

	checkCudaErrors(cudaMallocHost((void**)&cudaImage, viewport.GetWidth() * viewport.GetHeight() * sizeof(unsigned int)));

	return true;
}

/*
 *	QuickSurfRaycaster::initOpenGL
 */
bool QuickSurfRaycaster::initOpenGL() {

	Vertex v0(-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	Vertex v1(-1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	Vertex v2(1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
	Vertex v3(1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);

	std::vector<Vertex> verts = { v0, v2, v1, v3 };

	glGenVertexArrays(1, &textureVAO);
	glGenBuffers(1, &textureVBO);

	glBindVertexArray(textureVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textureVBO);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)* verts.size(), verts.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, r));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	using namespace vislib::sys;
	using namespace vislib::graphics::gl;

	ShaderSource vertSrc;
	ShaderSource fragSrc;

	if (!this->GetCoreInstance()->ShaderSourceFactory().MakeShaderSource("quicksurfraycast::texture::textureVertex", vertSrc)) {
		Log::DefaultLog.WriteMsg(Log::LEVEL_ERROR, "Unable to load vertex shader source for texture shader");
		return false;
	}

	if (!this->GetCoreInstance()->ShaderSourceFactory().MakeShaderSource("quicksurfraycast::texture::textureFragment", fragSrc)) {
		Log::DefaultLog.WriteMsg(Log::LEVEL_ERROR, "Unable to load fragment shader source for texture shader");
		return false;
	}

	this->textureShader.Compile(vertSrc.Code(), vertSrc.Count(), fragSrc.Code(), fragSrc.Count());
	this->textureShader.Link();

	return true;
}

/*
 *	QuickSurfRaycaster::Render
 */
bool QuickSurfRaycaster::Render(Call& call) {
	view::CallRender3D *cr3d = dynamic_cast<view::CallRender3D *>(&call);
	if (cr3d == NULL) return false;

	if (isoVals.size() < 1) return true;

	this->cameraInfo = cr3d->GetCameraParameters();

	callTime = cr3d->Time();

	//int myTime = curTime; // only for writing of velocity data
	int myTime = static_cast<int>(callTime);

	MultiParticleDataCall * mpdc = particleDataSlot.CallAs<MultiParticleDataCall>();
	MolecularDataCall * mdc = particleDataSlot.CallAs<MolecularDataCall>();

	float3 bbMin;
	float3 bbMax;

	float3 clipBoxMin;
	float3 clipBoxMax;

	float concMin = FLT_MAX;
	float concMax = FLT_MIN;

	if (mpdc == NULL && mdc == NULL) return false;

	if (mpdc != NULL) {
		mpdc->SetFrameID(myTime);
		if (!(*mpdc)(1)) return false;
		if (!(*mpdc)(0)) return false;

		numParticles = 0;
		for (unsigned int i = 0; i < mpdc->GetParticleListCount(); i++) {
			numParticles += mpdc->AccessParticles(i).GetCount();
		}

		if (numParticles == 0) {
			return true;
		}

		if (this->particlesSize < this->numParticles * 4) {
			if (this->particles) {
				delete[] this->particles;
				this->particles = nullptr;
			}
			this->particles = new float[this->numParticles * 4];
			this->particlesSize = this->numParticles * 4;
		}
		memset(this->particles, 0, this->numParticles * 4 * sizeof(float));

		particleCnt = 0;
		this->colorTable.clear();
		this->colorTable.resize(numParticles * 4, 0.0f);

		auto bb = mpdc->GetBoundingBoxes().ObjectSpaceBBox();
		bbMin = make_float3(bb.Left(), bb.Bottom(), bb.Back());
		bbMax = make_float3(bb.Right(), bb.Top(), bb.Front());
		bb = mpdc->GetBoundingBoxes().ClipBox();
		clipBoxMin = make_float3(bb.Left(), bb.Bottom(), bb.Back());
		clipBoxMax = make_float3(bb.Right(), bb.Top(), bb.Front());

		//printf("bbMin %f %f %f\n", bbMin.x, bbMin.y, bbMin.z);
		//exit(-1);

//#define FILTER
#ifdef FILTER // filtering: calculate min and max beforehand
		for (unsigned int i = 0; i < mpdc->GetParticleListCount(); i++) {
			MultiParticleDataCall::Particles &parts = mpdc->AccessParticles(i);
			const float *colorPos = static_cast<const float*>(parts.GetColourData());
			unsigned int colStride = parts.GetColourDataStride();
			int numColors = 0;

			switch (parts.GetColourDataType()) {
			case MultiParticleDataCall::Particles::COLDATA_FLOAT_I: numColors = 1; break;
			case MultiParticleDataCall::Particles::COLDATA_FLOAT_RGB: numColors = 3; break;
			case MultiParticleDataCall::Particles::COLDATA_FLOAT_RGBA: numColors = 4; break;
			case MultiParticleDataCall::Particles::COLDATA_UINT8_RGB: numColors = 0; break; // TODO
			case MultiParticleDataCall::Particles::COLDATA_UINT8_RGBA: numColors = 0; break; // TODO
			}

			// if the vertices have no type, take the next list
			if (parts.GetVertexDataType() == megamol::core::moldyn::MultiParticleDataCall::Particles::VERTDATA_NONE) {
				continue;
			}

			if (numColors > 0) {

				for (UINT64 j = 0; j < parts.GetCount(); j++, colorPos = reinterpret_cast<const float*>(reinterpret_cast<const char*>(colorPos)+colStride)) {

					if (colorPos[numColors - 1] < concMin) concMin = colorPos[numColors - 1];
					if (colorPos[numColors - 1] > concMax) concMax = colorPos[numColors - 1];
				}
			}
		}
#endif

		particleCnt = 0;

		for (unsigned int i = 0; i < mpdc->GetParticleListCount(); i++) {
			MultiParticleDataCall::Particles &parts = mpdc->AccessParticles(i);
			const float *pos = static_cast<const float*>(parts.GetVertexData());
			const float *colorPos = static_cast<const float*>(parts.GetColourData());
			unsigned int posStride = parts.GetVertexDataStride();
			unsigned int colStride = parts.GetColourDataStride();
			float globalRadius = parts.GetGlobalRadius();
			bool useGlobRad = (parts.GetVertexDataType() == megamol::core::moldyn::MultiParticleDataCall::Particles::VERTDATA_FLOAT_XYZ);
			int numColors = 0;

			switch (parts.GetColourDataType()) {
			case MultiParticleDataCall::Particles::COLDATA_FLOAT_I: numColors = 1; break;
			case MultiParticleDataCall::Particles::COLDATA_FLOAT_RGB: numColors = 3; break;
			case MultiParticleDataCall::Particles::COLDATA_FLOAT_RGBA: numColors = 4; break;
			case MultiParticleDataCall::Particles::COLDATA_UINT8_RGB: numColors = 0; break; // TODO
			case MultiParticleDataCall::Particles::COLDATA_UINT8_RGBA: numColors = 0; break; // TODO
			}

			// if the vertices have no type, take the next list
			if (parts.GetVertexDataType() == megamol::core::moldyn::MultiParticleDataCall::Particles::VERTDATA_NONE) { 
				continue;
			}
			if (useGlobRad) { // TODO is this correct?
				if (posStride < 12) posStride = 12;
			}
			else {
				if (posStride < 16) posStride = 16;
			}

			for (UINT64 j = 0; j < parts.GetCount(); j++, pos = reinterpret_cast<const float*>(reinterpret_cast<const char*>(pos) + posStride),
				colorPos = reinterpret_cast<const float*>(reinterpret_cast<const char*>(colorPos) + colStride)) {

#ifdef FILTER
				if (colorPos[numColors - 1] > (concMax - concMin) * isoVals[0] + concMin
					&& colorPos[numColors - 1] < (concMax - concMin) * isoVals[isoVals.size() - 1] + concMin) {
#endif FILTER
					particles[particleCnt * 4 + 0] = pos[0] - bbMin.x;
					particles[particleCnt * 4 + 1] = pos[1] - bbMin.y;
					particles[particleCnt * 4 + 2] = pos[2] - bbMin.z;
					if (useGlobRad) {
						particles[particleCnt * 4 + 3] = globalRadius;
					}
					else {
						particles[particleCnt * 4 + 3] = pos[3];
					}

#ifndef FILTER // calculate the min and max here if no filtering performed
					if (colorPos[numColors - 1] < concMin) concMin = colorPos[numColors - 1];
					if (colorPos[numColors - 1] > concMax) concMax = colorPos[numColors - 1];
#endif

					/*---------------------------------choose-one---------------------------------------------------------*/
#define ALWAYS4COLORS
#ifndef ALWAYS4COLORS
					// 1. copy all available values into the color, the rest gets filled up with the last available value
					for (int k = 0; k < numColors; k++) {
						for (int l = 0; l < 3 - k; l++) {
							this->colorTable[particleCnt * 4 + k + l] = colorPos[k];
						}
					}
#else
					for(int k = 0; k < 4; k++) {
						this->colorTable[particleCnt * 4 + k] = colorPos[k];
					}
#endif

#ifdef FILTER
					// normalize concentration, multiply it with a factor and write it
					// TODO do weird things with the concentration so it results in a nice iso-surface
					this->colorTable[particleCnt * 4 + 3] = ((colorPos[numColors - 1] - concMin) / (concMax - concMin)) * concFactorParam.Param<param::FloatParam>()->Value();
#else
					// normalization of the values happens later
					this->colorTable[particleCnt * 4 + 3] = colorPos[numColors - 1];
#endif


					// 2. fill r,g & b with the last available color value (should be density)
					/*for (int k = 0; k < 3; k++) {
						this->colorTable[particleCnt * 4 + k] = colorPos[numColors - 1];
					}*/

					/*---------------------------------------------------------------------------------------------------*/

					particleCnt++;
#ifdef FILTER
				}
#endif FILTER
			}
		}

#ifndef FILTER // no filtering: we need to normalize the concentration values
		for (int i = 0; i < particleCnt; i++) {
			this->colorTable[i * 4 + 3] = ((this->colorTable[i * 4 + 3] - concMin) / (concMax - concMin)) * concFactorParam.Param<param::FloatParam>()->Value();
		}
#endif

		if (particleCnt == 0) {
			return true;
		}

		//this->particlesSize = this->particleCnt * 4; // adapt size of the particle list
		//this->colorTable.resize(particleCnt * 4); // shrink color vector

		//printf("conc: %f %f\n", concMin, concMax);
		//printf("part: %llu\n", particleCnt);
		//printf("col: %u\n", colorTable.size());

		glPushMatrix();
		float scale = 1.0f;
		if (!vislib::math::IsEqual(mpdc->AccessBoundingBoxes().ObjectSpaceBBox().LongestEdge(), 0.0f)) {
			scale = 2.0f / mpdc->AccessBoundingBoxes().ObjectSpaceBBox().LongestEdge();
		}
		glScalef(scale, scale, scale);

		mpdc->Unlock();

	} else if (mdc != NULL) {
		// TODO
		printf("MolecularDataCall currently not supported\n");
		mdc->Unlock();
		return false;
	}

	initCuda(*cr3d);
	initPixelBuffer(*cr3d);

	float factor = scalingFactor.Param<param::FloatParam>()->Value();

	auto viewport = cr3d->GetViewport().GetSize();

    GLfloat m[16];
	GLfloat m_proj[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
	glGetFloatv(GL_PROJECTION_MATRIX, m_proj);
    Mat4f modelMatrix(&m[0]);
	Mat4f projectionMatrix(&m_proj[0]);
    modelMatrix.Invert();
	projectionMatrix.Invert();
	float3 camPos = make_float3(modelMatrix.GetAt(0, 3), modelMatrix.GetAt(1, 3), modelMatrix.GetAt(2, 3));
	float3 camDir = norm(make_float3(-modelMatrix.GetAt(0, 2), -modelMatrix.GetAt(1, 2), -modelMatrix.GetAt(2, 2)));
	float3 camUp = norm(make_float3(modelMatrix.GetAt(0, 1), modelMatrix.GetAt(1, 1), modelMatrix.GetAt(2, 1)));
	float3 camRight = norm(make_float3(modelMatrix.GetAt(0, 0), modelMatrix.GetAt(1, 0), modelMatrix.GetAt(2, 0)));
	// the direction has to be negated because of the right-handed view space of OpenGL

	auto cam = cr3d->GetCameraParameters();

	float fovy = (float)(cam->ApertureAngle() * M_PI / 180.0f);

	float aspect = (float)viewport.GetWidth() / (float)viewport.GetHeight();
	if (viewport.GetHeight() == 0)
		aspect = 0.0f;

	float fovx = 2.0f * atan(tan(fovy / 2.0f) * aspect);
	float zNear = (2.0f * projectionMatrix.GetAt(2, 3)) / (2.0f * projectionMatrix.GetAt(2, 2) - 2.0f);

	float density = 0.5f;
	float brightness = 1.0f;
	float transferOffset = 0.0f;
	float transferScale = 1.0f;

	/*printf("min: %f %f %f \n", clipBoxMin.x, clipBoxMin.y, clipBoxMin.z);
	printf("max: %f %f %f \n\n", clipBoxMax.x, clipBoxMax.y, clipBoxMax.z);*/

	dim3 blockSize = dim3(8, 8);
	dim3 gridSize = dim3(iDivUp(viewport.GetWidth(), blockSize.x), iDivUp(viewport.GetHeight(), blockSize.y));

	if (cudaqsurf == NULL) {
		cudaqsurf = new CUDAQuickSurfAlternative();
	}

#ifdef FILTER
	bool suc = this->calcVolume(bbMin, bbMax, particles, 
					this->qualityParam.Param<param::IntParam>()->Value(),
					this->radscaleParam.Param<param::FloatParam>()->Value(),
					this->gridspacingParam.Param<param::FloatParam>()->Value(),
					1.0f, // necessary to switch off velocity scaling
					(concMax - concMin) * isoVals[0] + concMin, (concMax - concMin) * isoVals[isoVals.size() - 1] + concMin, true,
					myTime);
#else
	bool suc = this->calcVolume(bbMin, bbMax, particles,
		this->qualityParam.Param<param::IntParam>()->Value(),
		this->radscaleParam.Param<param::FloatParam>()->Value(),
		this->gridspacingParam.Param<param::FloatParam>()->Value(),
		1.0f, // necessary to switch off velocity scaling
		0.0f, this->concFactorParam.Param<param::FloatParam>()->Value(), true,
		myTime);
		// the concentrations are scaled to [0, concFactor]
#endif
	// TODO change maxRad?

	//if (!suc) return false;

	CUDAQuickSurfAlternative * cqs = (CUDAQuickSurfAlternative*)cudaqsurf;
	float * map = cqs->getMap();

	// read lighting parameters
	float lightPos[4];
	glGetLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	float3 light = make_float3(lightPos[0], lightPos[1], lightPos[2]);

	// TODO read slot for lighting params

	//transferNewVolume(map, volumeExtent);
	transferNewVolume(map, volumeExtentSmall);
	checkCudaErrors(cudaDeviceSynchronize());

	//if (!suc)
		render_kernel(gridSize, blockSize, cudaImage, viewport.GetWidth(), viewport.GetHeight(), fovx, fovy, camPos, camDir, camUp, camRight, zNear, density, brightness, transferOffset, transferScale, bbMin, bbMax, volumeExtent, light, make_float4(0.3f, 0.5f, 0.4f, 10.0f));
	//else
		//renderArray_kernel(cqs->getMap(), gridSize, blockSize, cudaImage, viewport.GetWidth(), viewport.GetHeight(), fovx, fovy, camPos, camDir, camUp, camRight, zNear, density, brightness, transferOffset, transferScale, bbMin, bbMax, dim3(cqs->getMapSizeX(), cqs->getMapSizeY(), cqs->getMapSizeZ()));
	
	getLastCudaError("kernel failed");
	checkCudaErrors(cudaDeviceSynchronize());

	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, viewport.GetWidth(), viewport.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, cudaImage);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);

	textureShader.Enable();
	
	glBindVertexArray(textureVAO);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	textureShader.Disable();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// parse selected isovalues if needed
	if (selectedIsovals.IsDirty() || firstTransfer) {
		isoVals.clear();
		vislib::TString valString = selectedIsovals.Param<param::StringParam>()->Value();
		vislib::StringA vala = T2A(valString);

		vislib::StringTokeniserA sta(vala, ',');
		while (sta.HasNext()) {
			vislib::StringA t = sta.Next();
			if (t.IsEmpty()) {
				continue;
			}
			isoVals.push_back((float)vislib::CharTraitsA::ParseDouble(t));
		}

		/*for (float f: isoVals)
		printf("value: %f\n", f);*/

		// sort the isovalues ascending
		std::sort(isoVals.begin(), isoVals.end());

		std::vector<float> adaptedIsoVals = isoVals;
		float div = isoVals[std::min((int)isoVals.size(), 4) - 1] - isoVals[0];

		float bla = 0.5f;

		// adapt the isovalues to the filtered values
		for (int i = 0; i < std::min((int)isoVals.size(), 4); i++) {
			adaptedIsoVals[i] = (adaptedIsoVals[i] - isoVals[0]) / div;
		}

		// copy the first four isovalues into a float4
		std::vector<float> help(4, -1.0f);
		for (int i = 0; i < std::min((int)adaptedIsoVals.size(), 4); i++) {
			help[i] = isoVals[i];
		}
		float4 values = make_float4(help[0], help[1], help[2], help[3]);
		printf("isos: %f %f %f %f\n", help[0], help[1], help[2], help[3]);
		transferIsoValues(values, (int)std::min((int)isoVals.size(), 4));

		selectedIsovals.ResetDirty();
		if (firstTransfer) firstTransfer = false;
	}

	curTime++;

	return true;
}