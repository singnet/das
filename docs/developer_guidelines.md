# Developer Guidelines

## Introduction

General guidelines and suggestions while working on DAS repositories.

### Pull Requests

Pull Requests (PRs) are crucial for collaboration in software development.
A well-crafted PR title helps reviewers understand the purpose and scope of
your changes quickly. This guide outlines best practices for creating clear and
informative Pull Requests.

While not currently enforced, we do want to link all PR's with it's own tracking
issue at the [DAS Board](https://github.com/orgs/singnet/projects/6)

#### Guidelines for PR Titles

1. **Format**: Use the format `[#issue] Concise, self-contained, explanatory title`.
2. **Include Issue References**: Always reference the associated issue number
in the PR title. This links the PR to the relevant discussion or task in our
tasks dashboard.
3. **Conciseness**:
Keep the title succinct while conveying the essence of the change.
Avoid unnecessary details.

4. **Self-contained Explanation**: Ensure the title provides enough context to
understand the changes without needing to delve into the PR description
immediately.

5. **Clarity and Specificity**: Clearly describe what the PR accomplishes.
Use specific terms related to functionality, bug fixes, or enhancements.

6. **Review Before Submitting**: Double-check your title to ensure it accurately
reflects the content and purpose of your changes.

7. **Think from the Reviewer's Perspective**: Imagine you are reviewing the
PR—what information would you need from the title to understand the changes
quickly?

8. **Examples**:
    - `[#234] Refactor authentication service for improved performance`
    - `[#890] Fix issue with date formatting in event calendar`

Clear and informative PR titles help streamline the review process and ensure
that changes are understood and integrated smoothly.

### Handling EPIC Issues

To ensure proper management and tracking of EPIC issues on GitHub, follow these
guidelines:

1. **Labeling EPIC Issues**: Add the `EPIC` label whenever creating an epic
   issue.
2. **Mandatory Text in EPIC Issues**: Include the following mandatory text in
   the epic issue description:
   ```
   EPIC

   * <link to child issue #1>
   * <link to child issue #2>
   * ...
   ```

3. Here is an example of an epic issue: https://github.com/orgs/singnet/projects/6/views/1?filterQuery=epic+label%3AEPIC&pane=issue&itemId=66967931

## Conclusion

By following these guidelines, you can significantly improve communication and
efficiency within our development team.
