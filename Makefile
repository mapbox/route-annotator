default: all

BUILD_TYPE ?= Release
BUILD_DIR := build/${BUILD_TYPE}
ROOT_DIR := $(shell pwd)

third_party:
	@./deps.sh

${BUILD_DIR}/Makefile: CMakeLists.txt
	@mkdir -p ${BUILD_DIR}
	@cd ${BUILD_DIR} && cmake ${ROOT_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

all: ${BUILD_DIR}/Makefile third_party
	@cmake --build ${BUILD_DIR}

test: ${BUILD_DIR}/Makefile
	@cmake --build ${BUILD_DIR} --target all-tests
	${BUILD_DIR}/test/basic-tests

watch:
	while inotifywait -e modify CMakeLists.txt Makefile *.cpp; do make; done

clean:
	@${RM} -rf ${BUILD_DIR}

.PHONE: all watch clean
