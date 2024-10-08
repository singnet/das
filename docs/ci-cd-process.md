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

### Deployment and PyPI Publishing Process (with Poetry)

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