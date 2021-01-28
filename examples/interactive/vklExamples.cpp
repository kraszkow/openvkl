// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AppInit.h"
#include "window/GLFWVKLWindow.h"
#include "window/TransferFunctionWidget.h"
// openvkl_testing
#include "openvkl_testing.h"
// imgui
#include <imgui.h>
// std
#include <array>
#include <map>
#include <memory>
#include <random>

using namespace rkcommon;
using namespace openvkl::testing;
using namespace openvkl::examples;

bool addSamplingRateUI(GLFWVKLWindow &window)
{
  auto &renderer = window.currentRenderer();

  static float samplingRate = 1.f;
  if (ImGui::SliderFloat("samplingRate", &samplingRate, 0.01f, 4.f)) {
    renderer.setParam<float>("samplingRate", samplingRate);
    renderer.commit();
    return true;
  }

  return false;
}

bool addPathTracerUI(GLFWVKLWindow &window, Scene &scene)
{
  auto &renderer = window.currentRenderer();

  bool changed = false;

  static int attributeIndex  = 0;
  unsigned int numAttributes = vklGetNumAttributes(scene.volume);

  if (numAttributes > 1) {
    if (ImGui::SliderInt(
            "attributeIndex", &attributeIndex, 0, numAttributes - 1)) {
      scene.attributeIndex = attributeIndex;
      changed              = true;
    }
  }

  static float time = 0.f;
  if (ImGui::SliderFloat("time", &time, 0.f, 1.f)) {
    renderer.setParam<float>("time", time);
    changed = true;
  }

  static bool motionBlur = false;
  if (ImGui::Checkbox("motion blur", &motionBlur)) {
    renderer.setParam<bool>("motionBlur", motionBlur);
    changed = true;
  }

  static float sigmaTScale = 1.f;
  if (ImGui::SliderFloat("sigmaTScale", &sigmaTScale, 0.001f, 100.f)) {
    renderer.setParam<float>("sigmaTScale", sigmaTScale);
    changed = true;
  }

  static float sigmaSScale = 1.f;
  if (ImGui::SliderFloat("sigmaSScale", &sigmaSScale, 0.01f, 1.f)) {
    renderer.setParam<float>("sigmaSScale", sigmaSScale);
    changed = true;
  }

  static int maxNumScatters = 1;
  if (ImGui::SliderInt("maxNumScatters", &maxNumScatters, 1, 32)) {
    renderer.setParam<int>("maxNumScatters", maxNumScatters);
    changed = true;
  }

  static float ambientLightIntensity = 1.f;
  if (ImGui::SliderFloat(
          "ambientLightIntensity", &ambientLightIntensity, 0.f, 10.f)) {
    renderer.setParam<float>("ambientLightIntensity", ambientLightIntensity);
    changed = true;
  }

  if (changed) {
    renderer.commit();
  }

  return changed;
}

bool addIsosurfacesUI(GLFWVKLWindow &window, std::vector<float> &isoValues)
{
  auto &renderer = window.currentRenderer();

  static bool showIsosurfaces = true;

  static constexpr int maxNumIsosurfaces = 3;

  struct IsosurfaceParameters
  {
    bool enabled{true};
    float isovalue{0.f};
  };

  static std::array<IsosurfaceParameters, maxNumIsosurfaces> isosurfaces;

  static bool initialized = false;
  bool isosurfacesChanged = false;

  if (!initialized) {
    isosurfaces[0].isovalue = -1.f;
    isosurfaces[1].isovalue = 0.f;
    isosurfaces[2].isovalue = 1.f;

    initialized        = true;
    isosurfacesChanged = true;  // Update isovalues on init!
  }

  static float time = 0.f;
  if (ImGui::SliderFloat("time", &time, 0.f, 1.f)) {
    renderer.setParam<float>("time", time);
    isosurfacesChanged = true;
  }

  if (ImGui::Checkbox("show isosurfaces", &showIsosurfaces)) {
    isosurfacesChanged = true;
  }

  if (showIsosurfaces) {
    int labelCounter = 0;

    for (auto &isosurface : isosurfaces) {
      std::ostringstream enabledLabel;
      enabledLabel << "##enabled_isosurface " << labelCounter;

      std::ostringstream isovalueLabel;
      isovalueLabel << "isosurface " << labelCounter;

      if (ImGui::Checkbox(enabledLabel.str().c_str(), &isosurface.enabled)) {
        isosurfacesChanged = true;
      }

      ImGui::SameLine();

      if (ImGui::SliderFloat(
              isovalueLabel.str().c_str(), &isosurface.isovalue, -1.f, 1.f)) {
        isosurfacesChanged = true;
      }

      labelCounter++;
    }
  }

  if (isosurfacesChanged) {
    isoValues.clear();

    if (showIsosurfaces) {
      for (const auto &isosurface : isosurfaces) {
        if (isosurface.enabled) {
          isoValues.push_back(isosurface.isovalue);
        }
      }
    }
  }

  if (isosurfacesChanged) {
    renderer.commit();
  }

  return isosurfacesChanged;
}

bool addIntervalIteratorDebugUI(GLFWVKLWindow &window)
{
  auto &renderer = window.currentRenderer();

  bool changed = false;

  static int intervalColorScale = 4;
  if (ImGui::SliderInt("intervalColorScale", &intervalColorScale, 1, 32)) {
    renderer.setParam<float>("intervalColorScale", float(intervalColorScale));
    changed = true;
  }

  static float intervalOpacity = 0.25f;
  if (ImGui::SliderFloat("intervalOpacity", &intervalOpacity, 0.01f, 1.f)) {
    renderer.setParam<float>("intervalOpacity", intervalOpacity);
    changed = true;
  }

  static bool firstIntervalOnly = false;
  if (ImGui::Checkbox("firstIntervalOnly", &firstIntervalOnly)) {
    renderer.setParam<bool>("firstIntervalOnly", firstIntervalOnly);
    changed = true;
  }

  static bool showIntervalBorders = false;
  if (ImGui::Checkbox("showIntervalBorders", &showIntervalBorders)) {
    renderer.setParam<bool>("showIntervalBorders", showIntervalBorders);
    changed = true;
  }

  if (changed) {
    renderer.commit();
  }

  return changed;
}

void usage(const char *progname)
{
  std::cerr
      << "usage: " << progname << "\n"
      << "\t-renderer density_pathtracer | hit_iterator |"
         " ray_march_iterator | interval_iterator_debug\n"
         "\t-disable-vsync\n"
         "\t-gridType structuredRegular | structuredSpherical | "
         "unstructured | amr | vdb | particle\n"
         "\t-gridOrigin <x> <y> <z>\n"
         "\t-gridSpacing <x> <y> <z>\n"
         "\t-gridDimensions <dimX> <dimY> <dimZ>\n"
         "\t-voxelType uchar | short | ushort | float | double\n"
         "\t-valueRange <lower> <upper>\n"
         "\t-multiAttribute (vdb and structuredRegular only, ignores -field)\n"
         "\t-motionBlur structured | unstructured (structuredRegular only)\n"
         "\t-filter nearest | trilinear (vdb and structured)\n"
         "\t-field wavelet | xyz | sphere | <vdb grid name>\n"
         "\t-file <filename>\n"
         "\t-numParticles <N> (particle only)\n"
      << std::endl;
}

int main(int argc, const char **argv)
{
  std::string rendererType("density_pathtracer");
  std::string gridType("structuredRegular");
  vec3i dimensions(128);
  vec3f gridOrigin(rkcommon::nan);
  vec3f gridSpacing(rkcommon::nan);
  std::string voxelTypeString("float");
  VKLDataType voxelType(VKL_FLOAT);
  bool multiAttribute(false);
  bool motionBlurStructured(false);
  uint8_t motionBlurStructuredNumTimesteps(6);
  bool motionBlurUnstructured(false);
  std::vector<float> motionBlurUnstructuredTimeSamples{
      0.f, 0.15f, 0.3f, 0.65f, 0.9f, 1.0f};
  std::string filename;
  bool disableVSync(false);
  VKLFilter filter(VKL_FILTER_TRILINEAR);
  VKLFilter gradientFilter(VKL_FILTER_TRILINEAR);
  bool haveFilter(false);
  bool haveVdb(false);
  std::string field;

  size_t numParticles(1000);
  range1f initialValueRange(0.f, 1.f);

  int argIndex = 1;
  while (argIndex < argc) {
    std::string switchArg(argv[argIndex++]);

    if (switchArg == "-gridType") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -gridType arguments");
      }

      gridType = std::string(argv[argIndex++]);
    } else if (switchArg == "-disable-vsync") {
      disableVSync = true;
    } else if (switchArg == "-gridOrigin") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridOrigin arguments");
      }

      const std::string gridOriginX(argv[argIndex++]);
      const std::string gridOriginY(argv[argIndex++]);
      const std::string gridOriginZ(argv[argIndex++]);

      gridOrigin =
          vec3f(stof(gridOriginX), stof(gridOriginY), stof(gridOriginZ));
    } else if (switchArg == "-gridSpacing") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridSpacing arguments");
      }

      const std::string gridSpacingX(argv[argIndex++]);
      const std::string gridSpacingY(argv[argIndex++]);
      const std::string gridSpacingZ(argv[argIndex++]);

      gridSpacing =
          vec3f(stof(gridSpacingX), stof(gridSpacingY), stof(gridSpacingZ));
    } else if (switchArg == "-gridDimensions") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridDimensions arguments");
      }

      const std::string dimX(argv[argIndex++]);
      const std::string dimY(argv[argIndex++]);
      const std::string dimZ(argv[argIndex++]);

      dimensions = vec3i(stoi(dimX), stoi(dimY), stoi(dimZ));
    } else if (switchArg == "-valueRange") {
      if (argc < argIndex + 2) {
        throw std::runtime_error("improper -valueRange arguments");
      }

      const std::string rangeLower(argv[argIndex++]);
      const std::string rangeUpper(argv[argIndex++]);

      initialValueRange = range1f(stof(rangeLower), stof(rangeUpper));
    } else if (switchArg == "-voxelType") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -voxelType argument");
      }

      std::map<std::string, VKLDataType> stringToVKLDataType = {
          {"uchar", VKL_UCHAR},
          {"short", VKL_SHORT},
          {"ushort", VKL_USHORT},
          {"float", VKL_FLOAT},
          {"double", VKL_DOUBLE}};

      voxelTypeString = std::string(argv[argIndex++]);

      if (!stringToVKLDataType.count(voxelTypeString)) {
        throw std::runtime_error("unsupported -voxelType specified");
      }

      voxelType = stringToVKLDataType[voxelTypeString];
    } else if (switchArg == "-multiAttribute") {
      multiAttribute = true;
    } else if (switchArg == "-motionBlur") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -motionBlur arguments");
      }

      std::string motionBlurType = argv[argIndex++];

      if (motionBlurType == "structured") {
        motionBlurStructured = true;
      } else if (motionBlurType == "unstructured") {
        motionBlurUnstructured = true;
      } else {
        throw std::runtime_error("improper -motionBlur arguments");
      }
    } else if (switchArg == "-file") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -file arguments");
      }

      filename = argv[argIndex++];
    } else if (switchArg == "-field") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -field arguments");
      }

      field = argv[argIndex++];
    } else if (switchArg == "-filter") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -filter arguments");
      }

      haveFilter                  = true;
      const std::string filterArg = argv[argIndex++];
      if (filterArg == "trilinear")
        filter = VKL_FILTER_TRILINEAR;
      else if (filterArg == "nearest")
        filter = VKL_FILTER_NEAREST;
      else
        throw std::runtime_error("unsupported -filter specified");
      gradientFilter = filter;
    } else if (switchArg == "-numParticles") {
      numParticles = std::stoul(argv[argIndex++]);
    } else if (switchArg == "-renderer") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -renderer arguments");
      }

      rendererType = argv[argIndex++];
    } else if (switchArg == "-help") {
      usage(argv[0]);
      return 0;
    } else {
      std::cerr << "unknown argument " << switchArg << std::endl;
      usage(argv[0]);
      throw std::runtime_error("unknown switch argument");
    }
  }

  if (field.empty()) {
    if (filename.empty())
      field = "wavelet";
    else
      field = "density";
  }

  if (haveFilter && gridType != "vdb" && gridType != "structuredRegular" &&
      gridType != "structuredSpherical") {
    std::cerr << "warning: -filter has no effect on " << gridType << " volumes"
              << std::endl;
  }

  // generate gridOrigin and gridSpacing if not specified on the command-line
  if (std::isnan(gridOrigin.x) || std::isnan(gridSpacing.x)) {
    const float boundingBoxSize = 2.f;

    if (gridType == "structuredSpherical") {
      ProceduralStructuredSphericalVolume<>::generateGridParameters(
          dimensions, boundingBoxSize, gridOrigin, gridSpacing);
    } else {
      // all other grid types can use values generated for structured regular
      // volumes
      ProceduralStructuredRegularVolume<>::generateGridParameters(
          dimensions, boundingBoxSize, gridOrigin, gridSpacing);
    }
  }

  initializeOpenVKL();

  std::shared_ptr<TestingVolume> testingVolume;

  if (!filename.empty()) {
    std::cout << "filename:       " << filename << std::endl;

    std::string ext = filename.substr(filename.size() - 4);
    std::for_each(ext.begin(), ext.end(), [](char &c) { c = ::tolower(c); });
    if (ext == ".vdb") {
      gridType = "vdb";
      auto vol = std::shared_ptr<OpenVdbVolume>(
          OpenVdbVolume::loadVdbFile(filename, field, filter));
      testingVolume = std::move(vol);
      haveVdb       = true;
    } else {
      testingVolume = std::shared_ptr<RawFileStructuredVolume>(
          new RawFileStructuredVolume(filename,
                                      gridType,
                                      dimensions,
                                      gridOrigin,
                                      gridSpacing,
                                      voxelType));
    }
  } else {
    if (gridType == "structuredRegular") {
      TemporalConfig temporalConfig;

      if (motionBlurStructured) {
        temporalConfig = TemporalConfig(TemporalConfig::Structured,
                                        motionBlurStructuredNumTimesteps);
      } else if (motionBlurUnstructured) {
        if (field == "sphere" && !multiAttribute) {
          temporalConfig = TemporalConfig(TemporalConfig::Unstructured, 256);
          temporalConfig.numRefitSamples = 4;
        } else {
          temporalConfig = TemporalConfig(motionBlurUnstructuredTimeSamples);
        }
      }

      if (multiAttribute) {
        testingVolume = std::shared_ptr<TestingStructuredVolumeMulti>(
            generateMultiAttributeStructuredRegularVolume(
                dimensions,
                gridOrigin,
                gridSpacing,
                temporalConfig,
                VKL_DATA_SHARED_BUFFER,
                false));
      } else {
        if (voxelType == VKL_UCHAR) {
          if (field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeUChar>(
                dimensions, gridOrigin, gridSpacing, temporalConfig);
          } else if (field == "sphere") {
            testingVolume =
                std::make_shared<SphereStructuredRegularVolumeUChar>(
                    dimensions, gridOrigin, gridSpacing, temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeUChar>(
                    dimensions, gridOrigin, gridSpacing, temporalConfig);
          }
        } else if (voxelType == VKL_SHORT) {
          if (field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeShort>(
                dimensions, gridOrigin, gridSpacing, temporalConfig);
          } else if (field == "sphere") {
            testingVolume =
                std::make_shared<SphereStructuredRegularVolumeShort>(
                    dimensions, gridOrigin, gridSpacing, temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeShort>(
                    dimensions, gridOrigin, gridSpacing, temporalConfig);
          }
        } else if (voxelType == VKL_USHORT) {
          if (field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeUShort>(
                dimensions, gridOrigin, gridSpacing, temporalConfig);
          } else if (field == "sphere") {
            testingVolume =
                std::make_shared<SphereStructuredRegularVolumeUShort>(
                    dimensions, gridOrigin, gridSpacing, temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeUShort>(
                    dimensions, gridOrigin, gridSpacing, temporalConfig);
          }
        } else if (voxelType == VKL_FLOAT) {
          if (field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeFloat>(
                dimensions, gridOrigin, gridSpacing, temporalConfig);
          } else if (field == "sphere") {
            testingVolume =
                std::make_shared<SphereStructuredRegularVolumeFloat>(
                    dimensions, gridOrigin, gridSpacing, temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeFloat>(
                    dimensions, gridOrigin, gridSpacing, temporalConfig);
          }
        } else if (voxelType == VKL_DOUBLE) {
          if (field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeDouble>(
                dimensions, gridOrigin, gridSpacing, temporalConfig);
          } else if (field == "sphere") {
            testingVolume =
                std::make_shared<SphereStructuredRegularVolumeDouble>(
                    dimensions, gridOrigin, gridSpacing, temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeDouble>(
                    dimensions, gridOrigin, gridSpacing, temporalConfig);
          }
        } else {
          throw std::runtime_error(
              "cannot create procedural volume for unknown voxel type");
        }
      }
    }

    else if (gridType == "structuredSpherical") {
      if (voxelType == VKL_UCHAR) {
        if (field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeUChar>(
              dimensions, gridOrigin, gridSpacing);
        } else if (field == "sphere") {
          testingVolume =
              std::make_shared<SphereStructuredSphericalVolumeUChar>(
                  dimensions, gridOrigin, gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeUChar>(
                  dimensions, gridOrigin, gridSpacing);
        }
      } else if (voxelType == VKL_SHORT) {
        if (field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeShort>(
              dimensions, gridOrigin, gridSpacing);
        } else if (field == "sphere") {
          testingVolume =
              std::make_shared<SphereStructuredSphericalVolumeShort>(
                  dimensions, gridOrigin, gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeShort>(
                  dimensions, gridOrigin, gridSpacing);
        }
      } else if (voxelType == VKL_USHORT) {
        if (field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeUShort>(
              dimensions, gridOrigin, gridSpacing);
        } else if (field == "sphere") {
          testingVolume =
              std::make_shared<SphereStructuredSphericalVolumeUShort>(
                  dimensions, gridOrigin, gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeUShort>(
                  dimensions, gridOrigin, gridSpacing);
        }
      } else if (voxelType == VKL_FLOAT) {
        if (field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeFloat>(
              dimensions, gridOrigin, gridSpacing);
        } else if (field == "sphere") {
          testingVolume =
              std::make_shared<SphereStructuredSphericalVolumeFloat>(
                  dimensions, gridOrigin, gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeFloat>(
                  dimensions, gridOrigin, gridSpacing);
        }
      } else if (voxelType == VKL_DOUBLE) {
        if (field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeDouble>(
              dimensions, gridOrigin, gridSpacing);
        } else if (field == "sphere") {
          testingVolume =
              std::make_shared<SphereStructuredSphericalVolumeDouble>(
                  dimensions, gridOrigin, gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeDouble>(
                  dimensions, gridOrigin, gridSpacing);
        }
      } else {
        throw std::runtime_error(
            "cannot create procedural volume for unknown voxel type");
      }
    }

    else if (gridType == "unstructured") {
      if (field == "xyz") {
        testingVolume = std::make_shared<XYZUnstructuredProceduralVolume>(
            dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false);
      } else if (field == "sphere") {
        testingVolume = std::make_shared<SphereUnstructuredProceduralVolume>(
            dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false);
      } else {
        testingVolume = std::make_shared<WaveletUnstructuredProceduralVolume>(
            dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false);
      }
    }

    else if (gridType == "amr") {
      if (voxelType == VKL_FLOAT) {
        testingVolume = std::shared_ptr<ProceduralShellsAMRVolume<>>(
            new ProceduralShellsAMRVolume<>(
                dimensions, gridOrigin, gridSpacing));
      } else {
        throw std::runtime_error(
            "cannot create procedural AMR volume for non-float voxel type");
      }
    }

    else if (gridType == "vdb") {
      if (multiAttribute) {
        testingVolume = std::shared_ptr<ProceduralVdbVolumeMulti>(
            generateMultiAttributeVdbVolume(dimensions,
                                            gridOrigin,
                                            gridSpacing,
                                            filter,
                                            VKL_DATA_SHARED_BUFFER,
                                            false));
      } else {
        if (field == "xyz") {
          testingVolume = std::make_shared<XYZVdbVolume>(
              dimensions, gridOrigin, gridSpacing);
        } else if (field == "sphere") {
          testingVolume = std::make_shared<SphereVdbVolume>(
              dimensions, gridOrigin, gridSpacing);
        } else {
          testingVolume = std::make_shared<WaveletVdbVolume>(
              dimensions, gridOrigin, gridSpacing);
        }
      }

      haveVdb = true;
    }

    else if (gridType == "particle") {
      testingVolume = std::make_shared<ProceduralParticleVolume>(numParticles);
    }

    else {
      throw std::runtime_error("unknown gridType specified");
    }
  }

  if (haveFilter && !haveVdb) {
    std::cerr << "warning: -filter has no effect on " << gridType << " volumes"
              << std::endl;
  }

  int maxIteratorDepth = (gridType == "vdb" ? VKL_VDB_NUM_LEVELS - 2 : 6);
  int maxSamplingDepth = VKL_VDB_NUM_LEVELS - 1;

  Scene scene;
  TransferFunction transferFunction;
  std::vector<float> isoValues;
  scene.updateVolume(testingVolume->getVKLVolume());
  vklSetInt(scene.sampler, "filter", filter);
  vklSetInt(scene.sampler, "gradientFilter", gradientFilter);
  vklSetInt(scene.sampler, "maxSamplingDepth", maxSamplingDepth);
  vklSetInt(scene.sampler, "maxIteratorDepth", maxIteratorDepth);
  vklCommit(scene.sampler);

  VKLObserver leafAccessObserver = nullptr;
  {
    auto vdbVolume = dynamic_cast<OpenVdbVolume *>(testingVolume.get());
    if (vdbVolume)
      leafAccessObserver = vdbVolume->newLeafAccessObserver(scene.sampler);
  }

  std::cout << "renderer:       " << rendererType << std::endl;
  std::cout << "gridType:       " << gridType << std::endl;
  std::cout << "gridDimensions: " << dimensions << std::endl;
  std::cout << "gridOrigin:     " << gridOrigin << std::endl;
  std::cout << "gridSpacing:    " << gridSpacing << std::endl;
  std::cout << "voxelType:      " << voxelTypeString << std::endl;
  std::cout << "field:          " << field << std::endl;

  vkl_box3f bbox = vklGetBoundingBox(scene.volume);

  std::cout << "boundingBox:    "
            << "(" << bbox.lower.x << ", " << bbox.lower.y << ", "
            << bbox.lower.z << ") -> (" << bbox.upper.x << ", " << bbox.upper.y
            << ", " << bbox.upper.z << ")" << std::endl;

  auto glfwVKLWindow = rkcommon::make_unique<GLFWVKLWindow>(
      vec2i{1024, 1024}, scene, rendererType, disableVSync);

  glfwVKLWindow->registerImGuiCallback([&]() {
    bool changed = false;

    static int whichRenderer = 0;
    if (ImGui::Combo("renderer",
                     &whichRenderer,
                     "density_pathtracer\0hit_iterator\0ray_march_"
                     "iterator\0interval_iterator_debug\0\0")) {
      switch (whichRenderer) {
      case 0:
        rendererType = "density_pathtracer";
        break;
      case 1:
        rendererType = "hit_iterator";
        break;
      case 2:
        rendererType = "ray_march_iterator";
        break;
      case 3:
        rendererType = "interval_iterator_debug";
        break;
      default:
        break;
      }

      glfwVKLWindow->setActiveRenderer(rendererType);
    }

    // maxIteratorDepth parameter currently only applies to unstructured and
    // particle volume samplers (special case below for vdb).
    if (gridType == "unstructured" || gridType == "particle") {
      if (ImGui::SliderInt("maxIteratorDepth", &maxIteratorDepth, 0, 31)) {
        vklSetInt(scene.sampler, "maxIteratorDepth", maxIteratorDepth);
        vklCommit(scene.sampler);
        changed = true;
      }
    }

    // VDB specific parameters.
    if (gridType == "vdb") {
      static int whichFilter = (filter == VKL_FILTER_NEAREST ? 0 : 1);
      static int whichGradientFilter =
          (gradientFilter == VKL_FILTER_NEAREST ? 0 : 1);
      if (ImGui::Combo("filter", &whichFilter, "nearest\0trilinear\0\0") ||
          ImGui::Combo("gradientFilter",
                       &whichGradientFilter,
                       "nearest\0trilinear\0\0") ||
          ImGui::SliderInt("maxSamplingDepth",
                           &maxSamplingDepth,
                           0,
                           VKL_VDB_NUM_LEVELS - 1) ||
          ImGui::SliderInt("maxIteratorDepth",
                           &maxIteratorDepth,
                           0,
                           VKL_VDB_NUM_LEVELS - 1)) {
        switch (whichFilter) {
        case 0:
          filter = VKL_FILTER_NEAREST;
          break;
        case 1:
          filter = VKL_FILTER_TRILINEAR;
          break;
        default:
          break;
        }
        vklSetInt(scene.sampler, "filter", filter);
        switch (whichGradientFilter) {
        case 0:
          gradientFilter = VKL_FILTER_NEAREST;
          break;
        case 1:
          gradientFilter = VKL_FILTER_TRILINEAR;
          break;
        default:
          break;
        }
        vklSetInt(scene.sampler, "gradientFilter", gradientFilter);
        vklSetInt(scene.sampler, "maxSamplingDepth", maxSamplingDepth);
        vklSetInt(scene.sampler, "maxIteratorDepth", maxIteratorDepth);
        vklCommit(scene.sampler);
        changed = true;
      }
    }

    static int useISPC = 1;
    if (ImGui::Combo("OpenVKL API used", &useISPC, "C scalar\0ISPC\0\0")) {
      glfwVKLWindow->setUseISPC(useISPC);
      changed = true;
    }

    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 16)) {
      auto &renderer = glfwVKLWindow->currentRenderer();
      renderer.setParam<int>("spp", spp);
      renderer.commit();
    }

    if (rendererType == "ray_march_iterator") {
      changed |= addSamplingRateUI(*glfwVKLWindow);
    }

    if (rendererType == "density_pathtracer") {
      changed |= addPathTracerUI(*glfwVKLWindow, scene);
    }

    if (rendererType == "hit_iterator") {
      if (addIsosurfacesUI(*glfwVKLWindow, isoValues)) {
        changed = true;
        scene.updateValueSelector(transferFunction, isoValues);
      }
    }

    if (rendererType == "interval_iterator_debug") {
      changed |= addIntervalIteratorDebugUI(*glfwVKLWindow);
    }

    auto transferFunctionUpdatedCallback =
        [&](const range1f &valueRange,
            const std::vector<vec4f> &colorsAndOpacities) {
          transferFunction = TransferFunction{valueRange, colorsAndOpacities};
          scene.tfColorsAndOpacities =
              transferFunction.colorsAndOpacities.data();
          scene.tfNumColorsAndOpacities =
              transferFunction.colorsAndOpacities.size();
          scene.tfValueRange = valueRange;
          scene.updateValueSelector(transferFunction, isoValues);
          glfwVKLWindow->resetAccumulation();
        };

    static TransferFunctionWidget transferFunctionWidget(
        transferFunctionUpdatedCallback, initialValueRange);
    transferFunctionWidget.updateUI();

    if (changed) {
      glfwVKLWindow->resetAccumulation();
    }
  });

  glfwVKLWindow->registerEndOfFrameCallback([&]() {
    auto vdbVolume = dynamic_cast<OpenVdbVolume *>(testingVolume.get());
    if (vdbVolume && vdbVolume->updateVolume(leafAccessObserver)) {
      scene.updateVolume(vdbVolume->getVKLVolume());

      vklSetInt(scene.sampler, "filter", filter);
      vklSetInt(scene.sampler, "gradientFilter", gradientFilter);
      vklSetInt(scene.sampler, "maxSamplingDepth", maxSamplingDepth);
      vklSetInt(scene.sampler, "maxIteratorDepth", maxIteratorDepth);
      vklCommit(scene.sampler);
      scene.updateValueSelector(transferFunction, isoValues);

      if (leafAccessObserver)
        vklRelease(leafAccessObserver);
      leafAccessObserver = vdbVolume->newLeafAccessObserver(scene.sampler);

      glfwVKLWindow->resetAccumulation();
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwVKLWindow->mainLoop();

  // cleanly shut VKL down
  scene = Scene();
  if (leafAccessObserver)
    vklRelease(leafAccessObserver);

  testingVolume.reset();
  glfwVKLWindow.reset();

  vklShutdown();

  return 0;
}
