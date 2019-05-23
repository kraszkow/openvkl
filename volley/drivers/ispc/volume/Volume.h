// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include <ospray/ospcommon/box.h>
#include "common/ManagedObject.h"
#include "common/objectFactory.h"
#include "volley/volley.h"
#include "../iterator/RayIterator.h"

using namespace ospcommon;

namespace volley {
  namespace ispc_driver {

    struct Volume : public ManagedObject
    {
      Volume()                   = default;
      virtual ~Volume() override = default;

      static Volume *createInstance(const std::string &type)
      {
        return createInstanceHelper<Volume, VLY_VOLUME>(type);
      }

      virtual void commit() override
      {
        ManagedObject::commit();
      }

      // volumes must provide their own ray iterators based on their internal
      // acceleration structures.
      virtual RayIterator<1> *newRayIterator(const vec3f &origin,
                                             const vec3f &direction,
                                             const range1f &tRange,
                                             const SamplesMask *samplesMask)
      {
        throw std::runtime_error(
            "newRayIterator() not implemented in this volume!");
      }

      virtual RayIterator<8> *newRayIterator8(const vvec3fn<8> &origin,
                                              const vvec3fn<8> &direction,
                                              const vrange1fn<8> &tRange,
                                              const SamplesMask *samplesMask)
      {
        throw std::runtime_error(
            "newRayIterator8() not implemented in this volume!");
      }

      virtual SamplesMask *newSamplesMask()
      {
        throw std::runtime_error(
            "newSamplesMask() not implemented in this volume!");
      }

      virtual float computeSample(const vec3f &objectCoordinates) const = 0;

      // assumes parameters match the native ISPC data layout for the native
      // vector width; we don't use explicit types partly because virtual
      // template methods are not allowed.
      virtual void computeSampleV(const int *valid,
                                  const void *objectCoordinates,
                                  void *samples) const = 0;

      virtual vec3f computeGradient(const vec3f &objectCoordinates) const = 0;
      virtual box3f getBoundingBox() const                                = 0;
    };

#define VLY_REGISTER_VOLUME(InternalClass, external_name) \
  VLY_REGISTER_OBJECT(Volume, volume, InternalClass, external_name)

  }  // namespace ispc_driver
}  // namespace volley
