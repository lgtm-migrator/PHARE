#ifndef HYBRID_HYBRID_STATE_H
#define HYBRID_HYBRID_STATE_H


#include "data/ions/ion_initializer.h"
#include "models/physical_state.h"

#include <cstddef>
#include <utility>

namespace PHARE
{
namespace core
{
    template<typename IonsInitializer>
    class HybridStateInitializer : public PhysicalStateInitializer
    {
    public:
        IonsInitializer ionInitializer;
    };

    /**
     * @brief The HybridState class is a concrete implementation of a IPhysicalState.
     * It holds an Electromag and Ion object manipulated by Hybrid concrete type of ISolver
     */
    template<typename Electromag, typename Ions, typename IonsInitializer>
    class HybridState : public IPhysicalState
    {
    public:
        // TODO HybridState ResourcesUser
        HybridState(IonsInitializer ionsInitializer)
            : ions{std::move(ionsInitializer)}
        {
        }


        Electromag electromag{"EM"};
        Ions ions;

        static constexpr std::size_t dimension = Electromag::dimension;
    };


} // namespace core
} // namespace PHARE


#endif // PHARE_HYBRID_STATE_H