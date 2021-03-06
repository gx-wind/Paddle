//  Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserve.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/*
  Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
  http://www.apache.org/licenses/LICENSE-2.0
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "paddle/framework/tensor_util.h"
#include <gtest/gtest.h>
#include <cmath>
#include <string>

namespace paddle {
namespace framework {

TEST(Copy, Tensor) {
  Tensor src_tensor;
  Tensor dst_tensor;
  platform::CPUDeviceContext cpu_ctx((platform::CPUPlace()));

  int* src_ptr =
      src_tensor.mutable_data<int>(make_ddim({3, 3}), platform::CPUPlace());

  int arr[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  memcpy(src_ptr, arr, 9 * sizeof(int));
  src_tensor.set_layout(DataLayout::kAnyLayout);

  auto cpu_place = new platform::CPUPlace();
  Copy(src_tensor, *cpu_place, &dst_tensor);

  const int* dst_ptr = dst_tensor.data<int>();
  ASSERT_NE(src_ptr, dst_ptr);
  for (size_t i = 0; i < 9; ++i) {
    EXPECT_EQ(src_ptr[i], dst_ptr[i]);
  }

  EXPECT_TRUE(dst_tensor.layout() == src_tensor.layout());

  Tensor slice_tensor = src_tensor.Slice(1, 2);
  Copy(slice_tensor, *cpu_place, &dst_tensor);
  const int* slice_ptr = slice_tensor.data<int>();
  dst_ptr = dst_tensor.data<int>();
  ASSERT_NE(dst_ptr, slice_ptr);
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(dst_ptr[i], slice_ptr[i]);
  }
  EXPECT_TRUE(dst_tensor.layout() == src_tensor.layout());

#ifdef PADDLE_WITH_CUDA
  {
    Tensor src_tensor;
    Tensor gpu_tensor;
    Tensor dst_tensor;

    int* src_ptr =
        src_tensor.mutable_data<int>(make_ddim({3, 3}), platform::CPUPlace());

    int arr[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    memcpy(src_ptr, arr, 9 * sizeof(int));

    // CPU Tensor to GPU Tensor
    auto gpu_place = new platform::CUDAPlace(0);
    platform::CUDADeviceContext gpu_ctx(*gpu_place);
    Copy(src_tensor, *gpu_place, gpu_ctx, &gpu_tensor);

    // GPU Tensor to CPU Tensor
    auto cpu_place = new platform::CPUPlace();
    Copy(gpu_tensor, *cpu_place, gpu_ctx, &dst_tensor);

    // Sync before Compare Tensors
    gpu_ctx.Wait();
    const int* dst_ptr = dst_tensor.data<int>();
    ASSERT_NE(src_ptr, dst_ptr);
    for (size_t i = 0; i < 9; ++i) {
      EXPECT_EQ(src_ptr[i], dst_ptr[i]);
    }

    Tensor slice_tensor = src_tensor.Slice(1, 2);

    // CPU Slice Tensor to GPU Tensor
    Copy(slice_tensor, *gpu_place, gpu_ctx, &gpu_tensor);

    // GPU Tensor to CPU Tensor
    Copy(gpu_tensor, *cpu_place, gpu_ctx, &dst_tensor);

    // Sync before Compare Slice Tensors
    gpu_ctx.Wait();
    const int* slice_ptr = slice_tensor.data<int>();
    dst_ptr = dst_tensor.data<int>();
    ASSERT_NE(dst_ptr, slice_ptr);
    for (size_t i = 0; i < 3; ++i) {
      EXPECT_EQ(dst_ptr[i], slice_ptr[i]);
    }

    EXPECT_TRUE(dst_tensor.layout() == src_tensor.layout());
  }
#endif
}

TEST(CopyFromVector, Tensor) {
  using namespace paddle::framework;
  using namespace paddle::platform;
  {
    std::vector<int> src_vec = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    Tensor cpu_tensor;

    // Copy to CPU Tensor
    cpu_tensor.Resize(make_ddim({3, 3}));
    auto cpu_place = new paddle::platform::CPUPlace();
    CopyFromVector<int>(src_vec, &cpu_tensor);

    // Compare Tensors
    const int* cpu_ptr = cpu_tensor.data<int>();
    const int* src_ptr = src_vec.data();
    ASSERT_NE(src_ptr, cpu_ptr);
    for (size_t i = 0; i < 9; ++i) {
      EXPECT_EQ(src_ptr[i], cpu_ptr[i]);
    }

    src_vec.erase(src_vec.begin(), src_vec.begin() + 5);
    cpu_tensor.Resize(make_ddim({2, 2}));
    CopyFromVector<int>(src_vec, &cpu_tensor);
    cpu_ptr = cpu_tensor.data<int>();
    src_ptr = src_vec.data();
    ASSERT_NE(src_ptr, cpu_ptr);
    for (size_t i = 0; i < 5; ++i) {
      EXPECT_EQ(src_ptr[i], cpu_ptr[i]);
    }

    delete cpu_place;
  }

#ifdef PADDLE_WITH_CUDA
  {
    std::vector<int> src_vec = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    Tensor cpu_tensor;
    Tensor gpu_tensor;
    Tensor dst_tensor;

    // Copy to CPU Tensor
    cpu_tensor.Resize(make_ddim({3, 3}));
    auto cpu_place = new paddle::platform::CPUPlace();
    CPUDeviceContext cpu_ctx(*cpu_place);
    CopyFromVector<int>(src_vec, cpu_ctx, &cpu_tensor);

    // Copy to GPUTensor
    gpu_tensor.Resize(make_ddim({3, 3}));
    auto gpu_place = new paddle::platform::CUDAPlace();
    CUDADeviceContext gpu_ctx(*gpu_place);
    CopyFromVector<int>(src_vec, gpu_ctx, &gpu_tensor);
    // Copy from GPU to CPU tensor for comparison
    Copy(gpu_tensor, *cpu_place, gpu_ctx, &dst_tensor);

    // Sync before Compare Tensors
    gpu_ctx.Wait();
    const int* src_ptr = src_vec.data();
    const int* cpu_ptr = cpu_tensor.data<int>();
    const int* dst_ptr = dst_tensor.data<int>();
    ASSERT_NE(src_ptr, cpu_ptr);
    ASSERT_NE(src_ptr, dst_ptr);
    for (size_t i = 0; i < 9; ++i) {
      EXPECT_EQ(src_ptr[i], cpu_ptr[i]);
      EXPECT_EQ(src_ptr[i], dst_ptr[i]);
    }

    src_vec.erase(src_vec.begin(), src_vec.begin() + 5);

    cpu_tensor.Resize(make_ddim({2, 2}));
    CopyFromVector<int>(src_vec, cpu_ctx, &cpu_tensor);
    gpu_tensor.Resize(make_ddim({2, 2}));
    CopyFromVector<int>(src_vec, gpu_ctx, &gpu_tensor);
    Copy(gpu_tensor, *cpu_place, gpu_ctx, &dst_tensor);

    // Sync before Compare Tensors
    gpu_ctx.Wait();
    src_ptr = src_vec.data();
    cpu_ptr = cpu_tensor.data<int>();
    dst_ptr = dst_tensor.data<int>();
    ASSERT_NE(src_ptr, cpu_ptr);
    ASSERT_NE(src_ptr, dst_ptr);
    for (size_t i = 0; i < 5; ++i) {
      EXPECT_EQ(src_ptr[i], cpu_ptr[i]);
      EXPECT_EQ(src_ptr[i], dst_ptr[i]);
    }

    delete cpu_place;
    delete gpu_place;
  }
#endif
}

TEST(CopyToVector, Tensor) {
  using namespace paddle::framework;
  using namespace paddle::platform;
  {
    Tensor src;
    int* src_ptr = src.mutable_data<int>({3, 3}, CPUPlace());
    for (int i = 0; i < 3 * 3; ++i) {
      src_ptr[i] = i;
    }

    CPUPlace place;
    std::vector<int> dst;
    CopyToVector<int>(src, &dst);

    for (int i = 0; i < 3 * 3; ++i) {
      EXPECT_EQ(src_ptr[i], dst[i]);
    }
  }
#ifdef PADDLE_WITH_CUDA
  {
    std::vector<int> src_vec = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    Tensor gpu_tensor;
    CUDAPlace place;
    CUDADeviceContext gpu_ctx(place);
    CopyFromVector<int>(src_vec, gpu_ctx, &gpu_tensor);

    std::vector<int> dst;
    CopyToVector<int>(gpu_tensor, gpu_ctx, &dst);

    for (int i = 0; i < 3 * 3; ++i) {
      EXPECT_EQ(src_vec[i], dst[i]);
    }
  }
#endif
}

TEST(HasNAN, CPU) {
  using namespace paddle::framework;
  using namespace paddle::platform;
  Tensor src;
  float* buf = src.mutable_data<float>({3}, CPUPlace());
  buf[0] = 0.0;
  buf[1] = NAN;
  buf[2] = 0.0;

  ASSERT_TRUE(HasNAN(src));
}

TEST(HasInf, CPU) {
  using namespace paddle::framework;
  using namespace paddle::platform;
  Tensor src;
  double* buf = src.mutable_data<double>({3}, CPUPlace());
  buf[0] = 1.0;
  buf[1] = INFINITY;
  buf[2] = 0.0;
  ASSERT_TRUE(HasInf(src));
}

TEST(Tensor, SerializeAndDeserialize) {
  framework::Tensor src_tensor;
  int array[6] = {1, 2, 3, 4, 5, 6};
  src_tensor.Resize({2, 3});
  int* src_ptr = src_tensor.mutable_data<int>(platform::CPUPlace());
  for (int i = 0; i < 6; ++i) {
    src_ptr[i] = array[i];
  }
  {
    framework::Tensor dst_tensor;
    auto place = new platform::CPUPlace();
    platform::CPUDeviceContext cpu_ctx(*place);
    std::ostringstream oss;
    SerializeToStream(oss, src_tensor, cpu_ctx);

    std::istringstream iss(oss.str());
    DeserializeFromStream(iss, &dst_tensor, cpu_ctx);
    int* dst_ptr = dst_tensor.mutable_data<int>(platform::CPUPlace());
    for (int i = 0; i < 5; ++i) {
      ASSERT_EQ(dst_ptr[i], array[i]);
    }
    ASSERT_EQ(dst_tensor.dims(), src_tensor.dims());
    delete place;
  }
#ifdef PADDLE_WITH_CUDA
  {
    Tensor gpu_tensor;
    gpu_tensor.Resize({2, 3});
    Tensor dst_tensor;

    auto gpu_place = new platform::CUDAPlace();
    platform::CUDADeviceContext gpu_ctx(*gpu_place);

    Copy(src_tensor, *gpu_place, gpu_ctx, &gpu_tensor);

    std::ostringstream oss;
    SerializeToStream(oss, gpu_tensor, gpu_ctx);

    std::istringstream iss(oss.str());
    DeserializeFromStream(iss, &dst_tensor, gpu_ctx);

    int* dst_ptr = dst_tensor.mutable_data<int>(platform::CPUPlace());
    for (int i = 0; i < 6; ++i) {
      ASSERT_EQ(dst_ptr[i], array[i]);
    }
    delete gpu_place;
  }
#endif
}

}  // namespace framework
}  // namespace paddle
