TARGETS=commons
TEST_TARGETS=test_commons
LINT_TARGETS=$(TARGETS) $(TEST_TARGETS)

shell:
	@poetry shell

install:
	@poetry install

isort:
	@echo "Running isort"
	@poetry run isort $(LINT_TARGETS)

black:
	@echo "Running black"
	@poetry run black $(LINT_TARGETS)

flake8:
	@echo "Running flake8"
	@poetry run flake8 $(LINT_TARGETS)

mypy:
	@echo "Running mypy"
	@poetry run mypy $(LINT_TARGETS)

pylint:
	@echo "Running pylint"
	@poetry run pylint $(LINT_TARGETS)

lint: isort black flake8 pylint mypy

unit-tests:
	@echo "Running unit tests"
	@poetry run pytest $(TEST_TARGETS)

pytest: unit-tests
