#!/bin/bash

ERR=0

LIBROOT="$(readlink -f "../../")"

case "$1" in
	build)
		"$LIBROOT/build.sh" ; ERR=$?
		shift 1 ;;
	rebuild)
		"$LIBROOT/build.sh" --rebuild ; ERR=$?
		shift 1 ;;
esac

if [ -n "$1" -a $ERR -eq 0 ]
then
	export PYTHONPATH="$LIBROOT/tool:$PYTHONPATH"
	python3 "$LIBROOT/AVR/runner.py" "$1"
fi

