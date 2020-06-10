# Ops Management Starter Project - Supply Finder

### Prerequisite
- Bazel
- A GCP project with billing enabled

### How to run

#### Run it locally
Open four terminals, and run the following commands seperately:
```
env STACKDRIVER_PROJECT_ID=[YOUR-PROJECT-ID] ./bazel-bin/supplyfinder_finder
env STACKDRIVER_PROJECT_ID=[YOUR-PROJECT-ID] ./bazel-bin/supplyfinder_client
env STACKDRIVER_PROJECT_ID=[YOUR-PROJECT-ID] ./bazel-bin/supplyfinder_supplier
env STACKDRIVER_PROJECT_ID=[YOUR-PROJECT-ID] ./bazel-bin/supplyfinder_vendor
```
e.g.:
`env STACKDRIVER_PROJECT_ID=cal-intern-project ./bazel-bin/supplyfinder_supplier`
