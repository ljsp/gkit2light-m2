cmake_minimum_required(VERSION 3.10)

# set the project name
project(gkit2light)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# gkit files
file(GLOB gkit_files "src/gKit/*.cpp")

#imgui files
file(GLOB imgui_files "src/imgui/*.cpp")

# add the executable
add_executable(tuto7_camera "tutos/tuto7_camera.cpp" ${gkit_files} ${imgui_files})
target_include_directories(tuto7_camera PUBLIC "src/gKit" "src/imgui")
target_link_libraries(tuto7_camera GL GLEW SDL2 SDL2_image)
set_target_properties(tuto7_camera PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(tuto7_camera PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(tuto_decal "tutos/tuto_decal.cpp" ${gkit_files} ${imgui_files})
target_include_directories(tuto_decal PUBLIC "src/gKit" "src/imgui")
target_link_libraries(tuto_decal GL GLEW SDL2 SDL2_image)
set_target_properties(tuto_decal PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(tuto_decal PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")


add_executable(tuto_shadows "tutos/tuto_shadows.cpp" ${gkit_files} ${imgui_files})
target_include_directories(tuto_shadows PUBLIC "src/gKit" "src/imgui")
target_link_libraries(tuto_shadows GL GLEW SDL2 SDL2_image)
set_target_properties(tuto_shadows PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(tuto_shadows PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(tp1 "src/tp/tp1.cpp" ${gkit_files} ${imgui_files})
target_include_directories(tp1 PUBLIC "src/gKit" "src/imgui")
target_link_libraries(tp1 GL GLEW SDL2 SDL2_image)
set_target_properties(tp1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(tp1 PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(tp2 "src/tp/tp2.cpp" ${gkit_files} ${imgui_files})
target_include_directories(tp2 PUBLIC "src/gKit" "src/imgui")
target_link_libraries(tp2 GL GLEW SDL2 SDL2_image)
set_target_properties(tp2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(tp2 PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(tp3 "src/tp/tp3.cpp" ${gkit_files} ${imgui_files})
target_include_directories(tp3 PUBLIC "src/gKit" "src/imgui")
target_link_libraries(tp3 GL GLEW SDL2 SDL2_image)
set_target_properties(tp3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(tp3 PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(shader_kit "src/shader_kit.cpp" ${gkit_files} ${imgui_files})
target_include_directories(shader_kit PUBLIC "src/gKit" "src/imgui")
target_link_libraries(shader_kit GL GLEW SDL2 SDL2_image)
set_target_properties(shader_kit PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(shader_kit PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")


