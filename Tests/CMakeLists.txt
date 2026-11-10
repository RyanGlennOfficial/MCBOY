add_executable(mcboy_core_smoke CoreSmoke.cpp)
target_compile_features(mcboy_core_smoke PRIVATE cxx_std_17)
if (MSVC)
 target_compile_options(mcboy_core_smoke PRIVATE /W4)
else()
 target_compile_options(mcboy_core_smoke PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME CoreSmoke COMMAND mcboy_core_smoke)

add_executable(mcboy_audio_stress AudioStress.cpp)
target_compile_features(mcboy_audio_stress PRIVATE cxx_std_17)
add_test(NAME AudioStress COMMAND mcboy_audio_stress)

add_executable(mcboy_uniqueness Uniqueness.cpp)
target_compile_features(mcboy_uniqueness PRIVATE cxx_std_17)
add_test(NAME All100Uniqueness COMMAND mcboy_uniqueness)

add_executable(mcboy_render_demo RenderDemo.cpp)
target_compile_features(mcboy_render_demo PRIVATE cxx_std_17)
