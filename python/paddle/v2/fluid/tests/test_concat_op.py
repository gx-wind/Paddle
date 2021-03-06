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
from op_test import OpTest


class TestConcatOp(OpTest):
    def setUp(self):
        self.op_type = "concat"
        x0 = np.random.random((2, 1, 4, 5)).astype('float32')
        x1 = np.random.random((2, 2, 4, 5)).astype('float32')
        x2 = np.random.random((2, 3, 4, 5)).astype('float32')
        axis = 1
        self.inputs = {'X': [('x0', x0), ('x1', x1), ('x2', x2)]}
        self.attrs = {'axis': axis}
        self.outputs = {'Out': np.concatenate((x0, x1, x2), axis=axis)}

    def test_check_output(self):
        self.check_output()

    def test_check_grad(self):
        self.check_grad(['x0'], 'Out')


if __name__ == '__main__':
    unittest.main()
