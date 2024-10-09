## AtomDB

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
   - After PR approval, the code is merged into the `master` branch, triggering the deployment process.

### Deployment and PyPI Publishing Process

1. **Publishing Pipeline**
   - After the merge into the `master` branch, a publishing pipeline is triggered to deploy the new version to PyPI using **Poetry**. The pipeline follows these steps:
     - **Tag:** Generates a tag for the new version.
     - **Publish:** Publishes the `hyperon-das-atomdb` package to PyPI using **Poetry**. The `poetry publish` command is used to package and upload the project to PyPI.
2. **Mattermost Notification**

   - After the publishing process, a notification is automatically sent to the Mattermost channel to announce the newly published version.

3. **Update on PyPI**
   - The `hyperon-das-atomdb` package is made available on PyPI, where it can be downloaded and installed by other projects.

### Component Details

1. **DAS**

   - The DAS orchestrates the publishing flow and performs the following tasks:
     - **Setup:** Configures the necessary environment for deployment.
     - **Tag:** Generates a version tag based on recent changes.
     - **Notify Mattermost:** Sends a notification to the appropriate Mattermost channel after a successful deployment.

2. **Integration with PyPI using Poetry**
   - The project is configured to automatically publish the package to PyPI after merging, using **Poetry** as the package management and publishing tool.
   - The publishing process includes:
     - Creating a version tag.
     - Publishing the package to PyPI.

## Query Engine

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

4. **Merge into `master`**
   - After PR approval, the code is merged into the `master` branch, starting the deployment process.

### Deployment and Publishing to PyPI

1. **Publishing Pipeline**

   - After the code is merged into `master`, a pipeline is triggered to deploy the package to PyPI using **Poetry**. The process follows these steps:
     - **Tag Creation:** A tag for the new version is generated.
     - **Publish to PyPI:** The package `das-query-engine` is published to PyPI via the `poetry publish` command.

2. **Mattermost Notifications**

   - After the publishing process, an automated notification is sent to a Mattermost channel to inform about the new release.

3. **PyPI Availability**
   - The package `das-query-engine` is now available on PyPI for other projects to download and install.

### Component Breakdown

1. **DAS (Deploy Automation System)**

   - The DAS automates the deployment process and handles the following tasks:
     - **Setup:** Prepares the environment for deployment.
     - **Tag Generation:** Creates a version tag based on recent changes.
     - **Mattermost Notification:** Sends an automated notification to a designated Mattermost channel after a successful deployment.

2. **Poetry and PyPI Integration**
   - The project is set up to automatically publish the package to PyPI after the merge using **Poetry**.
   - The publishing process involves:
     - Tagging the new version.
     - Publishing the package to PyPI.

## DAS Metta Parser

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
   - After PR approval, the code is merged into the `master` branch, triggering the build and deployment processes.

### Build and Docker Image Publishing Process

1. **Build Pipeline**
   - After the merge into the `master` branch, a pipeline is triggered to build and deploy a new Docker image. The pipeline follows these steps:
     - **Build Binary:** A binary version of the code is built and saved as a GitHub artifact.
     - **Build Image:** A Docker image is built from the binary.
     - **Tag:** A version tag is created to uniquely identify the build.
     - **Push Docker Image:** The built Docker image is automatically pushed to Docker Hub under the `trueagi/das` repository.

2. **Mattermost Notification**
   - After the Docker image is pushed to Docker Hub, a notification is sent to the Mattermost channel to inform the team about the new deployment and version update.

3. **Docker Hub Availability**
   - The `trueagi/das` Docker image is now available on Docker Hub for use in deployment and further development.

### Component Details

1. **DAS (Deploy Automation System)**
   - DAS orchestrates the entire deployment process and performs the following tasks:
     - **Setup:** Prepares the environment and prerequisites for building and deployment.
     - **Tag:** Generates a version tag based on the current build.
     - **Notify Mattermost:** Sends a notification to the appropriate Mattermost channel about the successful build and deployment.

2. **Integration with Docker Hub**
   - The project is configured to automatically push the Docker image to Docker Hub after a successful build. The process includes:
     - Building the Docker image from the binary.
     - Pushing the image to Docker Hub under the `trueagi/das` repository.
     - Tagging the new version to keep track of updates and changes.
