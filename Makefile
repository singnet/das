# release: build-deployment
# 	@docker run --rm -it -v ${HOME}/.ssh:/root/.ssh:ro -v $(CURDIR)/scripts:/opt/scripts das-deployment:latest /opt/scripts/deployment/release.sh

# deploy: build-deployment
# 	@docker run --rm -it -v ${HOME}/.ssh:/root/.ssh:ro -v $(CURDIR)/scripts:/opt/scripts das-deployment:latest /opt/scripts/deployment/fn_deploy.sh

release:
	@bash $(CURDIR)/scripts/deployment/release.sh

deploy:
	@bash $(CURDIR)/scripts/deployment/fn_deploy.sh

integration-tests:
	@bash $(CURDIR)/scripts/deployment/tests.sh

build-deployment:
	@docker build -f .docker/deployment/Dockerfile -t das-deployment:latest .

build-semver:
	@docker image build -t trueagi/das:semantic-versioning ./scripts/semver

publish-semver:
	@docker image push trueagi/das:semantic-versioning

github-runner:
	@bash $(CURDIR)/scripts/github-runner/main.sh

build-image:
	@bash -x src/scripts/docker_image_build.sh

build-mork-image:
	@bash -x src/scripts/docker_image_build_mork.sh

build-all: build-image
	@bash -x src/scripts/build.sh

run-query-agent:
	@PORT=$$(bash src/scripts/gkctl_auto_join_and_reserve.sh | tail -n 1); \
	PORT_RANGE=$$(bash src/scripts/gkctl_auto_join_and_reserve.sh --range=999 | tail -n 1); \
	bash -x src/scripts/run.sh query_broker $$PORT $$PORT_RANGE

run-attention-broker:
	@bash -x src/scripts/run.sh attention_broker_service 37007

run-link-creation-agent:
	@bash -x src/scripts/run.sh link_creation_server $(OPTIONS)

run-link-creation-client:
	@bash -x src/scripts/run.sh link_creation_agent_client $(OPTIONS)

run-inference-agent:
	@bash -x src/scripts/run.sh inference_agent_server $(OPTIONS)

run-inference-agent-client:
	@bash -x src/scripts/run.sh inference_agent_client $(OPTIONS)

run-das-agent:
	@bash ./src/scripts/bazel.sh run //das_agent:main -- $(OPTIONS)

run-mork-server: build-mork-image
	@bash -x src/scripts/mork_server.sh

mork-loader:
	@bash -x src/scripts/mork_loader.sh $(FILE)

agents:
	@bash -x src/scripts/run_agents.sh $(filter-out $@, $(MAKECMDGOALS))

run-tests-db-loader:
	@bash -x src/scripts/run.sh tests_db_loader

setup-nunet-dms:
	@bash -x src/scripts/setup-nunet-dms.sh

reset-nunet-dms:
	@bash -x src/scripts/reset-nunet-dms.sh

bazel: build-image
	@bash ./src/scripts/bazel.sh $(filter-out $@, $(MAKECMDGOALS))

test-all-no-cache:
	@$(MAKE) bazel 'test --show_progress --cache_test_results=no //tests/...'

test-all: build-image run-tests-db-loader
	@$(MAKE) bazel 'test --show_progress //tests/...'

test-agents-integration:
	@bash  ./src/scripts/integration_test_setup.sh &
	@$(MAKE) bazel 'test --show_progress --cache_test_results=no //tests/integration/...' || true; \
	touch ./bin/kill

lint-all:
	@$(MAKE) bazel lint \
		"//... --fix --report --diff" \
		| grep -vE "(Lint results|All checks passed|^[[:blank:]]*$$)"

format-all:
	@$(MAKE) bazel run format

format-check:
	@$(MAKE) bazel run //:format.check

performance-tests:
	@python3 src/tests/integration/performance/query_agent_metrics.py

# Catch-all pattern to prevent make from complaining about unknown targets
%:
	@:

