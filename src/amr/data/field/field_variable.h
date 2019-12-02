#ifndef PHARE_SRC_AMR_FIELD_FIELD_VARIABLE_H
#define PHARE_SRC_AMR_FIELD_FIELD_VARIABLE_H

#include <SAMRAI/hier/Variable.h>

#include <utility>

//#include "core/data/grid/gridlayout.h"
//  #include "core/data/grid/gridlayout_impl.h"
#include "field_data_factory.h"

namespace PHARE
{
namespace amr
{
    template<typename GridLayoutT, typename FieldImpl,
             typename PhysicalQuantity = decltype(std::declval<FieldImpl>().physicalQuantity())>
    /**
     * @brief The FieldVariable class
     */
    class FieldVariable : public SAMRAI::hier::Variable
    {
    public:
        static constexpr std::size_t dimension    = GridLayoutT::dimension;
        static constexpr std::size_t interp_order = GridLayoutT::interp_order;

        /** \brief Construct a new variable with an unique name, and a specific PhysicalQuantity
         *
         *  FieldVariable represent a data on a patch, it does not contain the data itself,
         *  after creation, one need to register it with a context : see registerVariableAndContext.
         */
        FieldVariable(std::string const& name, PhysicalQuantity qty,
                      bool fineBoundaryRepresentsVariable = true)
            : SAMRAI::hier::Variable(
                name,
                std::make_shared<FieldDataFactory<GridLayoutT, FieldImpl>>(
                    fineBoundaryRepresentsVariable, computeDataLivesOnPatchBorder_(qty), name, qty))
            , fineBoundaryRepresentsVariable_{fineBoundaryRepresentsVariable}
            , dataLivesOnPatchBorder_{computeDataLivesOnPatchBorder_(qty)}
        {
        }


        // The fine boundary representation boolean argument indicates which values (either coarse
        // or fine) take precedence at coarse-fine mesh boundaries during coarsen and refine
        // operations. The default is that fine data values take precedence on coarse-fine
        // interfaces.
        bool fineBoundaryRepresentsVariable() const final
        {
            return fineBoundaryRepresentsVariable_;
        }



        /** \brief Determines whether or not if data may lives on patch border
         *
         *  It will be true if in at least one direction, the data is primal
         */
        bool dataLivesOnPatchBorder() const final { return dataLivesOnPatchBorder_; }

    private:
        bool const fineBoundaryRepresentsVariable_;
        bool const dataLivesOnPatchBorder_;



        bool computeDataLivesOnPatchBorder_(PhysicalQuantity qty)
        {
            auto const& centering = GridLayoutT::centering(qty);


            for (auto const& qtyCentering : centering)
            {
                if (qtyCentering == core::QtyCentering::primal)
                {
                    return true;
                }
            }
            return false;
        }
    };
} // namespace amr


} // namespace PHARE
#endif
