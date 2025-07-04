# Set the minimum version required for cmake. The version 4.0 was 
# chosen as the required version for Allpix squared compatibility
cmake_minimum_required(VERSION 3.26 FATAL_ERROR)
# Define the project with the name Tutorial.
project(Tutorial)
# Locate and configure Geant4. Required == Fatal Error if not 
# found (good test for Geant4 initialization)
# additionally require visualization
find_package(Geant4 REQUIRED ui_all vis_all)
# GEANT4_USE_FILE is a variable defined by the find_package that points to the file
# where GEANT was found. Including it sets up environment variables and other basic
include(${Geant4_USE_FILE})
# Tells compiler where to find project-specific header files during compilation
include_directories(${PROJECT_SOURCE_DIR}/include)
# glob function to find all .cc source files in the src directory and puts it in
# the sources variable. No need to list all source files by hand
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
# Copy all te macro files in the macros directory
file(GLOB MACRO_FILES "macros/*.mac")
file(COPY ${MACRO_FILES} DESTINATION ${CMAKE_BINARY_DIR}/)
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
