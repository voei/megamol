/**
* CinematicView.cpp
*/

#include "stdafx.h"
#include "CinematicView.h"
#include "CallCinematicCamera.h"
#include "CameraParamsOverride.h"
#include "mmcore/param/BoolParam.h"
#include "mmcore/param/EnumParam.h"
#include "mmcore/view/CallRender3D.h"

using namespace megamol;
using namespace megamol::core;
using namespace cinematiccamera;

CinematicView::CinematicView(void) : View3D(),
		keyframeKeeperSlot("keyframeKeeper", "Connects to the Keyframe Keeper."),
		selectedKeyframeParam("cinematicCam::selectedKeyframeParam", "render selected Keyframe if true. render keyframe at animationtime if false"),
		selectedSkyboxSideParam("cinematicCam::skyboxSide", "Skybox side rendering"),
		autoLoadKeyframes("keyframeAutoLoad","shall the keyframes be loaded automatically from the file provided in the keyframe-keeper?"),
		autoSetTotalTime("totalTimeAutoSet", "shall the total animation time be determined automatically?"),
		firstframe(true) {


	this->keyframeKeeperSlot.SetCompatibleCall<CallCinematicCameraDescription>();
	this->MakeSlotAvailable(&this->keyframeKeeperSlot);

	this->selectedKeyframeParam << new core::param::BoolParam(true);
	this->MakeSlotAvailable(&this->selectedKeyframeParam);

	this->autoLoadKeyframes.SetParameter(new core::param::BoolParam(false));
	this->MakeSlotAvailable(&this->autoLoadKeyframes);

	this->autoSetTotalTime.SetParameter(new core::param::BoolParam(true));
	this->MakeSlotAvailable(&this->autoSetTotalTime);

	param::EnumParam *sbs = new param::EnumParam(SKYBOX_NONE);
	sbs->SetTypePair(SKYBOX_NONE,   "None");
	sbs->SetTypePair(SKYBOX_FRONT,	"Front");
	sbs->SetTypePair(SKYBOX_BACK,	"Back");
	sbs->SetTypePair(SKYBOX_LEFT,	"Left");
	sbs->SetTypePair(SKYBOX_RIGHT,	"Right");
	sbs->SetTypePair(SKYBOX_UP,		"Up");
	sbs->SetTypePair(SKYBOX_DOWN,	"Down");
	this->selectedSkyboxSideParam << sbs;
	this->MakeSlotAvailable(&this->selectedSkyboxSideParam);
}


megamol::cinematiccamera::CinematicView::~CinematicView() {}

void megamol::cinematiccamera::CinematicView::Render(const mmcRenderViewContext& context) {
	auto cr3d = this->rendererSlot.CallAs<core::view::CallRender3D>();
	(*cr3d)(1); // get extents

	auto kfc = this->keyframeKeeperSlot.CallAs<CallCinematicCamera>();
	if (!kfc) {
		vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_ERROR, "KeyframeKeeper slot not connected!");
	}

	

	if (firstframe) {
		if (autoLoadKeyframes.Param<param::BoolParam>()->Value()) {
			(*kfc)(CallCinematicCamera::CallForLoadKeyframe);
		}
		if (autoSetTotalTime.Param<param::BoolParam>()->Value()) {
			kfc->setTotalTime(static_cast<float>(cr3d->TimeFramesCount()));
			(*kfc)(CallCinematicCamera::CallForSetTotalTime);
		}
		firstframe = false;
	}

	if (selectedKeyframeParam.Param<core::param::BoolParam>()->Value()){

		if ((*kfc)(CallCinematicCamera::CallForGetSelectedKeyframe)){

			if (kfc->getSelectedKeyframe().getID() != -2){

				if (kfc->getSelectedKeyframe().getID() == -1){
					kfc->getInterpolatedKeyframe().putCamParameters(this->cam.Parameters());
				}
				else{
					kfc->getSelectedKeyframe().putCamParameters(this->cam.Parameters());
				}
			}
		}
		Base::Render(context);
	}
	else {

		kfc->setTimeofKeyframeToGet(static_cast<float>(context.Time));
		if ((*kfc)(CallCinematicCamera::CallForGetKeyframeAtTime)){
			kfc->getInterpolatedKeyframe().putCamParameters(this->cam.Parameters());

		}

		vislib::SmartPtr<vislib::graphics::CameraParameters> cp = this->cam.Parameters();
		vislib::math::Point<float, 3> camPos = cp->Position();
		vislib::math::Vector<float, 3> camRight = cp->Right();
		vislib::math::Vector<float, 3> camUp = cp->Up();
		vislib::math::Vector<float, 3> camFront = cp->Front();
		float tmpDist = cp->FocalDistance();
		// adjust cam to selected skybox side
		SkyboxSides side = static_cast<SkyboxSides>(this->selectedSkyboxSideParam.Param<param::EnumParam>()->Value());
		if (side != SKYBOX_NONE) {
			// set aperture angle to 90 deg
			cp->SetApertureAngle(90.0f);
			if (side == SKYBOX_BACK) {
				cp->SetView(camPos, camPos - camFront * tmpDist, camUp);
			}
			else if (side == SKYBOX_RIGHT) {
				cp->SetView(camPos, camPos + camRight * tmpDist, camUp);
			}
			else if (side == SKYBOX_LEFT) {
				cp->SetView(camPos, camPos - camRight * tmpDist, camUp);
			}
			else if (side == SKYBOX_UP) {
				cp->SetView(camPos, camPos + camUp * tmpDist, -camFront);
			}
			else if (side == SKYBOX_DOWN) {
				cp->SetView(camPos, camPos - camUp * tmpDist, camFront);
			}

		}

		Base::Render(context);

		// reset cam (in case some skybox side was rendered)
		//if (side != SKYBOX_NONE) {
		//	cp->SetView(camPos, camPos + camFront * tmpDist, camUp);
		//}

	}

}