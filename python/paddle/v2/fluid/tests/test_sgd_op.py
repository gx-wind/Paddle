#  Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserve.
#
#Licensed under the Apache License, Version 2.0 (the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.
import unittest
import numpy as np
import paddle.v2.fluid.core as core
from paddle.v2.fluid.op import Operator
from op_test import OpTest


class TestSGDOp(OpTest):
    def setUp(self):
        self.op_type = "sgd"
        w = np.random.random((102, 105)).astype("float32")
        g = np.random.random((102, 105)).astype("float32")
        lr = np.array([0.1]).astype("float32")

        self.inputs = {'Param': w, 'Grad': g, 'LearningRate': lr}
        self.outputs = {'ParamOut': w - lr * g}

    def test_check_output(self):
        self.check_output()


class TestSparseSGDOp(unittest.TestCase):
    def check_with_place(self, place):
        scope = core.Scope()

        # create and initialize Grad Variable   
        height = 10
        rows = [0, 4, 7]
        row_numel = 12

        grad_selected_rows = scope.var('Grad').get_selected_rows()
        grad_selected_rows.set_height(height)
        grad_selected_rows.set_rows(rows)
        np_array = np.ones((len(rows), row_numel)).astype("float32")
        np_array[0, 0] = 2.0
        np_array[2, 8] = 4.0

        grad_tensor = grad_selected_rows.get_tensor()
        grad_tensor.set(np_array, place)

        # create and initialize Param Variable
        param = scope.var('Param').get_tensor()
        param_array = np.full((height, row_numel), 5.0).astype("float32")
        param.set(param_array, place)

        # create and initialize LeraningRate Variable
        lr = scope.var('LearningRate').get_tensor()
        lr_array = np.full((1), 2.0).astype("float32")
        lr.set(lr_array, place)

        # create and run sgd operator
        sgd_op = Operator(
            "sgd",
            Param='Param',
            Grad='Grad',
            ParamOut='Param',
            LearningRate='LearningRate')
        sgd_op.run(scope, place)

        # get and compare result
        result_array = np.array(param)

        # rows[0] = 0, 5.0 - 2.0 * 2.0
        self.assertAlmostEqual(1.0, result_array[rows[0], 0])
        # rows[0] = 0, 5.0 - 2.0 * 1.0
        self.assertAlmostEqual(3.0, result_array[rows[0], 2])
        # 5.0 - 2.0 * 0.0
        self.assertAlmostEqual(5.0, result_array[1, 0])
        # rows[1] = 4, 5.0 - 2.0 * 1.0
        self.assertAlmostEqual(3.0, result_array[rows[1], 10])
        # 5.0 - 2.0 * 0.0
        self.assertAlmostEqual(5.0, result_array[5, 8])
        # rows[2] = 7, 5.0 - 2.0 * 1.0
        self.assertAlmostEqual(3.0, result_array[rows[2], 1])
        # rows[2] = 7, 5.0 - 2.0 * 4.0
        self.assertAlmostEqual(-3.0, result_array[rows[2], 8])

    def test_sparse_sgd(self):
        places = [core.CPUPlace()]
        if core.is_compile_gpu():
            places.append(core.CUDAPlace(0))
        for place in places:
            self.check_with_place(place)


if __name__ == "__main__":
    unittest.main()
