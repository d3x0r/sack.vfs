#!/bin/bash

echo arg: $1

type patchelf >/dev/null 2>&1 || { echo >&2 "patchelf required but it's not installed.  Aborting."; exit 1; }

libs=$(find $1 -name *.so -o -name *.nex -o -name *.isp)

for lib in $libs; do
	strip0=${lib//$1\/}
	strip1=${strip0//[^\/]}
	slashes=${#strip1}

	#echo $lib $strip1 $slashes

	if [ $slashes -eq 0 ]; then patchelf --set-rpath $pwd/build/:\$ORIGIN $lib ; fi
	if [ $slashes -eq 1 ]; then patchelf --set-rpath  $pwd:\$ORIGIN/..:\$ORIGIN $lib ; fi
	if [ $slashes -gt 1 ]; then echo $lib $slashes; fi
done

