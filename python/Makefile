TARGETS=commons

shell:
	@poetry shell

install:
	@poetry install

isort:
	@echo "Running isort"
	@poetry run isort $(TARGETS)

black:
	@echo "Running black"
	@poetry run black $(TARGETS)

flake8:
	@echo "Running flake8"
	@poetry run flake8 $(TARGETS)

mypy:
	@echo "Running mypy"
	@poetry run mypy $(TARGETS)

pylint:
	@echo "Running pylint"
	@poetry run pylint $(TARGETS)

lint: isort black flake8 pylint mypy
