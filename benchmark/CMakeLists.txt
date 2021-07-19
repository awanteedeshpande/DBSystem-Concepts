add_executable(milestone1_bench milestone1.cpp $<TARGET_OBJECTS:dbsys20>)
target_link_libraries(milestone1_bench PRIVATE mutable)

add_executable(milestone2_bench milestone2.cpp $<TARGET_OBJECTS:dbsys20>)
target_link_libraries(milestone2_bench PRIVATE mutable)

add_executable(milestone3_bench milestone3.cpp $<TARGET_OBJECTS:dbsys20>)
target_link_libraries(milestone3_bench PRIVATE mutable)
