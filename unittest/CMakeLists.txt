add_executable(
    unittest
    main.cpp
    BPlusTreeTest.cpp
    ColumnStoreTest.cpp
    MyPlanEnumeratorTest.cpp
    RowStoreTest.cpp
)
target_link_libraries(unittest $<TARGET_OBJECTS:dbsys20> mutable)
