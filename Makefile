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

build: build-image
	cd src && bash -x scripts/build.sh

tests: build-image
	cd src && bash -x scripts/unit_tests.sh
