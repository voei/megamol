#include "stdafx.h"
#include "HistogramRenderer2D.h"

#include "mmcore/param/BoolParam.h"
#include "mmcore/param/IntParam.h"

using namespace megamol;
using namespace megamol::infovis;
using namespace megamol::stdplugin::datatools;

using vislib::sys::Log;

HistogramRenderer2D::HistogramRenderer2D()
    : Renderer2D()
    , tableDataCallerSlot("getData", "Float table input")
    , transferFunctionCallerSlot("getTransferFunction", "Transfer function input")
    , flagStorageReadCallerSlot("readFlagStorage", "Flag storage read input")
    , numberOfBinsParam("numberOfBins", "Number of bins")
    , logPlotParam("logPlot", "Logarithmic scale")
    , currentTableDataHash(std::numeric_limits<std::size_t>::max())
    , currentTableFrameId(std::numeric_limits<unsigned int>::max())
    , bins(10)
    , colCount(0)
    , rowCount(0)
    , maxBinValue(0)
    , font("Evolventa-SansSerif", core::utility::SDFFont::RenderType::RENDERTYPE_FILL) {
    this->tableDataCallerSlot.SetCompatibleCall<table::TableDataCallDescription>();
    this->MakeSlotAvailable(&this->tableDataCallerSlot);

    this->transferFunctionCallerSlot.SetCompatibleCall<core::view::CallGetTransferFunctionDescription>();
    this->MakeSlotAvailable(&this->transferFunctionCallerSlot);

    this->flagStorageReadCallerSlot.SetCompatibleCall<core::FlagCallRead_GLDescription>();
    this->MakeSlotAvailable(&this->flagStorageReadCallerSlot);

    this->numberOfBinsParam << new core::param::IntParam(this->bins, 1);
    this->MakeSlotAvailable(&this->numberOfBinsParam);

    this->logPlotParam << new core::param::BoolParam(false);
    this->MakeSlotAvailable(&this->logPlotParam);
}

HistogramRenderer2D::~HistogramRenderer2D() { this->Release(); }

bool HistogramRenderer2D::create() {
    if (!this->font.Initialise(this->GetCoreInstance())) return false;
    this->font.SetBatchDrawMode(true);

    if (!makeProgram("::histo::calc", this->calcHistogramProgram)) return false;
    if (!makeProgram("::histo::draw", this->histogramProgram)) return false;
    if (!makeProgram("::histo::axes", this->axesProgram)) return false;

    glGenBuffers(1, &this->floatDataBuffer);
    glGenBuffers(1, &this->minBuffer);
    glGenBuffers(1, &this->maxBuffer);
    glGenBuffers(1, &this->histogramBuffer);
    glGenBuffers(1, &this->selectedHistogramBuffer);
    glGenBuffers(1, &this->maxBinValueBuffer);

    return true;
}

void HistogramRenderer2D::release() {
    this->font.Deinitialise();

    this->calcHistogramProgram.Release();
    this->histogramProgram.Release();
    this->axesProgram.Release();

    glDeleteBuffers(1, &this->floatDataBuffer);
    glDeleteBuffers(1, &this->minBuffer);
    glDeleteBuffers(1, &this->maxBuffer);
    glDeleteBuffers(1, &this->histogramBuffer);
    glDeleteBuffers(1, &this->selectedHistogramBuffer);
    glDeleteBuffers(1, &this->maxBinValueBuffer);
}

bool HistogramRenderer2D::GetExtents(core::view::CallRender2D& call) {
    if (!handleCall(call)) {
        return false;
    }

    // Draw histogram within 10.0 x 10.0 quads, left + right margin 1.0, top and bottom 2.0 for title and axes
    float sizeX = static_cast<float>(std::max<size_t>(1, this->colCount)) * 12.0f;
    call.SetBoundingBox(0.0f, 0.0f, sizeX, 14.0f);
    return true;
}

bool HistogramRenderer2D::Render(core::view::CallRender2D& call) {
    if (!handleCall(call)) {
        return false;
    }

    auto tfCall = this->transferFunctionCallerSlot.CallAs<core::view::CallGetTransferFunction>();
    if (tfCall == nullptr) {
        return false;
    }

    // this is the apex of suck and must die
    GLfloat modelViewMatrix_column[16];
    GLfloat projMatrix_column[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelViewMatrix_column);
    glGetFloatv(GL_PROJECTION_MATRIX, projMatrix_column);
    // end suck

    this->histogramProgram.Enable();
    glUniformMatrix4fv(this->histogramProgram.ParameterLocation("modelView"), 1, GL_FALSE, modelViewMatrix_column);
    glUniformMatrix4fv(this->histogramProgram.ParameterLocation("projection"), 1, GL_FALSE, projMatrix_column);

    tfCall->BindConvenience(this->histogramProgram, GL_TEXTURE0, 0);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->histogramBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, this->selectedHistogramBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, this->maxBinValueBuffer);

    glUniform1i(this->histogramProgram.ParameterLocation("binCount"), this->bins);
    glUniform1i(this->histogramProgram.ParameterLocation("colCount"), this->colCount);
    glUniform1i(this->histogramProgram.ParameterLocation("logPlot"),
        static_cast<int>(this->logPlotParam.Param<core::param::BoolParam>()->Value()));

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, this->bins * this->colCount);

    tfCall->UnbindConvenience();
    glBindVertexArray(0);
    this->histogramProgram.Disable();

    this->axesProgram.Enable();
    glUniformMatrix4fv(this->axesProgram.ParameterLocation("modelView"), 1, GL_FALSE, modelViewMatrix_column);
    glUniformMatrix4fv(this->axesProgram.ParameterLocation("projection"), 1, GL_FALSE, projMatrix_column);
    glUniform2f(this->axesProgram.ParameterLocation("colTotalSize"), 12.0f, 14.0f);
    glUniform2f(this->axesProgram.ParameterLocation("colDrawSize"), 10.0f, 10.0f);
    glUniform2f(this->axesProgram.ParameterLocation("colDrawOffset"), 1.0f, 2.0f);

    glUniform1i(this->axesProgram.ParameterLocation("mode"), 0);
    glDrawArraysInstanced(GL_LINES, 0, 2, this->colCount);

    glUniform1i(this->axesProgram.ParameterLocation("mode"), 1);
    glDrawArrays(GL_LINES, 0, 2);

    this->axesProgram.Disable();

    this->font.ClearBatchDrawCache();

    float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    for (size_t c = 0; c < this->colCount; ++c) {
        float posX = 12.0f * c + 6.0f;
        this->font.DrawString(white, posX, 13.0f, 1.0f, false, this->colNames[c].c_str(),
            core::utility::AbstractFont::ALIGN_CENTER_MIDDLE);
        this->font.DrawString(white, posX - 5.0f, 2.0f, 1.0f, false, std::to_string(this->colMinimums[c]).c_str(),
            core::utility::AbstractFont::ALIGN_LEFT_TOP);
        this->font.DrawString(white, posX + 5.0f, 2.0f, 1.0f, false, std::to_string(this->colMaximums[c]).c_str(),
            core::utility::AbstractFont::ALIGN_RIGHT_TOP);
    }
    this->font.DrawString(white, 1.0f, 12.0f, 1.0f, false, std::to_string(this->maxBinValue).c_str(),
        core::utility::AbstractFont::ALIGN_RIGHT_TOP);
    this->font.DrawString(white, 1.0f, 2.0f, 1.0f, false, "0", core::utility::AbstractFont::ALIGN_RIGHT_BOTTOM);

    this->font.BatchDrawString();

    return true;
}

bool HistogramRenderer2D::handleCall(core::view::CallRender2D& call) {
    auto floatTableCall = this->tableDataCallerSlot.CallAs<table::TableDataCall>();
    if (floatTableCall == nullptr) {
        return false;
    }
    auto tfCall = this->transferFunctionCallerSlot.CallAs<core::view::CallGetTransferFunction>();
    if (tfCall == nullptr) {
        vislib::sys::Log::DefaultLog.WriteMsg(
            vislib::sys::Log::LEVEL_ERROR, "HistogramRenderer2D requires a transfer function!");
        return false;
    }
    auto readFlagsCall = this->flagStorageReadCallerSlot.CallAs<core::FlagCallRead_GL>();
    if (readFlagsCall == nullptr) {
        vislib::sys::Log::DefaultLog.WriteMsg(
            vislib::sys::Log::LEVEL_ERROR, "HistogramRenderer2D requires a flag storage!");
        return false;
    }

    floatTableCall->SetFrameID(static_cast<unsigned int>(call.Time()));
    (*floatTableCall)(1);
    (*floatTableCall)(0);
    call.SetTimeFramesCount(floatTableCall->GetFrameCount());
    auto hash = floatTableCall->DataHash();
    auto frameId = floatTableCall->GetFrameID();
    (*tfCall)(0);
    (*readFlagsCall)(core::FlagCallRead_GL::CallGetData);

    bool dataChanged = this->currentTableDataHash != hash || this->currentTableFrameId != frameId;
    if (dataChanged) {
        this->colCount = floatTableCall->GetColumnsCount();
        this->rowCount = floatTableCall->GetRowsCount();

        this->colMinimums.resize(this->colCount);
        this->colMaximums.resize(this->colCount);
        this->colNames.resize(this->colCount);

        for (size_t i = 0; i < this->colCount; ++i) {
            auto colInfo = floatTableCall->GetColumnsInfos()[i];
            this->colMinimums[i] = colInfo.MinimumValue();
            this->colMaximums[i] = colInfo.MaximumValue();
            this->colNames[i] = colInfo.Name();
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->floatDataBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->colCount * this->rowCount * sizeof(float),
            floatTableCall->GetData(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->minBuffer);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, this->colCount * sizeof(float), this->colMinimums.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->maxBuffer);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, this->colCount * sizeof(float), this->colMaximums.data(), GL_STATIC_DRAW);
    }

    auto binsParam = static_cast<size_t>(this->numberOfBinsParam.Param<core::param::IntParam>()->Value());
    if (dataChanged || readFlagsCall->hasUpdate() || this->bins != binsParam) {
        vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_INFO, "Calculate Histogram");

        this->bins = binsParam;

        GLint zero = 0.0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->histogramBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->colCount * this->bins * sizeof(GLint), nullptr, GL_STATIC_COPY);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &zero);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->selectedHistogramBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->colCount * this->bins * sizeof(GLint), nullptr, GL_STATIC_COPY);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &zero);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->maxBinValueBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLint), nullptr, GL_STATIC_COPY);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &zero);

        readFlagsCall->getData()->validateFlagCount(floatTableCall->GetRowsCount());

        this->calcHistogramProgram.Enable();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->floatDataBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, this->minBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, this->maxBuffer);
        readFlagsCall->getData()->flags->bind(3);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, this->histogramBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, this->selectedHistogramBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, this->maxBinValueBuffer);

        glUniform1ui(this->calcHistogramProgram.ParameterLocation("binCount"), this->bins);
        glUniform1ui(this->calcHistogramProgram.ParameterLocation("colCount"), this->colCount);
        glUniform1ui(this->calcHistogramProgram.ParameterLocation("rowCount"), this->rowCount);

        this->calcHistogramProgram.Dispatch(1, 1, 1);

        this->calcHistogramProgram.Disable();

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Download max bin value for text label.
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->maxBinValueBuffer);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLint), &this->maxBinValue);

        this->currentTableDataHash = hash;
        this->currentTableFrameId = frameId;
    }

    return true;
}

bool HistogramRenderer2D::OnMouseButton(
    core::view::MouseButton button, core::view::MouseButtonAction action, core::view::Modifiers mods) {
    return false;
}

bool HistogramRenderer2D::OnMouseMove(double x, double y) { return false; }
