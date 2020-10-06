// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "GridAccelerator_ispc.h"
#include "SharedStructuredVolume_ispc.h"
#include "Volume.h"
#include "openvkl/VKLFilter.h"
#include "rkcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredVolume : public Volume<W>
    {
      ~StructuredVolume();

      virtual void commit() override;

      Sampler<W> *newSampler() override;

      box3f getBoundingBox() const override;

      unsigned int getNumAttributes() const override;

      range1f getValueRange() const override;

      VKLFilter getFilter() const
      {
        return filter;
      }

      VKLFilter getGradientFilter() const
      {
        return gradientFilter;
      }

     protected:
      void buildAccelerator();

      range1f valueRange{empty};

      // parameters set in commit()
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      std::vector<Ref<const Data>> attributesData;
      std::vector<Ref<const DataT<uint8_t>>> attributesTimeConfig;
      std::vector<Ref<const DataT<float>>> attributesTimeData;
      VKLFilter filter{VKL_FILTER_TRILINEAR};
      VKLFilter gradientFilter{VKL_FILTER_TRILINEAR};

      // processed on commit(); temporally unstructured `numTimesteps` are
      // converted to uint64_t indices; other values are passed as is (uint8_t)
      std::vector<Ref<const Data>> attributesTimeConfigProcessed;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    StructuredVolume<W>::~StructuredVolume()
    {
      if (this->ispcEquivalent) {
        CALL_ISPC(SharedStructuredVolume_Destructor, this->ispcEquivalent);
      }
    }

    template <int W>
    inline void StructuredVolume<W>::commit()
    {
      dimensions  = this->template getParam<vec3i>("dimensions");
      gridOrigin  = this->template getParam<vec3f>("gridOrigin", vec3f(0.f));
      gridSpacing = this->template getParam<vec3f>("gridSpacing", vec3f(1.f));

      if (this->template hasParamDataT<Data *>("data")) {
        // multiple attributes provided through VKLData array
        Ref<const DataT<Data *>> data =
            this->template getParamDataT<Data *>("data");

        for (const auto &d : *data) {
          attributesData.push_back(d);
        }
      } else if (this->template getParam<Data *>("data", nullptr)) {
        // single attribute provided through single VKLData object
        attributesData.push_back(this->template getParam<Data *>("data"));
      } else {
        throw std::runtime_error(this->toString() +
                                 ": missing required 'data' parameter");
      }

      // motion blur data provided by user through 2 VKLData arrays
      Ref<const DataT<Data *>> timeConfig =
          this->template getParamDataT<Data *>("timeConfig", nullptr);

      if (timeConfig) {
        Ref<const DataT<Data *>> timeData =
            this->template getParamDataT<Data *>("timeData");

        for (const auto &tc : *timeConfig) {
          if (tc) {
            attributesTimeConfig.push_back(&tc->as<uint8_t>());
          } else {
            attributesTimeConfig.push_back(nullptr);
          }
        }

        for (const auto &td : *timeData) {
          if (td) {
            attributesTimeData.push_back(&td->as<float>());
          } else {
            attributesTimeData.push_back(nullptr);
          }
        }

        // check input sizes and types
        if (attributesData.size() != attributesTimeConfig.size() ||
            attributesTimeConfig.size() != attributesTimeData.size()) {
          throw std::runtime_error(
              "mismatch in number of attributes between data and motion blur "
              "inputs");
        }

        for (int i = 0; i < attributesData.size(); i++) {
          if (!attributesTimeConfig[i]) {
            throw std::runtime_error("incorrect data size (attribute " +
                                     std::to_string(i) + ") empty time config");
          }

          if (attributesTimeConfig[i]->size() > 1 &&
              (!attributesTimeData[i] ||
               attributesTimeData[i]->size() != attributesData[i]->size())) {
            throw std::runtime_error("incorrect data size (attribute " +
                                     std::to_string(i) +
                                     ") mismatched data and time sample sizes");
          } else if (attributesTimeConfig[i]->size() == 1 &&
                     attributesTimeData[i]) {
            throw std::runtime_error(
                "incorrect data size (attribute " + std::to_string(i) +
                ") TUV indices supplied but no time samples provided");
          }
        }

        // pre-process TUV `numTimesteps` input into per-voxel indices
        attributesTimeConfigProcessed.clear();

        for (int i = 0; i < attributesTimeConfig.size(); i++) {
          if (attributesTimeConfig[i] && attributesTimeConfig[i]->size() > 1) {
            DataT<uint64_t> *dp =
                new DataT<uint64_t>(attributesTimeConfig[i]->size());

            // this would be a std::exclusive_scan() in C++17
            (*dp)[0] = 0;
            for (size_t j = 1; j < attributesTimeConfig[i]->size(); j++) {
              (*dp)[j] = (*dp)[j - 1] + (*attributesTimeConfig[i])[j - 1];
            }

            attributesTimeConfigProcessed.push_back(dp);
            dp->refDec();
          } else {
            // not TUV; use as-is
            attributesTimeConfigProcessed.push_back(attributesTimeConfig[i]);
          }
        }
      }

      // validate size and type of each provided attribute
      const std::vector<VKLDataType> supportedDataTypes{
          VKL_UCHAR, VKL_SHORT, VKL_USHORT, VKL_FLOAT, VKL_DOUBLE};

      for (int i = 0; i < attributesData.size(); i++) {
        if (attributesData[i]->size() < this->dimensions.long_product()) {
          throw std::runtime_error("incorrect data size (attribute " +
                                   std::to_string(i) +
                                   ") for provided volume dimensions");
        }

        if (std::find(supportedDataTypes.begin(),
                      supportedDataTypes.end(),
                      attributesData[i]->dataType) ==
            supportedDataTypes.end()) {
          throw std::runtime_error(
              this->toString() + ": unsupported data element type (attribute " +
              std::to_string(i) + ") for 'data' parameter");
        }
      }

      filter = (VKLFilter)this->template getParam<int>("filter", filter);
      gradientFilter =
          (VKLFilter)this->template getParam<int>("gradientFilter", filter);
    }

    template <int W>
    inline box3f StructuredVolume<W>::getBoundingBox() const
    {
      ispc::box3f bb = CALL_ISPC(SharedStructuredVolume_getBoundingBox,
                                 this->ispcEquivalent);

      return box3f(vec3f(bb.lower.x, bb.lower.y, bb.lower.z),
                   vec3f(bb.upper.x, bb.upper.y, bb.upper.z));
    }

    template <int W>
    inline unsigned int StructuredVolume<W>::getNumAttributes() const
    {
      return attributesData.size();
    }

    template <int W>
    inline range1f StructuredVolume<W>::getValueRange() const
    {
      return valueRange;
    }

    template <int W>
    inline void StructuredVolume<W>::buildAccelerator()
    {
      void *accelerator = CALL_ISPC(SharedStructuredVolume_createAccelerator,
                                    this->ispcEquivalent);

      vec3i bricksPerDimension;
      bricksPerDimension.x =
          CALL_ISPC(GridAccelerator_getBricksPerDimension_x, accelerator);
      bricksPerDimension.y =
          CALL_ISPC(GridAccelerator_getBricksPerDimension_y, accelerator);
      bricksPerDimension.z =
          CALL_ISPC(GridAccelerator_getBricksPerDimension_z, accelerator);

      const int numTasks =
          bricksPerDimension.x * bricksPerDimension.y * bricksPerDimension.z;
      tasking::parallel_for(numTasks, [&](int taskIndex) {
        CALL_ISPC(GridAccelerator_build, accelerator, taskIndex);
      });

      CALL_ISPC(GridAccelerator_computeValueRange,  // TODO make value range for
                                                    // time samples
                accelerator,
                valueRange.lower,
                valueRange.upper);
    }

  }  // namespace ispc_driver
}  // namespace openvkl
