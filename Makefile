# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.7

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/tacticalmelonfarmer/Desktop/ideas/events

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tacticalmelonfarmer/Desktop/ideas/events

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake cache editor..."
	/usr/bin/cmake-gui -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/tacticalmelonfarmer/Desktop/ideas/events/CMakeFiles /home/tacticalmelonfarmer/Desktop/ideas/events/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/tacticalmelonfarmer/Desktop/ideas/events/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named typelist_test1

# Build rule for target.
typelist_test1: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 typelist_test1
.PHONY : typelist_test1

# fast build rule for target.
typelist_test1/fast:
	$(MAKE) -f CMakeFiles/typelist_test1.dir/build.make CMakeFiles/typelist_test1.dir/build
.PHONY : typelist_test1/fast

#=============================================================================
# Target rules for targets named variant

# Build rule for target.
variant: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 variant
.PHONY : variant

# fast build rule for target.
variant/fast:
	$(MAKE) -f CMakeFiles/variant.dir/build.make CMakeFiles/variant.dir/build
.PHONY : variant/fast

#=============================================================================
# Target rules for targets named test1

# Build rule for target.
test1: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 test1
.PHONY : test1

# fast build rule for target.
test1/fast:
	$(MAKE) -f CMakeFiles/test1.dir/build.make CMakeFiles/test1.dir/build
.PHONY : test1/fast

tests/test1.o: tests/test1.cpp.o

.PHONY : tests/test1.o

# target to build an object file
tests/test1.cpp.o:
	$(MAKE) -f CMakeFiles/test1.dir/build.make CMakeFiles/test1.dir/tests/test1.cpp.o
.PHONY : tests/test1.cpp.o

tests/test1.i: tests/test1.cpp.i

.PHONY : tests/test1.i

# target to preprocess a source file
tests/test1.cpp.i:
	$(MAKE) -f CMakeFiles/test1.dir/build.make CMakeFiles/test1.dir/tests/test1.cpp.i
.PHONY : tests/test1.cpp.i

tests/test1.s: tests/test1.cpp.s

.PHONY : tests/test1.s

# target to generate assembly for a file
tests/test1.cpp.s:
	$(MAKE) -f CMakeFiles/test1.dir/build.make CMakeFiles/test1.dir/tests/test1.cpp.s
.PHONY : tests/test1.cpp.s

tests/typelist_test1.o: tests/typelist_test1.cpp.o

.PHONY : tests/typelist_test1.o

# target to build an object file
tests/typelist_test1.cpp.o:
	$(MAKE) -f CMakeFiles/typelist_test1.dir/build.make CMakeFiles/typelist_test1.dir/tests/typelist_test1.cpp.o
.PHONY : tests/typelist_test1.cpp.o

tests/typelist_test1.i: tests/typelist_test1.cpp.i

.PHONY : tests/typelist_test1.i

# target to preprocess a source file
tests/typelist_test1.cpp.i:
	$(MAKE) -f CMakeFiles/typelist_test1.dir/build.make CMakeFiles/typelist_test1.dir/tests/typelist_test1.cpp.i
.PHONY : tests/typelist_test1.cpp.i

tests/typelist_test1.s: tests/typelist_test1.cpp.s

.PHONY : tests/typelist_test1.s

# target to generate assembly for a file
tests/typelist_test1.cpp.s:
	$(MAKE) -f CMakeFiles/typelist_test1.dir/build.make CMakeFiles/typelist_test1.dir/tests/typelist_test1.cpp.s
.PHONY : tests/typelist_test1.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... rebuild_cache"
	@echo "... edit_cache"
	@echo "... typelist_test1"
	@echo "... variant"
	@echo "... test1"
	@echo "... tests/test1.o"
	@echo "... tests/test1.i"
	@echo "... tests/test1.s"
	@echo "... tests/typelist_test1.o"
	@echo "... tests/typelist_test1.i"
	@echo "... tests/typelist_test1.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

