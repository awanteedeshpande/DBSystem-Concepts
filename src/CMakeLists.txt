add_library(
    dbsys20
    OBJECT
    ColumnStore.cpp
    MyPlanEnumerator.cpp
    RowStore.cpp
)
add_dependencies(dbsys20 Mutable)

add_executable(milestone1 milestone1.cpp $<TARGET_OBJECTS:dbsys20>)
target_link_libraries(milestone1 PRIVATE mutable)

add_executable(milestone2 milestone2.cpp $<TARGET_OBJECTS:dbsys20>)
target_link_libraries(milestone2 PRIVATE mutable)

add_executable(milestone3 milestone3.cpp $<TARGET_OBJECTS:dbsys20>)
target_link_libraries(milestone3 PRIVATE mutable dl)
