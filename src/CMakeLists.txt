# Executables

# Main program
#add_executable(main src/main.cpp
#  src/camera.cpp
#  src/buffer_util.cpp
#  common/shader.cpp
#  common/texture.cpp
#  common/objloader.cpp
#)

#target_link_libraries(main ${ALL_LIBS})

# Dual test program
#add_executable(dual src/dual.cpp
#  src/camera.cpp
#  src/mesh.cpp
#  src/buffer_util.cpp
#  common/shader.cpp
#  common/texture.cpp
#)
#target_link_libraries(dual ${ALL_LIBS} assimp)

# Convert pcd to ply for visualization
#add_executable(pcd2ply src/pcd2ply.cpp)
#target_link_libraries(pcd2ply ${PCL_COMMON_LIBRARIES} ${PCL_IO_LIBRARIES})

#add_executable(test_anim src/test_anim)
#target_link_libraries(test_anim assimp)

add_executable(v3 v3.cpp
  camera.cpp
  mesh.cpp
  buffer_util.cpp
  shader.cpp
  ../common/texture.cpp
)
target_link_libraries(v3 ${ALL_LIBS} assimp glfx)

add_executable(v4 v4.cpp
  camera.cpp
  mesh.cpp
  buffer_util.cpp
  shader.cpp
  BvhLoader.cpp
  ../common/texture.cpp
)
target_link_libraries(v4 ${ALL_LIBS} assimp glfx)

#add_executable(tm src/test_mesh.cpp src/mesh.cpp)
#target_link_libraries(tm assimp ${ALL_LIBS})

add_executable(test test.cpp BvhLoader.cpp)
target_link_libraries(test ${ALL_LIBS})

add_executable(dvb dae_vs_bvh.cpp)
target_link_libraries(dvb assimp)
