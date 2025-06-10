build-image:
	@bash -x scripts/docker_image_build.sh

build-all: build-image
	@bash -x scripts/run.sh

unit-tests: build-all
	-@py.test -sx -vv ./tests/unit --cov=./hyperon_das/ --cov-config=.coveragerc --cov-report=term-missing --cov-fail-under=70; ret=$$?; $(MAKE) clean; exit $$ret

clean:
	find . -type d -name "__pycache__" -prune -exec rm -rf {} +
	rm -Rf ./pytest_cache
	rm -Rf ./dist
	rm -Rf ./hyperon_das/_grpc
