#!/bin/bash
#
# Copyright 2019 The Cloud Robotics Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -o pipefail -o errexit
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd ${DIR}

function die {
  echo "$1" >&2
  exit 1
}

function init_address {
  export SUPPLIER=$(kubectl get svc supplyfinder-supplier -o jsonpath='{..ip}'):$(kubectl get svc supplyfinder-supplier -o jsonpath='{..port}')
  echo SUPPLIER: $SUPPLIER
  export SVC=supplyfinder-vendor-1
  export VENDOR1=$(kubectl get svc $SVC -o jsonpath='{..ip}'):$(kubectl get svc $SVC -o jsonpath='{..port}')
  echo VENDOR1: $VENDOR1
  export SVC=supplyfinder-vendor-2
  export VENDOR2=$(kubectl get svc $SVC -o jsonpath='{..ip}'):$(kubectl get svc $SVC -o jsonpath='{..port}')
  echo VENDOR2: $VENDOR2
  export SVC=supplyfinder-vendor-3
  export VENDOR3=$(kubectl get svc $SVC -o jsonpath='{..ip}'):$(kubectl get svc $SVC -o jsonpath='{..port}')
  echo VENDOR3: $VENDOR3
  export SVC=supplyfinder-finder
  export FINDER=$(kubectl get svc $SVC -o jsonpath='{..ip}'):$(kubectl get svc $SVC -o jsonpath='{..port}')
  echo FINDER: $FINDER
}

function build_images {
  export SUPPLIER_ADDRESS=$(kubectl get svc supplyfinder-supplier -o jsonpath='{..ip}'):$(kubectl get svc supplyfinder-supplier -o jsonpath='{..port}')
  push_image client
  push_image finder
  push_image supplier
  push_image vendor
}

function run_finder {
  docker run --rm -p 50051:50051 --network=host --name supplyfinder-finder supplyfinder-finder
}

function run_supplier {
  docker run --rm -p 50052:50052 --network=host --name supplyfinder-supplier supplyfinder-supplier
}

function run_vendor {
  docker run --rm -p 50053:50053 --network=host --name supplyfinder-vendor supplyfinder-vendor 
}

function run_client {
  docker run --rm --network=host --name supplyfinder-client supplyfinder-client supplyfinder-client
}

function push_image {
  local target=$1
  init_address
  docker build --build-arg SUPPLIER --build-arg VENDOR1 --build-arg VENDOR2 --build-arg VENDOR3 --build-arg FINDER -f "${target}/Dockerfile" -t "supplyfinder-${target}" .
  docker tag "supplyfinder-${target}" "gcr.io/${PROJECT_ID}/supplyfinder-${target}"
  docker push "gcr.io/${PROJECT_ID}/supplyfinder-${target}"
}

function create_config {
  cat greeter-server.yaml.tmpl | envsubst >greeter-server.yaml
}

# public functions
function push_client {
  push_image client
}

function deploy_endpoints {
  gcloud endpoints services deploy api_descriptor.pb api_config.yaml
}

function enable_api {
  gcloud services enable supplyfinder.endpoints.cal-intern-project.cloud.goog
}

function delete_api {
  gcloud endpoints services delete supplyfinder.endpoints.cal-intern-project.cloud.goog
}

function update_config {
  kubectl apply -f grpc-supplyfinder.yaml
}

function update_server {
  push_image server
  kubectl delete pod -l 'app=greeter-server-app'
  update_config
}

function create {
  create_pb
  deploy_endpoints
  enable_api
  
  # push_image server
  # push_client
  # update_config
}

function delete {
  create_config
  kubectl delete -f greeter-server.yaml greeter-server.yaml
}

function create_pb {
  protoc --include_imports --include_source_info proto/supplyfinder.proto --descriptor_set_out api_descriptor.pb
}

# main
if [[ -z ${PROJECT_ID} ]]; then
#   die "Set PROJECT_ID first: export PROJECT_ID=[GCP project id]"
echo "PROJECT_ID set to cal-intern-project"
export PROJECT_ID=cal-intern-project
fi

# if [[ ! "$1" =~ ^(create|delete|update_config|update_server|push_client|create_pb|deploy_endpoints|build_images|run_images)$ ]]; then
#   die "Usage: $0 {create|delete|update_config|update_server|push_client}"
# fi

# call arguments verbatim:
"$@"
