#!/usr/bin/env python3

from pyphare.cpp import cpp_lib
cpp = cpp_lib()

import os
import h5py
import unittest
import numpy as np
from ddt import ddt, data

from tests.diagnostic import dump_all_diags
from pyphare.pharein import ElectronModel
from pyphare.simulator.simulator import Simulator, startMPI
from pyphare.pharein.simulation import supported_dimensions
from pyphare.pharesee.hierarchy import hierarchy_from, h5_filename_from, h5_time_grp_key
import pyphare.pharein as ph


def setup_model(ppc=100):
    def density(*xyz):
        return 1.

    def by(*xyz):
        from pyphare.pharein.global_vars import sim
        L = sim.simulation_domain()
        _ = lambda i: 0.1*np.sin(2*np.pi*xyz[i]/L[i])
        return np.asarray([_(i) for i,v in enumerate(xyz)]).prod(axis=0)

    def bz(*xyz):
        from pyphare.pharein.global_vars import sim
        L = sim.simulation_domain()
        _ = lambda i: 0.1*np.sin(2*np.pi*xyz[i]/L[i])
        return np.asarray([_(i) for i,v in enumerate(xyz)]).prod(axis=0)

    def bx(*xyz):
        return 1.

    def vx(*xyz):
        return 0.

    def vy(*xyz):
        from pyphare.pharein.global_vars import sim
        L = sim.simulation_domain()
        _ = lambda i: 0.1*np.cos(2*np.pi*xyz[i]/L[i])
        return np.asarray([_(i) for i,v in enumerate(xyz)]).prod(axis=0)

    def vz(*xyz):
        from pyphare.pharein.global_vars import sim
        L = sim.simulation_domain()
        _ = lambda i: 0.1*np.cos(2*np.pi*xyz[i]/L[i])
        return np.asarray([_(i) for i,v in enumerate(xyz)]).prod(axis=0)

    def vthx(*xyz):
        return 0.01

    def vthy(*xyz):
        return 0.01

    def vthz(*xyz):
        return 0.01

    vvv = {
        "vbulkx": vx, "vbulky": vy, "vbulkz": vz,
        "vthx": vthx, "vthy": vthy, "vthz": vthz
    }

    model = ph.MaxwellianFluidModel(
        bx=bx, by=by, bz=bz,
        protons={"mass":1, "charge": 1, "density": density, **vvv, "nbr_part_per_cell":ppc, "init": {"seed": 1337}},
        alpha={"mass":4, "charge": 1, "density": density, **vvv, "nbr_part_per_cell":ppc, "init": {"seed": 2334}},
    )
    ElectronModel(closure="isothermal", Te=0.12)
    return model


out = "phare_outputs/tagging_test/"
simArgs = {
  "time_step_nbr":30000,
  "final_time":30.,
  "boundary_types":"periodic",
  "cells":40,
  "dl":0.3,
  "refinement":"tagging",
  "max_nbr_levels": 3,
  "diag_options": {"format": "phareh5", "options": {"dir": out, "mode":"overwrite", "fine_dump_lvl_max": 10}}
}

def dup(dic):
    dic.update(simArgs.copy())
    return dic


@ddt
class TaggingTest(unittest.TestCase):

    _test_cases = (
      dup({
        "smallest_patch_size": 10,
        "largest_patch_size": 20}),
      dup({
        "smallest_patch_size": 20,
        "largest_patch_size": 20}),
      dup({
        "smallest_patch_size": 20,
        "largest_patch_size": 40})
    )

    def __init__(self, *args, **kwargs):
        super(TaggingTest, self).__init__(*args, **kwargs)
        startMPI()
        self.simulator = None


    def tearDown(self):
        if self.simulator is not None:
            self.simulator.reset()
        self.simulator = None

    def ddt_test_id(self):
        return self._testMethodName.split("_")[-1]


    @data(*_test_cases)
    def test_tagging(self, simInput):
        # UPDATE when 2d tagging is finished
        for ndim in [1]: #supported_dimensions():
            self._test_dump_diags(ndim, **simInput)

    def _test_dump_diags(self, dim, **simInput):
        test_id = self.ddt_test_id()
        for key in ["cells", "dl", "boundary_types"]:
            simInput[key] = [simInput[key] for d in range(dim)]

        for interp in range(1, 4):
            local_out = f"{out}_dim{dim}_interp{interp}_mpi_n_{cpp.mpi_size()}_id{test_id}"
            simInput["diag_options"]["options"]["dir"] = local_out

            simulation = ph.Simulation(**simInput)
            self.assertTrue(len(simulation.cells) == dim)

            dump_all_diags(setup_model().populations)
            self.simulator = Simulator(simulation).initialize().advance().reset()

            self.assertTrue(any([diagInfo.quantity.endswith("tags") for diagInfo in ph.global_vars.sim.diagnostics]))

            for diagInfo in ph.global_vars.sim.diagnostics:
                h5_filepath = os.path.join(local_out, h5_filename_from(diagInfo))
                self.assertTrue(os.path.exists(h5_filepath))

                h5_file = h5py.File(h5_filepath, "r")
                if h5_filepath.endswith("tags.h5"):
                    hier = hierarchy_from(h5_filename=h5_filepath)
                    for patch in hier.level(0).patches:
                        self.assertTrue(len(patch.patch_datas.items()))
                        for qty_name, pd in patch.patch_datas.items():
                            self.assertTrue((pd.dataset[:] >= 0).all())
                            self.assertTrue((pd.dataset[:] <  2).all())

            self.simulator = None
            ph.global_vars.sim = None


if __name__ == "__main__":
    unittest.main()