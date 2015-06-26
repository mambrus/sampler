1. Prerequisites

SAMPLER depends on some external libraries written in a "shared way". I.e.
they are both developed and used in several projects simultaneously.
Currently the build-system doesn't support autoconf/automake and it can't
detect the difference between if you already have one of these libraries
build and installed, and assumes it's in source in form of links to
the each libs sub-folder (if any) capable of building each lib with either a
Makefile or Android.mk (both needed for full transparency) of it's own.

I.e. to prepare the build, for each of the projects mentioned below (see last
section for list):

1) Get the source
2) preferably place it side-by-side with SAMPLER

and:

cd $YOUR_SAMPLER_DIR
ln -s ../therproj/libthelib

2. Build
Builds should be done in-source in a corresponding environment. For each
supported environment there is a specific environment file: '.env<type>'.
It's not strictly necessary to source them as each build should use safe
fall-backs, but as all possible combinations are not fully tested, please
source each environment file per terminal once prior each build or
clean-build.

Hint: Check that all components are properly in place by building naively
first as there's a slightly better dependency-checker for native builds than
for Adroid NDK builds. Non-root build will not install in your system and
outputs don't collide as they are build in separate build-trees. I.e.
different builds shouldn't interfere with each other. I.e. you don't need to
build-clean in-between.

2.1 Native
 . .env
 (Optionally '. .envr' for build as root and system-install)

 2.1.1 Normal build
 make

2.1.2. Clean build
 make clean

2.2 Android
. .envx

2.1.1 Normal build
 make android

2.1.2. Clean build
 make android-clean

3. List of external libs needed in-source:
Suggested origin given in parenthesis

* libmqueue (https://github.com/mambrus/clib-mqueue)

If you get the following error "sys/capability.h: No such file or
directory" then there is missing dependency. Can be solved with:
$ sudo aptitude install libcap-dev
