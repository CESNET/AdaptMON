BUILD_TYPE=$(or $(file < build/BUILD_TYPE),release-build)

all: build
.PHONY: all

build: $(BUILD_TYPE)
.PHONY: build

release-build:
	mkdir -p build && cd build && echo $@ > BUILD_TYPE && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(shell nproc)
.PHONY: release-build

debug-build:
	mkdir -p build && cd build && echo $@ > BUILD_TYPE && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j $(shell nproc)
.PHONY: debug-build

debug-build-strict:
	mkdir -p build && cd build && echo $@ > BUILD_TYPE && env CFLAGS=-Werror CXXFLAGS=-Werror cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j $(shell nproc)
.PHONY: debug-build-strict

clean:
	rm -rf build
.PHONY: clean

clean-build: clean build
.PHONY: clean-build

format:
	find lib/ apps/ -type f -regex '.*\.\(cpp\|hpp\|c\|h\)' -print0 | xargs -0 clang-format --dry-run --Werror
.PHONY: format

format-fix:
	find lib/ apps/ -type f -regex '.*\.\(cpp\|hpp\|c\|h\)' -print0 | xargs -0 clang-format -i --Werror
.PHONY: format-fix

tidy:
	mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
	run-clang-tidy -p build -quiet -j $(shell nproc) -header-filter "$(shell pwd)/(apps|lib)" lib/ apps/
.PHONY: tidy

tidy-fix:
	mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
	run-clang-tidy -p build -quiet -fix -j $(shell nproc) -header-filter "$(shell pwd)/(apps|lib)" lib/ apps/
.PHONY: tidy-fix

doxygen:
	mkdir -p build && cd build && cmake -DENABLE_DOXYGEN=On -DCMAKE_BUILD_TYPE=Debug .. && make doxygen
.PHONY: doxygen

doxygen-strict:
	mkdir -p build && cd build && cmake -DENABLE_DOXYGEN=On -DCMAKE_BUILD_TYPE=Debug -DDOXYGEN_WARN_AS_ERROR=YES .. && make doxygen
.PHONY: doxygen-strict

editorconfig:
	docker run --rm -v $(shell pwd):/check mstruebing/editorconfig-checker
.PHONY: editorconfig
