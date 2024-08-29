build-nanobind: clean
	@rm -f CMakeLists.txt cxx/handletrie_nanobind*.so
	@ln -s CMakeListsNanoBind.txt CMakeLists.txt
	@cd build && cmake .. && make && cp *.so ../cxx/

build-pybind: clean
	@rm -f CMakeLists.txt cxx/handletrie_pybind*.so
	@ln -s CMakeListsPyBind.txt CMakeLists.txt 
	@cd build && cmake .. && make && cp *.so ../cxx/


build-cpython: clean
	@rm -f cxx/handletrie_cpython*.so
	@python setup.py build_ext --inplace && mv *.so cxx/

build: clean
	@rm -f CMakeLists.txt
	@ln -s CMakeListsCXX.txt CMakeLists.txt
	@cd build && cmake .. && make

clean:
	@rm -rf build/*
	@rm -rf cpython/*
