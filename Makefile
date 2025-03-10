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

build-all: build-image
	@bash -x src/scripts/build.sh

run-query-agent:
	@bash -x src/scripts/run.sh query_broker 31700

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

setup-nunet-dms:
	@bash -x src/scripts/setup-nunet-dms.sh

reset-nunet-dms:
	@bash -x src/scripts/reset-nunet-dms.sh

bazel:
	@bash ./src/scripts/bazel.sh $(filter-out $@, $(MAKECMDGOALS))

test-all: build-image
	@$(MAKE) bazel test //...

lint-all:
	@$(MAKE) bazel lint \
		"//... --fix --report --diff" \
		| grep -vE "(Lint results|All checks passed|^[[:blank:]]*$$)"

format-all:
	@$(MAKE) bazel run format

# Catch-all pattern to prevent make from complaining about unknown targets
%:
	@:

