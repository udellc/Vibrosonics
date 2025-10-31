# Contributing Guide

How to set up, code, test, review, and release so contributions meet our Definition of Done.

## Code of Conduct

Reference the project/community behavior expectations and reporting process.

## Getting Started

Refer to the [Setup (Developer Setup)](https://udellc.github.io/Vibrosonics/setup.html) document for instructions on setting up a development environment.

## Branching & Workflow

This repository follows a Git Flow workflow. Developers for this capstone year will create feature/fix branches off of the 25-26\_dev branch and make PRs to merge back to the developer branch. 25-26\_dev will be periodically merged into the main branch once changes have been thoroughly tested and deemed release-ready. Feature branches should follow the naming convention “feat/featureName”, and fix branches should follow the naming convention “fix/fixName”. To maintain a clean commit history, all branches should be rebased onto the branch they are merging into.

## Issues & Planning

Explain how to file issues, required templates/labels, estimation, and triage/assignment practices.  
To report a bug or request a feature, open an issue if you do not find a relevant one already made. If you’d like to help improve the project:

* Create a branch for your feature/fix. If it is corresponding to an issue, make sure the branch is linked to that issue.  
* Commit your changes with descriptive commit messages.  
* Push to your branch and open a new Pull Request.

## Commit Messages

Commits should follow Conventional Commits standards. At a minimum, commits should contain a type and a short description of the commit contents. The most common commit types are “feat” and “fix”, which correspond to a new feature or a bug fix respectively. Other relevant types are “doc” for updating documentation, “test” for test changes, and “refactor” for code refactors. For the purposes of this project, we do not require a body or footer for commits. In-depth descriptions of changes and links to GitHub issues can be contained in PRs rather than each individual commit.

Commit examples:  
feat: add newFunct() method to TestClass  
fix: fix out of bounds error in brokenFunct()  
doc: update README with new build instructions

## Code Style, Linting & Formatting

\*\*many linters require npm to be installed\*\*  
JavaScript linter: ESLint – [https://github.com/eslint/eslint](https://github.com/eslint/eslint)

* To run manually: npm run lint

C++ and Arduino formatter: .clang-format (given with the Vibrosonics project)  – [https://github.com/udellc/Vibrosonics/blob/main/.clang-format](https://github.com/udellc/Vibrosonics/blob/main/.clang-format)  

* Located in the root of the repository  
* Example of how to manually format multiple files:
  * clang-format \-i \*.cpp  
  * clang-format \-i \*.ino

* When using Visual Studio Code, it can be installed automatically through the CSSCop extension.

## Testing

API Testing

* Primarily used for the interaction between the API and web application.  
* Each test is expected to cover one corresponding HTTP method to ensure precise coverage.

UI Testing

* Primarily used for testing the web application’s user interface.  
* Each test is expected to cover one corresponding feature of the web app to ensure precise coverage.  
* Each test is expected to cover the entire process of a feature, from login to final expected feature state in order to guarantee that the feature being tested has not made an impact on the flow and set up of the web app.

Regression Testing

* Used primarily for testing the web application’s user interface.  
* Manual testing to ensure the user interface features are functioning and work in tandem with each other.  
* These tests will not be completed as regularly as the other testing, as it is more time consuming and if done regularly can be very monotonous.

Integration Testing

* Used primarily for testing the team’s libraries.  
* Each test is expected to test the different interactions between our library modules.  
* These tests will verify a seamless interaction between library functions and their actions/responses.

New/updated tests are needed when there is an updated or created aspect of the codebase. Because each test is going to be applied to one aspect of the API, web app, and libraries, there should be enough features between the overall codebase where the tests can be easily repeated or modified.

## Pull Requests & Reviews

Follow existing code style and conventions, which largely follow the Arduino library style guidelines and the Webkit formatting. There is a .clang-format file for formatting .cpp/.h files, and use the default Arduino formatting for .ino files.

In order for status checks to pass to merge, each PR must pass the linter/formatter and have at least two reviews from other members.

## CI/CD

Link to Github Actions: [udellc/Vibrosonics/actions](https://github.com/udellc/Vibrosonics/actions)

Workflow Jobs:

* Publish Documentation to Github Pages  
  * Publishes VibroSonics documentation files to Github pages using Doxygen.  
  * Triggers the workflow when pushes or release events occur on the main branch.  
* ESLint Web App  
  * Runs ESLint tool for the WebApp directory, checking for syntax errors or potential bugs.  
  * Triggers the workflow when pushes or pull requests occur on the 25-36\_dev or main branches.

Viewing Workflow Logs:

* Navigate the Github actions link provided in this document.  
* Select the desired workflow you want to view.

Reruning Workflows:

* Navigate the Github actions link provided in this document.  
* Select the desired workflow to rerun, workflows should be in the left sidebar  
* Select the desired workflow run then re-execute and select Re-run all jobs or specific one if desired.

Mandatory Requirements Prior to Merges:

* The  25-26\_dev branch must pass ALL workflows before merging into main. Any exceptions made must be clearly documented with sections detailing what was falling, why if failed (if applicable), and why the exception was made.

  NOTE: The 25-26\_dev branch may fail from features/fix branches merging into it, it only needs to pass before merging it into main.  
  * ESLint Web App \[ Critical \]:  
    * This job evaluates .js and .jsx files in the WebApp directory and logs any syntax errors or potential issues. This job MUST pass in order to be merged into the main branch

## Security & Secrets

Reporting Vulnerabilities:

* Do not open a public GitHub issue for the vulnerability  
  * This could lead to further exploits before the vulnerability can be fully resolved  
* Directly email the repository owners with a description of the vulnerability  
  * Reference specific code files and lines when possible  
  * If applicable, provide step-by-step instructions on how to reproduce the vulnerability  
* A repository owner will investigate and resolve the reported issue as soon as possible

Secrets Rules:

* No API keys, passwords, or other secrets should be committed to the public repository  
* If a secret is mistakenly committed, undo the commit and change/invalidate the secret as soon as possible

## Documentation Expectations

README should contain a simple but complete description of the project that includes program functionality, installation and setup instructions, including dependencies, examples of usage and key features, directions on how to contribute such as pull request guidelines and issue reporting, and any license information. Self-documenting code should be implemented for improved readability. Functions and more complex pieces of code should be well commented, explaining its functionality and logic. Documentation should be regularly updated to reflect changes made in the codebase, with plain and concise language.  

Current API References:

* VibroSonics README: [https://github.com/udellc/Vibrosonics/blob/main/README.md](https://github.com/udellc/Vibrosonics/blob/main/README.md)
* VibroSonics documentation: [https://udellc.github.io/Vibrosonics/](https://udellc.github.io/Vibrosonics/)
* AudioPrism README: [https://github.com/udellc/AudioPrism/blob/main/README.md](https://github.com/udellc/AudioPrism/blob/main/README.md)

## Release Process

Versioning Scheme:

* Adhere to Semantic Version, where version numbers follow a MAJOR.MINOR.PATCH format  
* PATCH increment is for bug fixes  
* MINOR is for new, backward-compatible features  
* MAJOR is for any breaking changes

Tagging:

* Create an annotated git tag on the main branch  
  * Example of annotated git tag: git tag \-a v1.2.5 \-m “Release version 1.2.5”

Changelog Generation:

* Our process uses manual changelog generation  
* We follow the past release format, with each changelog having a descriptive name, a few sentences about the updates, and the changed files attached in .zip and tar.zip folders

Packaging/Publishing Steps:

* For our libraries, we build a source distribution that can be found in the release section of the github repo, sorted by release  
* These folders are then uploaded to the public repository

Rollback Process:

* We use a manual procedure triggered by human observation  
* Each deployment is monitored both by the internal team and the project partners who perform a series of manual checks.  
* If any of the manual checks fail, there will be an immediate rollback  
* If all the manual checks pass, the deployment is successful

## Support & Contact

* Main support contact channels: ‘cs-capstone’ Discord channel and internal Teams channel  
* Expected response windows: 24 hours  
* Questions pertaining to the low-level codebase structure, hardware, systems, and web development will be directed to project partners Nick, Hans, or Dr.Udell via Discord or email.  
* Higher level questions or questions about the coursework will be directed to the internal teams channel via Microsoft Teams.
