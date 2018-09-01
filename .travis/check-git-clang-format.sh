#!/bin/bash

if [ "$TRAVIS_PULL_REQUEST" == "true" ] ; then
    base_commit="$TRAVIS_BRANCH"
else
    base_commit="HEAD^"
fi

output="$(sudo python .travis/git-clang-format --binary clang-format-3.8 --commit $base_commit --diff)"

if [ "$output" == "no modified files to format" ] || [ "$output" == "clang-format did not modify any files" ] ; then
    echo "clang-format passed."
    exit 0
else
    echo "clang-format failed."
    echo "$output"
    exit 1
fi
