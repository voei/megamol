#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "mmcore/Module.h"
#include "mmcore/param/ParamSlot.h"
#include "mmcore/view/AbstractView.h"

#include "FBOCommFabric.h"
#include "FBOProto.h"

namespace megamol {
namespace pbs {

class FBOTransmitter2 : public megamol::core::Module, public megamol::core::view::AbstractView::Hooks {
public:
    /**
     * Answer the name of this module.
     *
     * @return The name of this module.
     */
    static const char* ClassName(void) { return "FBOTransmitter2"; }

    /**
     * Answer a human readable description of this module.
     *
     * @return A human readable description of this module.
     */
    static const char* Description(void) { return "A simple job module used to transmit FBOs over TCP/IP"; }

    /**
     * Answers whether this module is available on the current system.
     *
     * @return 'true' if the module is available, 'false' otherwise.
     */
    static bool IsAvailable(void) { return true; }

    /**
     * Disallow usage in quickstarts
     *
     * @return false
     */
    static bool SupportQuickstart(void) { return false; }

    /**
     * Ctor
     */
    FBOTransmitter2(void);

    /**
     * Dtor
     */
    virtual ~FBOTransmitter2(void);

    void AfterRender(megamol::core::view::AbstractView* view) override;

protected:
    bool create() override;

    void release() override;

private:
    void swapBuffers(void) {
        std::scoped_lock<std::mutex, std::mutex> guard{this->buffer_send_guard_, this->buffer_read_guard_};
        swap(fbo_msg_read_, fbo_msg_send_);
        swap(color_buf_read_, color_buf_send_);
        swap(depth_buf_read_, depth_buf_send_);
    }

    void transmitterJob();

    bool triggerButtonClicked(core::param::ParamSlot& slot);

    bool extractBoundingBox(float bbox[6]);

    megamol::core::param::ParamSlot address_slot_;

    megamol::core::param::ParamSlot commSelectSlot_;

    megamol::core::param::ParamSlot view_name_slot_;

    megamol::core::param::ParamSlot trigger_button_slot_;

    megamol::core::param::ParamSlot target_machine_slot_;

    std::mutex buffer_read_guard_;

    std::mutex buffer_send_guard_;

    std::atomic<id_t> frame_id_;

    bool thread_stop_;

    std::thread transmitter_thread_;

    std::unique_ptr<fbo_msg_header_t> fbo_msg_read_;

    std::unique_ptr<fbo_msg_header_t> fbo_msg_send_;

    std::unique_ptr<std::vector<char>> color_buf_read_;

    std::unique_ptr<std::vector<char>> depth_buf_read_;

    std::unique_ptr<std::vector<char>> color_buf_send_;

    std::unique_ptr<std::vector<char>> depth_buf_send_;

    std::unique_ptr<AbstractCommFabric> comm_impl_;

    std::unique_ptr<FBOCommFabric> comm_;

    int col_buf_el_size_;

    int depth_buf_el_size_;

    bool connected_;
};

} // end namespace pbs
} // end namespace megamol