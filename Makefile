release: build-deployment
	@docker run --rm -it das-deployment:latest /opt/scripts/deployment/release.sh

deploy: build-deployment
	@docker run --rm -it das-deployment:latest /opt/scripts/deployment/fn_deploy.sh

build-deployment:
	@docker build -f .docker/deployment/Dockerfile -t das-deployment:latest .

build-semver:
	@docker image build -t trueagi/das:semantic-versioning ./scripts/semver

publish-semver:
	@docker image push trueagi/das:semantic-versioning
