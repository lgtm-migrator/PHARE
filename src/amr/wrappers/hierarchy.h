#ifndef PHARE_AMR_HIERARCHY_H
#define PHARE_AMR_HIERARCHY_H


#include <SAMRAI/algs/TimeRefinementIntegrator.h>
#include <SAMRAI/geom/CartesianGridGeometry.h>
#include <SAMRAI/hier/Box.h>
#include <SAMRAI/hier/BoxContainer.h>
#include <SAMRAI/hier/IntVector.h>
#include <SAMRAI/hier/PatchHierarchy.h>
#include <SAMRAI/mesh/BergerRigoutsos.h>
#include <SAMRAI/mesh/GriddingAlgorithm.h>
#include <SAMRAI/mesh/StandardTagAndInitialize.h>
#include <SAMRAI/mesh/TreeLoadBalancer.h>
#include <SAMRAI/tbox/Database.h>
#include <SAMRAI/tbox/DatabaseBox.h>
#include <SAMRAI/tbox/InputManager.h>
#include <SAMRAI/tbox/MemoryDatabase.h>


#include "initializer/data_provider.h"


namespace PHARE::amr
{
template<std::size_t dimension>
void getDomainCoords(PHARE::initializer::PHAREDict& grid, float lower[dimension],
                     float upper[dimension])
{
    static_assert(dimension > 0 and dimension <= 3, "invalid dimension should be >0 and <=3");

    auto nx  = grid["nbr_cells"]["x"].template to<int>();
    auto dx  = grid["meshsize"]["x"].template to<double>();
    lower[0] = static_cast<float>(grid["origin"]["x"].template to<double>());
    upper[0] = static_cast<float>(lower[0] + nx * dx);

    if constexpr (dimension >= 2)
    {
        auto ny  = grid["nbr_cells"]["y"].template to<int>();
        auto dy  = grid["meshsize"]["y"].template to<double>();
        lower[1] = static_cast<float>(grid["origin"]["y"].template to<double>());
        upper[1] = static_cast<float>(lower[1] + ny * dy);
    }

    if constexpr (dimension == 3)
    {
        auto nz  = grid["nbr_cells"]["z"].template to<int>();
        auto dz  = grid["meshsize"]["z"].template to<double>();
        lower[2] = static_cast<float>(grid["origin"]["z"].template to<double>());
        upper[2] = static_cast<float>(lower[2] + nz * dz);
    }
}


template<std::size_t dimension>
auto griddingAlgorithmDatabase(PHARE::initializer::PHAREDict& grid)
{
    static_assert(dimension > 0 and dimension <= 3, "invalid dimension should be >0 and <=3");

    auto samraiDim = SAMRAI::tbox::Dimension{dimension};
    auto db        = std::make_shared<SAMRAI::tbox::MemoryDatabase>("griddingAlgoDB");

    {
        int lowerCell[dimension];
        std::fill_n(lowerCell, dimension, 0);
        int upperCell[dimension];

        upperCell[0] = grid["nbr_cells"]["x"].template to<int>() - 1;

        if constexpr (dimension >= 2)
            upperCell[1] = grid["nbr_cells"]["y"].template to<int>();

        if constexpr (dimension == 3)
            upperCell[2] = grid["nbr_cells"]["z"].template to<int>();

        std::vector<SAMRAI::tbox::DatabaseBox> dbBoxes;
        dbBoxes.push_back(SAMRAI::tbox::DatabaseBox(samraiDim, lowerCell, upperCell));
        db->putDatabaseBoxVector("domain_boxes", dbBoxes);
    }

    {
        float lowerCoord[dimension];
        float upperCoord[dimension];
        getDomainCoords<dimension>(grid, lowerCoord, upperCoord);
        db->putFloatArray("x_lo", lowerCoord, dimension);
        db->putFloatArray("x_up", upperCoord, dimension);
    }

    int periodicity[dimension];
    std::fill_n(periodicity, dimension, 1); // 1==periodic, hardedcoded for all dims for now.
    db->putIntegerArray("periodic_dimension", periodicity, dimension);
    return db;
}



/*
// Required input: maximum number of levels in patch hierarchy
max_levels = 4
// Required input: vector ratio between each finer level and next coarser
ratio_to_coarser {
   level_1 = 2, 2, 2
   level_2 = 2, 2, 2
   level_3 = 4, 4, 4
}
// Optional input: int vector for largest patch size on each level.
largest_patch_size {
   level_0 = 40, 40, 40
   level_1 = 30, 30, 30
   // all finer levels will use same values as level_1...
}
// Optional input: int vector for smallest patch size on each level.
smallest_patch_size {
   level_0 = 16, 16, 16
   // all finer levels will use same values as level_0...
}
// Optional input:  buffer of one cell used on each level
proper_nesting_buffer = 1
 */
template<std::size_t dimension>
auto patchHierarchyDatabase(PHARE::initializer::PHAREDict& amr)
{
    constexpr int ratio = 2; // Nothing else supported

    auto hierDB = std::make_shared<SAMRAI::tbox::MemoryDatabase>("HierarchyDB");

    auto maxLevelNumber = amr["max_nbr_levels"].template to<int>();
    hierDB->putInteger("max_levels", maxLevelNumber);

    auto ratioToCoarserDB = hierDB->putDatabase("ratio_to_coarser");

    int smallestPatchSize = 0, largestPatchSize = 0;
    std::shared_ptr<SAMRAI::tbox::Database> smallestPatchSizeDB, largestPatchSizeDB;

    if (amr.contains("smallest_patch_size"))
    {
        smallestPatchSizeDB = hierDB->putDatabase("smallest_patch_size");
        smallestPatchSize   = amr["smallest_patch_size"].template to<int>();
    }

    if (amr.contains("largest_patch_size"))
    {
        largestPatchSizeDB = hierDB->putDatabase("largest_patch_size");
        largestPatchSize   = amr["largest_patch_size"].template to<int>();
    }

    auto addIntDimArray = [](auto& db, auto& value, auto& level) {
        int arr[dimension];
        std::fill_n(arr, dimension, value);
        db->putIntegerArray(level, arr, dimension);
    };

    for (auto iLevel = 0; iLevel < maxLevelNumber; ++iLevel)
    {
        std::string level{"level_" + std::to_string(iLevel)};

        if (iLevel > 0)
            addIntDimArray(ratioToCoarserDB, ratio, level);

        if (smallestPatchSizeDB)
            addIntDimArray(smallestPatchSizeDB, smallestPatchSize, level);

        if (largestPatchSizeDB)
            addIntDimArray(largestPatchSizeDB, largestPatchSize, level);
    }

    return hierDB;
}




class Hierarchy : public SAMRAI::hier::PatchHierarchy
{
private:
    static constexpr int dimension = 1;


public:
    Hierarchy(PHARE::initializer::PHAREDict dict)
        : SAMRAI::hier::PatchHierarchy{
            "PHARE_hierarchy",
            std::make_shared<SAMRAI::geom::CartesianGridGeometry>(
                SAMRAI::tbox::Dimension{dimension}, "CartesianGridGeom",
                griddingAlgorithmDatabase<dimension>(dict["simulation"]["grid"])),
            patchHierarchyDatabase<dimension>(dict["simulation"]["AMR"])}
    {
    }
};


} // namespace PHARE::amr
#endif