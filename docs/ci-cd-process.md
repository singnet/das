<h1><a href="https://github.com/singnet/das-atom-db" target="_blank">AtomDB</a></h1>

![atomdb](./assets/das-atom-db.jpg)

The process involves the following key phases:

1. **Branches**
   - The project is developed based on two main branches:
     - Main branch: `master`
     - Feature and development branches: any branch created during development (represented as `/*` in the diagram).
2. **Opening Pull Requests (PR)**
   - During development, developers create branches for new features or fixes and open Pull Requests to the `master` branch.

3. **Test Pipeline**
   - When a PR is opened, a CI pipeline is triggered to ensure code quality. This pipeline executes the following steps:
     - **Lint:** Checks if the code adheres to style and quality standards.
     - **Unit Test:** Runs unit tests to validate the behavior of individual code units.
     - **Coverage:** Calculates test coverage to verify which parts of the code are being tested.
     - **Integration Tests:** Runs integration tests to check the interaction between different system components.

4. **Merge into `master`**
   - After PR approval, the code is merged into the `master` branch, but the deployment process is not triggered automatically; it needs to be triggered manually.

### Deployment and PyPI Publishing Process

1. **Publishing Pipeline**
   - After the merge into the `master` branch, you can trigger a pipeline to deploy the new version to PyPI using **Poetry**. The pipeline follows these steps:
     - **Tag:** Generates a tag for the new version.
     - **Publish:** Publishes the <a href="https://pypi.org/project/hyperon-das-atomdb" target="_blank">hyperon-das-atomdb</a> package to PyPI using **Poetry**. The `poetry publish` command is used to package and upload the project to PyPI.
2. **Mattermost Notification**
   - After the publishing process, a notification is automatically sent to the Mattermost channel to announce the newly published version.

3. **Update on PyPI**
   - The <a href="https://pypi.org/project/hyperon-das-atomdb" target="_blank">hyperon-das-atomdb</a> package is made available on PyPI, where it can be downloaded and installed by other projects.

### Component Details

1. **<a href="https://github.com/singnet/das" target="_blank">DAS</a>**
   - The DAS orchestrates the publishing flow and performs the following tasks:
     - **Setup:** Configures the necessary environment for deployment.
     - **Tag:** Generates a version tag based on recent changes.
     - **Notify Mattermost:** Sends a notification to the appropriate Mattermost channel after a successful deployment.

2. **Integration with PyPI using Poetry**
   - The project is configured publish the package to PyPI, using **Poetry** as the package management and publishing tool.
   - The publishing process includes:
     - Creating a version tag.
     - Publishing the package to PyPI.

<h1><a href="https://github.com/singnet/das-atom-db" target="_blank">Query Engine</a></h1>

![query-engine](./assets/das-query-engine.jpg)

The process includes the following phases:

1. **Branches**
   - The project is developed with the following branches:
     - Main branch: `master`
     - Feature and development branches: represented as `/*`, created during the development process.

2. **Pull Requests (PR)**
   - Developers create new branches for features or bug fixes and open Pull Requests to merge into the `master` branch.

3. **Testing Pipeline**
   - A CI pipeline is triggered when a PR is opened, running the following steps:
     - **Lint:** Ensures the code adheres to style and quality standards.
     - **Unit Test:** Executes unit tests to validate individual components of the code.
     - **Coverage:** Measures the code coverage to ensure proper testing.
     - **Integration Tests:** Verifies the interaction between different components of the system.
     - **Benchmarks:** Runs performance benchmarks, which are executed only on the master branch.

4. **Merge into `master`**
   - After PR approval, the code is merged into the `master` branch, but the deployment process is not triggered automatically; it needs to be triggered manually.

### Deployment and Publishing to PyPI

1. **Publishing Pipeline**
   - After the merge into the `master` branch, you can trigger a pipeline to deploy the package to PyPI using **Poetry**. The process follows these steps:
     - **Tag Creation:** A tag for the new version is generated.
     - **Publish to PyPI:** The package <a href="https://pypi.org/project/das-query-engine" target="_blank">das-query-engine</a> is published to PyPI via the `poetry publish` command.

2. **Mattermost Notifications**
   - After the publishing process, an automated notification is sent to a Mattermost channel to inform about the new release.

3. **PyPI Availability**
   - The package <a href="https://pypi.org/project/das-query-engine" target="_blank">das-query-engine</a> is now available on PyPI for other projects to download and install.

### Component Breakdown

1. **<a href="https://github.com/singnet/das" target="_blank">DAS</a>**
   - The DAS automates the deployment process and handles the following tasks:
     - **Setup:** Prepares the environment for deployment.
     - **Tag Generation:** Creates a version tag based on recent changes.
     - **Mattermost Notification:** Sends an automated notification to a designated Mattermost channel after a successful deployment.

2. **Poetry and PyPI Integration**
   - The project is set up to publish the package to PyPI after create a new version using **Poetry**.
   - The publishing process involves:
     - Tagging the new version.
     - Publishing the package to PyPI.

<h1><a href="https://github.com/singnet/das-atom-db" target="_blank">DAS Metta Parser</a></h1>

![das-metta-parser](./assets/das-metta-parser.jpg)

The process involves the following key phases:

1. **Branches**
   - The project is developed based on two main branches:
     - Main branch: `master`
     - Feature and development branches: any branch created during development (represented as `/*` in the diagram).

2. **Opening Pull Requests (PR)**
   - During development, developers create branches for new features or fixes and open Pull Requests to the `master` branch to initiate the review and merging process.

3. **Test Pipeline**
   - When a PR is opened, a CI pipeline is triggered to ensure code quality. This pipeline executes the following steps:
     - **Lint:** Verifies if the code follows predefined style and quality standards.
     - **Unit Test:** Executes unit tests to validate individual parts of the code.
     - **Coverage:** Assesses the coverage to ensure sufficient testing across the codebase.
     - **Integration Tests:** Runs integration tests to validate the interaction between different components of the system.

4. **Merge into `master`**
   - After PR approval, the code is merged into the `master` branch, but the deployment process is not triggered automatically; it needs to be triggered manually.

### Build and Docker Image Publishing Process

1. **Build Pipeline**
   - After the merge into the `master` branch, you can trigger a pipeline to build and deploy a new Docker image. The pipeline follows these steps:
     - **Build Binary:** A binary version of the code is built and saved as a GitHub artifact.
     - **Build Image:** A Docker image is built from the binary.
     - **Tag:** A version tag is created to uniquely identify the build.
     - **Push Docker Image:** The built Docker image is automatically pushed to Docker Hub under the <a href="https://hub.docker.com/r/trueagi/das" target="_blank">trueagi/das</a> repository.

2. **Mattermost Notification**
   - After the Docker image is pushed to Docker Hub, a notification is sent to the Mattermost channel to inform the team about the new deployment and version update.

3. **Docker Hub Availability**
   - The <a href="https://hub.docker.com/r/trueagi/das" target="_blank">trueagi/das</a> Docker image is now available on Docker Hub for use in deployment and further development.

### Component Details

1. **<a href="https://github.com/singnet/das" target="_blank">DAS</a>**
   - DAS orchestrates the entire deployment process and performs the following tasks:
     - **Setup:** Prepares the environment and prerequisites for building and deployment.
     - **Tag:** Generates a version tag based on the current build.
     - **Notify Mattermost:** Sends a notification to the appropriate Mattermost channel about the successful build and deployment.

2. **Integration with Docker Hub**
   - The project is configured to automatically push the Docker image to Docker Hub after a successful build. The process includes:
     - Building the Docker image from the binary.
     - Pushing the image to Docker Hub under the <a href="https://hub.docker.com/r/trueagi/das" target="_blank">trueagi/das</a> repository.
     - Tagging the new version to keep track of updates and changes.

<h1><a href="https://github.com/singnet/das-serverless-functions" target="_blank">DAS Serverless Functions</a></h1>

![das-serverless-functions](./assets/das-serverless-functions.jpg)

The process involves the following key phases:

1. **Branches**
   - The project is developed based on two main branches:
     - Main branch: `master`
     - Feature and development branches: any branch created during development (represented as `/*` in the diagram).

2. **Opening Pull Requests (PR)**
   - During development, developers create branches for new features or fixes and open Pull Requests to the `master` branch to initiate code review and merging.

3. **Test Pipeline**
   - When a PR is opened, a CI pipeline is triggered to ensure code quality. This pipeline executes the following steps:
     - **Lint:** Ensures the code adheres to predefined style and quality standards.
     - **Unit Test:** Runs unit tests to validate the behavior of individual code components.
     - **Coverage:** Measures the code coverage to verify testing completeness.
     - **Integration Tests:** Runs integration tests to check the interactions between different components.

4. **Merge into `master`**
   - After PR approval, the code is merged into the `master` branch, but the deployment process is not triggered automatically; it needs to be triggered manually.

### Deployment and Serverless Functions Publishing Process

1. **Publishing Pipeline**
   - After the merge into the `master` branch, you can trigger a pipeline to build a new version of serverless functions. The pipeline follows these steps:
     - **Tag:** Generates a tag for the new version of serverless functions.
     - **Publish to Serverless:** Deploys the new version of the serverless functions to the cloud provider.
     - **Update Configuration:** Updates the necessary configuration to reflect the new version.

2. **Mattermost Notification**
   - After the publishing process, a notification is automatically sent to the Mattermost channel to announce the newly published version of serverless functions.

3. **Serverless Functions Availability**
   - The new version of the serverless functions is now available for use in production and can be accessed via the specified endpoints.

### Component Details

1. **<a href="https://github.com/singnet/das" target="_blank">DAS</a>**
   - DAS orchestrates the entire deployment process and performs the following tasks:
     - **Setup:** Configures the necessary environment for deployment.
     - **Tag:** Generates a version tag based on recent changes.
     - **Notify Mattermost:** Sends a notification to the appropriate Mattermost channel after a successful deployment.

2. **Serverless Functions Integration**
   - The project is set up to automatically deploy serverless functions after merging. The deployment process includes:
     - Tagging the new version of serverless functions.
     - Deploying to the cloud provider and updating the necessary configuration.
