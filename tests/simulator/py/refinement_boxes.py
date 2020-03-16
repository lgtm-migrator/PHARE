#!/usr/bin/env python3
#
# formatted with black

import unittest, os, phare.pharein as ph
from datetime import datetime, timezone
from ddt import ddt, data
from tests.simulator import test_simulator as tst
from tests.simulator.py import basicSimulatorArgs, makeBasicModel

out = "phare_outputs/valid/refinement_boxes/"
diags = {"diag_options": {"format": "phareh5", "options": {"dir": out}}}


@ddt
class SimulatorRefineBoxInputsB(unittest.TestCase):
    def dup(dic):
        dic.update(diags.copy())
        return dic
    """
      The first set of boxes "B0": [(10,), (14,)]
      Are configured to force there to be a single patch on L0
      This creates a case with MPI that there are an unequal number of
      Patches across MPI domains. This case must be handled and not hang due
      to collective calls not being handled properly.
    """
    valid1D = [
        dup({"refinement_boxes": {"L0": {"B0": [(10,), (14,)]}}}),
        dup({"refinement_boxes": {"L0": {"B0": [(5,), (55,)]}}}),
    ]

    invalid1D = [
        dup({"max_nbr_levels": 1, "refinement_boxes": {"L0": {"B0": [(10,), (50,)]}}}),
        dup({"cells": 55, "refinement_boxes": {"L0": {"B0": [(5,), (65,)]}}}),
        dup({"smallest_patch_size": 100, "largest_patch_size": 64,}),
    ]

    def tearDown(self):
        for k in ["dman", "sim", "hier"]:
            if hasattr(self, k):
                v = getattr(self, k)
                del v  # blocks segfault on test failure, could be None
        tst.reset()


    def _create_simulator(self, dim, interp, **input):
        ph.globals.sim = None
        ph.Simulation(**basicSimulatorArgs(dim, interp, **input))
        makeBasicModel()
        ph.populateDict()
        hier = tst.make_hierarchy()
        sim = tst.make_simulator(hier)
        sim.initialize()
        return [hier, sim, tst.make_diagnostic_manager(sim, hier)]


    def _do_dim(self, dim, input, valid: bool = False):
        for interp in range(1, 4):
            try:
                self.hier, self.sim, self.dman = self._create_simulator(dim, interp, **input)
                self.assertTrue(valid)
                self.dman.dump()
                del (
                    self.dman,
                    self.sim,
                    self.hier,
                )
                tst.reset()
            except ValueError as e:
                self.assertTrue(not valid)

    @data(*valid1D)
    def test_1d_valid(self, input):
        self._do_dim(1, input, True)

    @data(*invalid1D)
    def test_1d_invalid(self, input):
        self._do_dim(1, input)


if __name__ == "__main__":
    unittest.main()
