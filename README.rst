ebglvis - OpenGL 3D Visualization for Event Based data
======================================================


Requirements
------------
The application depends on the following 3rd party tools and libraries. It may
be able to build it with version lower than the ones mentioned below

* g++ 4.8.4 / clang++ 3.5.0
* cmake >= 2.8
* boost >= 1.55


Build Instructions
------------------

1. if you have only cloned the repository, make sure that you have the
   submodules initialized, i.e.

        $ git submodule init

        $ git submodule update

2. go the the edvstools directory and make an out of source build in the
   directory "build", i.e.

        $ cd edvstools

        $ mkdir build

        $ cd build

        $ cmake ..

        $ make

3. go back to the ebglvis directory, and build it, for instance out of source in
   the directory build

        $ mkdir build

        $ cd build

        $ cmake ..

        $ make
