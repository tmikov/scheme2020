#!/usr/bin/env bash
IFS=$'\n'
set -f

FORMAT_ALL=""
if [ "$1" == "-a" ]; then
    shift
    FORMAT_ALL="1"
fi

[ -n "$1" ] && echo "$0: Too many arguments" && exit 1

FILES=()

if [ -n "$FORMAT_ALL" ]; then
  echo "Formatting everything"
  FILES=( $(find . -path ./cxx/external -prune -o -path ./\*build\* -prune -o \
            -name \*.h -print -o -name \*.cpp -print) )
elif ! git diff --exit-code --quiet ; then
  echo "Formatting only changed"
  FILES=( $(git diff --name-only -- '*.cpp' '*.h' '*.hpp') )

elif ! git log --exit-code "HEAD~1..HEAD" --author=$USER > /dev/null ; then
  read -r -p "Format files in last commit? [y/N]" reply
  if [[ $reply == [Yy] ]]; then
    FILES=( $(git diff --name-only "HEAD~1..HEAD" -- '*.cpp' '*.h' '*.hpp') )
  else
    exit 1
  fi
else
  echo "Nothing to format"
  exit 1
fi

for f in "${FILES[@]}"; do
  echo "$f"
  clang-format -i -style=file "$f"
done
