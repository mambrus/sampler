README SAMPLER
==============

Gather and process system data from a POSIX system.

Building:
---------

Get documentation below first. Build depends on source-file in
documentation to generate manpages. Build-system is CMake-based. Please read
more about it in the wiki.

Documentation:
--------------

All documentation is a gitit wiki. Start reading in these simple steps:

1) Get all sub-modules - documentation is one of them

    git submodule init
    git submodule update

2) Install gitit if you don't have it already:

http://gitit.net/Installing

3) Execute the following:

    cd doc
    ./start_wiki.sh

4) Open browser and go to:

[http://localhost:5021](http://localhost:5021)

Cloning project recursively:
----------------------------

To clone project including it's submodules you can for example do as follows
(replace the server if cloning from some other repository):

    git clone --recursive https://github.com/mambrus/sampler.git

In this case #1 under "Documentation" can be omitted.


