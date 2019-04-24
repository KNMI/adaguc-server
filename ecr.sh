#!/bin/bash

BUILD_ACCOUNT_ID="127206424101"
REPO_NAMESPACE="knmi"
AWS_REGION="eu-west-1"

# Log in to the AWS environment using credentials configured on the machine this script is being run from.
login() {
  $(aws ecr get-login --no-include-email --region ${AWS_REGION})
}

# Creates a repository in ECR
# $1: Name of application the repository is for
create_repository() {
  REPO=$1
  echo "Creating repo: $1"

  if ! aws ecr describe-repositories --region $AWS_REGION --repository-name $REPO_NAMESPACE/$REPO; then
    echo -e "Repository does not exist, creating ..."
    aws ecr create-repository --repository-name $REPO_NAMESPACE/$REPO --region $AWS_REGION
    aws ecr put-lifecycle-policy --repository-name $REPO_NAMESPACE/$REPO --region $AWS_REGION --lifecycle-policy-text "{\"rules\":[{\"rulePriority\":1,\"description\":\"Keep last 10 tagged versions, expire others\",\"selection\": {\"tagStatus\":\"tagged\",\"tagPrefixList\":[\"build\"],\"countType\":\"imageCountMoreThan\",\"countNumber\":10},\"action\": {\"type\":\"expire\"}},{\"rulePriority\": 2,\"description\":\"Keep last 3 untagged versions, expire others\",\"selection\":{\"tagStatus\":\"untagged\",\"countType\":\"imageCountMoreThan\",\"countNumber\": 10},\"action\": {\"type\":\"expire\"}}]}"
    aws ecr set-repository-policy --repository-name $REPO_NAMESPACE/$REPO --region $AWS_REGION --policy-text "{\"Version\": \"2008-10-17\",\"Statement\": [{\"Sid\": \"AllowImageDownload\",\"Effect\": \"Allow\",\"Principal\": {\"AWS\": [\"arn:aws:iam::517047996289:root\",\"arn:aws:iam::865722340438:root\"]},\"Action\": [\"ecr:GetDownloadUrlForLayer\",\"ecr:BatchGetImage\",\"ecr:BatchCheckLayerAvailability\"]}]}"
  else
    echo -e "Repository $REPO exists"
  fi
}

usage() {
  echo ""
  echo "Usage: `basename $0` -r <repo_name> "
  echo -e "\n-r <name>: create an ECR repository with this name"
  echo ""
  exit 1
}

while getopts r: opt; do
  case $opt in
    r) REPO_NAME=${OPTARG};;
    \?) usage;;
  esac
done

shift $((OPTIND-1))

if [ -z ${REPO_NAME} ]; then
  usage
else
  login
  create_repository ${REPO_NAME}
fi
