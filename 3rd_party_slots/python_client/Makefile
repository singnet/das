build-image:
	@bash -x scripts/docker_image_build.sh

build: build-image
	@bash -x scripts/run.sh

unit-tests: build
	-@py.test -sx -vv ./tests/unit --cov=./hyperon_das/ --cov-config=.coveragerc --cov-report=term-missing --cov-fail-under=70

integration-tests: build
	-@py.test -sx -vv ./tests/integration --cov=./hyperon_das/ --cov-config=.coveragerc --cov-report=term-missing

clean:
	find . -type d -name "__pycache__" -prune -exec rm -rf {} +
	rm -Rf ./pytest_cache
	rm -Rf ./dist
	rm -Rf ./hyperon_das/_grpc

isort:
	@isort --settings-path .isort.cfg ./hyperon_das ./tests

black:
	@black --config .black.cfg ./hyperon_das ./tests

flake8:
	@flake8 --config .flake8.cfg ./hyperon_das ./tests

lint: isort black flake8
