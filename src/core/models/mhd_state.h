#ifndef PHARE_MHD_STATE_H
#define PHARE_MHD_STATE_H

#include "core/hybrid/hybrid_quantities.h"
#include "core/models/physical_state.h"

namespace PHARE
{
namespace core
{
    using MHDQuantity = HybridQuantity;

    class MHDStateInitializer : public PhysicalStateInitializer
    {
    };


    template<typename VecFieldT>
    class MHDState : public IPhysicalState
    {
    public:
        //-------------------------------------------------------------------------
        //                  start the ResourcesUser interface
        //-------------------------------------------------------------------------

        bool isUsable() const { return B.isUsable() and V.isUsable(); }



        bool isSettable() const { return B.isSettable() and V.isSettable(); }


        auto getCompileTimeResourcesUserList() const { return std::forward_as_tuple(B, V); }

        auto getCompileTimeResourcesUserList() { return std::forward_as_tuple(B, V); }


        //-------------------------------------------------------------------------
        //                  ends the ResourcesUser interface
        //-------------------------------------------------------------------------



        VecFieldT B{"B", MHDQuantity::Vector::B};
        VecFieldT V{"V", MHDQuantity::Vector::V};
    };
} // namespace core
} // namespace PHARE



#endif // PHARE_MHD_STATE_H
