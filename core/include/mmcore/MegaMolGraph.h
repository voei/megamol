#ifndef MEGAMOLCORE_MEGAMOLGRAPH_H_INCLUDED
#define MEGAMOLCORE_MEGAMOLGRAPH_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#    pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */

#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include "mmcore/Call.h"
#include "mmcore/Module.h"
#include "mmcore/ModuleNamespace.h"
#include "mmcore/api/MegaMolCore.h"
#include "mmcore/param/AbstractParam.h"
#include "mmcore/param/ParamSlot.h"
#include "mmcore/param/ParamUpdateListener.h"
#include "vislib/SmartPtr.h"

#include "mmcore/deferrable_construction.h"
#include "mmcore/lockable.h"
#include "mmcore/serializable.h"

#include "AbstractUpdateQueue.h"

namespace megamol {
namespace core {

class MegaMolGraph : public serializable, public deferrable_construction {

    // todo: where do the descriptionmanagers go?
    // todo: what about the view / job descriptions?
public:
    // todo: constructor(s)? see serialization

    // todo: the lock!
    // todo: probably get rid of RootModuleNamespace altogether

    ///////////////////////////// types ////////////////////////////////////////
    using ModuleDeletionRequest_t = std::string;

    using ModuleDeletionQueue_t = AbstractUpdateQueue<ModuleDeletionRequest_t>;

    struct ModuleInstantiationRequest {
        std::string className;
        std::string id;
    };

    using ModuleInstantiationRequest_t = ModuleInstantiationRequest;

    using ModuleInstantiationQueue_t = AbstractUpdateQueue<ModuleInstantiationRequest_t>;

    struct CallDeletionRequest {
        std::string from;
        std::string to;
    };

    using CallDeletionRequest_t = CallDeletionRequest;

    using CallDeletionQueue_t = AbstractUpdateQueue<CallDeletionRequest_t>;

    struct CallInstantiationRequest {
        std::string className;
        std::string from;
        std::string to;
    };

    using CallInstantiationRequest_t = CallInstantiationRequest;

    using CallInstantiationQueue_t = AbstractUpdateQueue<CallInstantiationRequest_t>;


    //////////////////////////// ctor / dtor ///////////////////////////////

    /**
     * Bare construction as stub for deserialization
     */
    MegaMolGraph();

    /**
     * No copy-construction. This can only be a legal operation, if we allow deep-copy of Modules in graph.
     */
    MegaMolGraph(MegaMolGraph const& rhs) = delete;

    /**
     * Same argument as for copy-construction.
     */
    MegaMolGraph& operator=(MegaMolGraph const& rhs) = delete;

    /**
     * A move of the graph should be OK, even without changing state of Modules in graph.
     */
    MegaMolGraph(MegaMolGraph&& rhs) noexcept;

    /**
     * Same is true for move-assignment.
     */
    MegaMolGraph& operator=(MegaMolGraph&& rhs) noexcept;

    /**
     * Construction from serialized string.
     */
    MegaMolGraph(std::string const& descr);

    /** dtor */
    virtual ~MegaMolGraph();

    //////////////////////////// END ctor / dtor ///////////////////////////////

    //////////////////////////// serialization ////////////////////////////////

    /*
     * Each module should be serializable, i.e. the modules capture their entire state.
     * As a result, an entire MegaMolGraph can basically be copied by reinitializing the serialized descriptor.
     * Therefore the MegaMolGraph creates its descriptor by iterating through all modules and calls in the graph.
     *
     * Maybe the ModuleGraph should even allow external objects to iterate through a linearized array of containing
     * modules and calls.
     */

    //////////////////////////// END serialization ////////////////////////////////

    //////////////////////////// queue methods ////////////////////////////////////
    bool QueueModuleDeletion(std::string const& id);

    bool QueueModuleDeletion(std::string&& id);

    bool QueueModuleDeletionNoLock(std::string const& id);

    bool QueueModuleDeletionNoLock(std::string&& id);

    bool QueueModuleInstantiation(std::string const& className, std::string const& id);

    bool QueueModuleInstantiation(std::string&& className, std::string&& id);

    bool QueueModuleInstantiationNoLock(std::string const& className, std::string const& id);

    bool QueueModuleInstantiationNoLock(std::string&& className, std::string&& id);

    bool QueueCallDeletion(std::string const& from, std::string const& to);

    bool QueueCallDeletion(std::string&& from, std::string&& to);

    bool QueueCallDeletionNoLock(std::string const& from, std::string const& to);

    bool QueueCallDeletionNoLock(std::string&& from, std::string&& to);

    bool QueueCallInstantiation(std::string const& className, std::string const& from, std::string const& to);

    bool QueueCallInstantiation(std::string&& className, std::string&& from, std::string&& to);

    bool QueueCallInstantiationNoLock(std::string const& className, std::string const& from, std::string const& to);

    bool QueueCallInstantiationNoLock(std::string&& className, std::string&& from, std::string&& to);

    //////////////////////////// locking //////////////////////////////////////////
    decltype(auto) AcquireQueueLocks() {
        auto lock1 = module_deletion_queue_.AcquireDeferredLock();
        auto lock2 = module_instantiation_queue_.AcquireDeferredLock();
        auto lock3 = call_deletion_queue_.AcquireDeferredLock();
        auto lock4 = call_instantiation_queue_.AcquireDeferredLock();
        return std::scoped_lock(lock1, lock2, lock3, lock4);
    }

private:
    template <typename Q, typename Arg> bool push_queue_element(Q& q, Arg&& arg) {
        q.Push(std::forward<Arg>(arg));
        return true;
    }

    template <typename Q, typename... Args> bool emplace_queue_element(Q& q, Args&&... args) {
        q.Emplace(std::forward<Args>(args)...);
        return true;
    }

    /** The MegaMolGraph is the owner of the root module */
    std::unique_ptr<Module> root_module_;

    /** Queue for module deletions */
    ModuleDeletionQueue_t module_deletion_queue_;

    /** Queue for module instantiation */
    ModuleInstantiationQueue_t module_instantiation_queue_;

    /** Queue for call deletions */
    CallDeletionQueue_t call_deletion_queue_;

    /** Queue for call instantiations */
    CallInstantiationQueue_t call_instantiation_queue_;


    ////////////////////////// old interface stuff //////////////////////////////////////////////
public:
    bool QueueChainCallInstantiation(const std::string className, const std::string chainStart, const std::string to);
    bool QueueParamValueChange(const std::string id, const std::string value);
    // a JSON serialization of all the requests as above (see updateListener)
    bool QueueGraphChange(const std::string changes);
    /// @return group id 0 is invalid and means failure
    uint32_t CreateParamGroup(const std::string name, int size);
    bool QueueParamGroupValue(const std::string groupName, const std::string id, const std::string value);
    bool QueueParamGroupValue(uint32_t groupId, const std::string id, const std::string value);
    bool QueueUpdateFlush();
    void PerformGraphUpdates();
    bool AnythingQueued();

    // todo: for everything below, RO version AND RW version? or would we just omit the const and imply the user needs
    // to lock?
    ////////////////////////////

    vislib::SmartPtr<param::AbstractParam> FindParameter(const std::string name, bool quiet = false) const;

    // todo: optionally ask for the parameters of a specific module (name OR module pointer?)
    inline void EnumerateParameters(std::function<void(const Module&, param::ParamSlot&)> cb) const;

    // todo: can we get rid of this? where would we do the data unpacking?
    inline void EnumerateParameters(mmcEnumStringAFunction func, void* data) const;

    // todo: optionally pass Module instead of name
    template <class A, class C>
    typename std::enable_if<std::is_convertible<A*, Module*>::value && std::is_convertible<C*, Call*>::value,
        bool>::type
    EnumerateCallerSlots(std::string module_name, std::function<void(C&)> cb) const;

    template <class A>
    typename std::enable_if<std::is_convertible<A*, Module*>::value, bool>::type FindModule(
        std::string module_name, std::function<void(A&)> cb) const;

    // WHY??? this is just EnumerateParameters(FindModule()...) GET RID OF IT!
    template <class A>
    typename std::enable_if<std::is_convertible<A*, Module*>::value, bool>::type EnumerateParameterSlots(
        std::string module_name, std::function<void(param::ParamSlot&)> cb) const;

    size_t GetGlobalParameterHash(void) const;

    // todo: why do we even need this? Like, honestly?
    std::string FindParameterName(const vislib::SmartPtr<param::AbstractParam>& param) const;

    // probably just throws everything away?
    void Cleanup(void);

    // serialize into... JSON? WHY THE FUCK IS THIS IN THE ROOTMODULENAMESPACE!
    std::string SerializeGraph(void) const;

    // replace the whole graph with whatever is in serialization
    void Deserialize(std::string othergraph);

    // construct from serialization
    MegaMolGraph(std::string othergraph);

    // nope, see below!
    // void NotifyParameterValueChange(param::ParamSlot& slot) const;
    // void RegisterParamUpdateListener(param::ParamUpdateListener* pul);
    // void UnregisterParamUpdateListener(param::ParamUpdateListener* pul);

    // accumulate the stuff the queues ask from the graph and give out a JSON diff right afterwards
    // bitfield says what (params, modules, whatnot) - and also reports whether something did not happen
    /// @return some ID to allow for removal of the listener later
    uint32_t RegisterGraphUpdateListener(
        std::function<void(std::string, uint32_t field)> func, int32_t serviceBitfield);
    void UnregisterGraphUpdateListener(uint32_t id);

private:
    // todo: signature is weird, data structure might be as well
    void computeGlobalParameterHash(ModuleNamespace::const_ptr_type path, ParamHashMap_t& map) const;
    static void compareParameterHash(ParamHashMap_t& one, ParamHashMap_t& other) const;

    void updateFlushIdxList(size_t const processedCount, std::vector<size_t>& list);
    bool checkForFlushEvent(size_t const eventIdx, std::vector<size_t>& list) const;
    void shortenFlushIdxList(size_t const eventCount, std::vector<size_t>& list);
};

} /* namespace core */
} /* namespace megamol */

#endif /* MEGAMOLCORE_MEGAMOLGRAPH_H_INCLUDED */