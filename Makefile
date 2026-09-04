BUILD_DIR := $(or $(BUILD_DIR),build)
BUILD_TYPE = $(or $(file < $(BUILD_DIR)/BUILD_TYPE),release-build)

all: build
.PHONY: all

build: $(BUILD_TYPE)
.PHONY: build

release-build:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && echo $@ > BUILD_TYPE && cmake -DCMAKE_BUILD_TYPE=Release $(shell pwd) && make -j $(shell nproc)
.PHONY: release-build

debug-build:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && echo $@ > BUILD_TYPE && cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=1 -DENABLE_SANITIZERS=1 -DENABLE_COVERAGE=0 $(shell pwd) && make -j $(shell nproc)
.PHONY: debug-build

debug-build-strict:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && echo $@ > BUILD_TYPE && env CFLAGS=-Werror CXXFLAGS=-Werror cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=1 -DENABLE_SANITIZERS=1 -DENABLE_COVERAGE=0 $(shell pwd) && make -j $(shell nproc)
.PHONY: debug-build-strict

test: $(if $(filter debug-build%,$(BUILD_TYPE)),$(BUILD_TYPE),debug-build)
	cd build && make test
.PHONY: test

coverage-build:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && echo $@ > BUILD_TYPE && cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=1 -DENABLE_SANITIZERS=1 -DENABLE_COVERAGE=1 $(shell pwd) && make -j $(shell nproc)
.PHONY: coverage-build

coverage: coverage-build
	find $(BUILD_DIR) -name '*.gcda' -delete
	cd $(BUILD_DIR) && make test
	mkdir -p $(BUILD_DIR)/coverage-html
	uv tool run gcovr --root . $(BUILD_DIR) \
		--filter 'lib/' \
		--exclude 'lib/src/TestWithMain.cpp' \
		--exclude-function '/.*_Test.*/' \
		--cobertura $(BUILD_DIR)/coverage.xml \
		--html-details $(BUILD_DIR)/coverage-html/index.html \
		--txt
.PHONY: coverage

clean:
	rm -rf $(BUILD_DIR)
.PHONY: clean

clean-build: clean build
.PHONY: clean-build

format:
	find lib/ apps/ -type f -regex '.*\.\(cpp\|hpp\|c\|h\)' -print0 | xargs -0 clang-format --dry-run --Werror
.PHONY: format

format-fix:
	find lib/ apps/ -type f -regex '.*\.\(cpp\|hpp\|c\|h\)' -print0 | xargs -0 clang-format -i --Werror
.PHONY: format-fix

compile-commands:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(shell pwd)
.PHONY: compile-commands

tidy:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_TESTS=OFF $(shell pwd)
	run-clang-tidy -p $(BUILD_DIR) -quiet -j $(shell nproc) -header-filter "$(shell pwd)/(apps|lib)" lib/ apps/
.PHONY: tidy

tidy-fix:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_TESTS=OFF $(shell pwd)
	run-clang-tidy -p $(BUILD_DIR) -quiet -fix -j $(shell nproc) -header-filter "$(shell pwd)/(apps|lib)" lib/ apps/
.PHONY: tidy-fix

format-cmake:
	tools/format-cmake --check
.PHONY: format-cmake

format-cmake-fix:
	tools/format-cmake
.PHONY: format-cmake-fix

doxygen:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake -DENABLE_DOXYGEN=On -DCMAKE_BUILD_TYPE=Debug $(shell pwd) && make doxygen
.PHONY: doxygen

doxygen-strict:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake -DENABLE_DOXYGEN=On -DCMAKE_BUILD_TYPE=Debug -DDOXYGEN_WARN_AS_ERROR=FAIL_ON_WARNINGS $(shell pwd) && make doxygen
.PHONY: doxygen-strict

editorconfig:
	editorconfig-checker
.PHONY: editorconfig

dev-container:
	docker build -f docker/Dockerfile.gitlab-ci -t adaptmon-dev-container \
		--build-arg USERID=$(shell id -u) --build-arg GROUPID=$(shell id -g) \
		--build-arg USER=$(shell whoami) --build-arg GROUP=$(shell getent group "$$(id -u)" | cut -d: -f1 ) \
		--build-arg OS_DISTRO=$(or ${OS_DISTRO},oraclelinux) --build-arg OS_RELEASE=$(or ${OS_RELEASE},10) \
		.
	@echo "Hint:"
	@echo "  docker run --rm -it -v .:/adaptmon --workdir /adaptmon adaptmon-dev-container /bin/bash"
.PHONY: dev-docker

enter-dev-container: dev-container
	docker run --rm -it -v .:/adaptmon --workdir /adaptmon adaptmon-dev-container /bin/bash
.PHONY: enter-dev-container

commitlint:
	if [ -n "$${CI_MERGE_REQUEST_DIFF_BASE_SHA}" ] && [ -n "${CI_COMMIT_SHA}" ]; then \
		BASE="$${CI_MERGE_REQUEST_DIFF_BASE_SHA}"; \
		HEAD="$${CI_COMMIT_SHA}"; \
	else \
		BASE="$$(git log --pretty=format:"%H%d" | grep -E "^\w+ " | tail -n+2 | head -n1 | cut -d" " -f1)"; \
		HEAD="$$(git rev-parse HEAD)"; \
	fi; \
	docker run --rm -v .:/w --workdir /w --entrypoint /bin/sh registry.hub.docker.com/commitlint/commitlint:latest -c \
		"git config --global --add safe.directory /w && commitlint --from=$$BASE --to=$$HEAD"
.PHONY: commitlint

atomic-commits:
	if [ -n "$${CI_MERGE_REQUEST_DIFF_BASE_SHA}" ] && [ -n "${CI_COMMIT_SHA}" ]; then \
		BASE="$${CI_MERGE_REQUEST_DIFF_BASE_SHA}"; \
		HEAD="$${CI_COMMIT_SHA}"; \
	else \
		BASE="$$(git log --pretty=format:"%H%d" | grep -E "^\w+ " | tail -n+2 | head -n1 | cut -d" " -f1)"; \
		HEAD="$$(git rev-parse HEAD)"; \
	fi; \
	branch="$$(git branch --show-current)"; \
	git stash store "$$(git stash create)" && did_stash=1 && git reset --hard HEAD; \
	git rev-list --reverse $${BASE}..$${HEAD} | xargs -I{} sh -c "set -e; echo; echo @@@@@@@@@@@@@@@@@@@@ {} @@@@@@@@@@@@@@@@@@@@; echo; git checkout {}; make; echo"; \
	EC=$$?; \
	if [ -n "$$branch" ]; then git checkout "$$branch"; fi; \
	if [ -n "$$did_stash" ]; then git stash pop; fi; \
	exit $${EC}
.PHONY: atomic-commits

pragma-once:
	@tools/checks/check-pragma-once
.PHONY: pragma-once

custom-checks:
	@rc=0; \
	GREEN='\033[0;32m'; \
	RED='\033[0;31m'; \
	YELLOW='\033[1;33m'; \
	NC='\033[0m'; \
	for script in tools/checks/*; do \
		if [ -x "$$script" ]; then \
			printf "$${YELLOW}==> Running %s$${NC}\n" "$$script"; \
			if "$$script"; then \
				printf "$${GREEN}✔ PASS %s$${NC}\n\n" "$$script"; \
			else \
				printf "$${RED}✖ FAIL %s$${NC}\n\n" "$$script"; \
				rc=1; \
			fi; \
		fi; \
	done; \
	exit $$rc
.PHONY: custom-checks

# don't be afraid to add Python tests for your applications to this target
pytest: build
	cd apps/amon-anonymizer && uv run pytest -v
.PHONY: pytest
