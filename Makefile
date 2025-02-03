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
	cd src && bash -x scripts/docker_image_build.sh

build-spawner-image: build
	cd src && bash +x scripts/docker_image_spawner_build.sh

build: build-image
	cd src && bash -x scripts/build.sh

tests: build-image
	cd src && bash -x scripts/unit_tests.sh

run-query-agent:
	@bash -x src/scripts/run.sh query_broker 31700

run-attention-broker:
	@bash -x src/scripts/run.sh attention_broker_service 37007

run-link-creation-agent:
	@bash -x src/scripts/run.sh link_creation_server $(OPTIONS)

run-link-creation-client:
	@bash -x src/scripts/run.sh link_creation_agent_client $(OPTIONS)

run-sentinel-server:
	@bash -x src/scripts/run.sh sentinel_server 55000

run-worker-client:
	@bash -x src/scripts/run.sh worker_client $(OPTIONS)


