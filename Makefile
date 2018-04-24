MODULE_NAME := $(shell node -e "console.log(require('./package.json').binary.module_name)")

default: release

node_modules:
	# install deps but for now ignore our own install script
	# so that we can run it directly in either debug or release
	npm install --ignore-scripts

release: node_modules
	V=1 ./node_modules/.bin/node-pre-gyp configure build --loglevel=error
	@echo "run 'make clean' for full rebuild"

debug: node_modules
	V=1 ./node_modules/.bin/node-pre-gyp configure build --loglevel=error --debug
	@echo "run 'make clean' for full rebuild"

coverage:
	./scripts/coverage.sh

clean:
	rm -rf lib/binding
	rm -rf build
	@echo "run 'make distclean' to also clear node_modules, mason_packages, and .mason directories"

distclean: clean
	rm -rf node_modules
	rm -rf mason_packages
	rm -rf .mason

xcode: node_modules
	./node_modules/.bin/node-pre-gyp configure -- -f xcode

	@# If you need more targets, e.g. to run other npm scripts, duplicate the last line and change NPM_ARGUMENT
	SCHEME_NAME="$(MODULE_NAME)" SCHEME_TYPE=library BLUEPRINT_NAME=$(MODULE_NAME) BUILDABLE_NAME=$(MODULE_NAME).node scripts/create_scheme.sh
	SCHEME_NAME="npm test" SCHEME_TYPE=node BLUEPRINT_NAME=$(MODULE_NAME) BUILDABLE_NAME=$(MODULE_NAME).node NODE_ARGUMENT="`npm bin tape`/tape test/*.test.js" scripts/create_scheme.sh

	open build/binding.xcodeproj

docs:
	npm run docs

unit-test-release:
	@echo "Running CXX tests..."
	./build/Release/basic-tests -x
	./build/Release/congestion-tests -x

unit-test-debug:
	@echo "Running CXX tests..."
	./build/Debug/basic-tests
	./build/Debug/congestion-tests

npmtest:
	@echo "Running nodejs tests..."
	npm test

test-release: release unit-test-release npmtest

test-debug: debug unit-test-debug npmtest

test: test-release

.PHONY: test docs
