# Set the minimum version required for cmake. The version 4.0 was 
# chosen as the required version for Allpix squared compatibility
cmake_minimum_required(VERSION 3.26 FATAL_ERROR)
#set(CMAKE_SKIP_RPATH ON)
# Define the project with the name Tutorial.
project(Tutorial)
# Locate and configure Geant4. Required == Fatal Error if not 
# found (good test for Geant4 initialization)
# additionally require visualization
find_package(Geant4 REQUIRED ui_all vis_all)
# GEANT4_USE_FILE is a variable defined by the find_package that points to the file
# where GEANT was found. Including it sets up environment variables and other basic
include(${Geant4_USE_FILE})

# ── ROOT ──────────────────────────────────────────────────────────────────────
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree Hist)
include(${ROOT_USE_FILE})   # sets up include paths, compile flags, etc.



# Tells compiler where to find project-specific header files during compilation
include_directories(${PROJECT_SOURCE_DIR}/include)
# glob function to find all .cc source files in the src directory and puts it in
# the sources variable. No need to list all source files by hand
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
# Remove this line because it requires a CMake level ROOT sourcing which is not yet compatible.
list(REMOVE_ITEM sources ${PROJECT_SOURCE_DIR}/src/Create2DEffMap.cc)
# Copy all te macro files in the macros directory
file(GLOB MACRO_FILES "macros/*.mac")
file(COPY ${MACRO_FILES} DESTINATION ${CMAKE_BINARY_DIR}/)
file(GLOB CONDOR_FILES "run_files/*")
file(COPY ${CONDOR_FILES} DESTINATION ${CMAKE_BINARY_DIR}/)
# Create executable called sim that compilse sim.cc and all source files from sources
# sim.cc is the main file with main() function
add_executable(sim sim.cc ${sources})
# Links the sim executable to the libraries listed in the varible Geant4_LIBRARIES
# added customizable functionality to the cmake. For vlad's machine it needs to be ran as 
#cmake -DEXTRA_LIBS=uuid ..
target_link_libraries(sim ${Geant4_LIBRARIES} ${EXTRA_LIBS})

# Needs explicit link to libuuid library
# Define target called Tutorial that depends on sim. Running make Tutorial builds sim
add_custom_target(Tutorial DEPENDS sim)

################################## ANALYSIS ##################################

# ── Shared analysis library ─────────────────────────────────────────────────
# These are helpers with no top-level entry point
set(lib_sources
    analysis_multiPlane/src/ConfigAnalysis.cc
    analysis_multiPlane/src/Utils.cc
    analysis_multiPlane/src/RootIO.cc
    analysis_multiPlane/src/DigitalUtils.cc
    analysis_multiPlane/src/TrackingUtils.cc
    analysis_multiPlane/src/ClusteringUtils.cc
    analysis_multiPlane/src/AnalysisUtils.cc
    analysis_multiPlane/src/CalorimetryUtils.cc
)

add_library(analysis_lib STATIC ${lib_sources})
target_include_directories(analysis_lib PUBLIC analysis_multiPlane/include)
target_link_libraries(analysis_lib ${ROOT_LIBRARIES})

# ── One executable per pipeline step ────────────────────────────────────────
# Each main_*.cc just parses argv and calls the corresponding function
set(steps Analysis Calorimetry Clustering DigitalProcessing PRIOFIFOHWCProcessing Tracking)

foreach(step ${steps})
    add_executable(run_${step}
        analysis_multiPlane/main/main_${step}.cc
        analysis_multiPlane/src/${step}_multiPlane.cc  # the actual logic
    )
    target_link_libraries(run_${step} analysis_lib)
endforeach()

# ── Tests ────────────────────────────────────────────────────────────────────
add_executable(test_thresholdMap
    analysis_multiPlane/tests/test_thresholdMap.cc
)
target_link_libraries(test_thresholdMap analysis_lib)

# ── Examples ────────────────────────────────────────────────────────────────────
add_executable(malta2Digitizer
    analysis_multiPlane/examples/MALTA2Digitizer.cc
)
target_link_libraries(malta2Digitizer analysis_lib)





