This project contains the work done during the course on Database Systems at Saarland University in the Winter Semester 2020-2021. <br/>

The skeletal framework for the project, including the build toold and unit tests have already been provided by the course organisers. This also includes the Mutable API (https://bigdata.uni-saarland.de/teaching/dbsys20/api/) for working with the Database System. Our own work here involves actually implementing the concepts stated below in the respective source files. <br/>
To build the project, the requirements include CMake3 3.0 or newer, a C/C++ compiler supporting C++17 (such as Clang4 or GCC5), and a build automation tool (e.g. GNU Make6 or Ninja7). Source files are located `src/` in
and files for unit testing are found in `unittest/`.
<br/><br/>
This project implements 3 important concepts of a database system: <br/>
1. Row and Column Storage Layout: <br/>
In the main memory database store, the row and column layout map how the relation tuples are physically stored on the storage.
2. B+ Tree: <br/>
This contains the general data structure for a B+ Tree with functionlity like getters and setters, iterators, lookup, and bulkloading.
3. Query Optimiser: <br/>
Here, an algorithm for cost-based query optimisation has been implemented. The code has been dissected into three components - a table to track the optimal plan for each subproblem, a cost function to estimate the costs of a plan, and an algorithm to enumerate all feasible plans.
<br/><br/>