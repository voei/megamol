/*
 * AmbientLight.cpp
 * Copyright (C) 2009-2017 by MegaMol Team
 * Alle Rechte vorbehalten.
 */

#include "stdafx.h"
#include "mmcore/view/light/AmbientLight.h"
#include "mmcore/param/FloatParam.h"
#include "mmcore/param/ColorParam.h"

using namespace megamol::core::view::light;

/*
 * megamol::core::view::light::AmbientLight::AmbientLight
 */
AmbientLight::AmbientLight(void) : AbstractLight() {}

/*
 * megamol::core::view::light::AmbientLight::AmbientLight
 */
AmbientLight::~AmbientLight(void) { this->Release(); }

/*
 * megamol::core::view::light::AmbientLight::readParams
 */
void AmbientLight::readParams() {
    lightContainer.lightType = lightenum::AMBIENTLIGHT;
    lightContainer.lightColor = this->lightColor.Param<core::param::ColorParam>()->Value();
    lightContainer.lightIntensity = this->lightIntensity.Param<core::param::FloatParam>()->Value();
}

/*
 * megamol::core::view::light::AmbientLight::InterfaceIsDirty
 */
bool AmbientLight::InterfaceIsDirty() {
    if (this->AbstractIsDirty()) {
        return true;
    } else {
        return false;
    }
}
