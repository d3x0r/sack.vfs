#!/bin/bash

cd "$(dirname "$0")"

#echo arg: $1 in $PWD

type patchelf >/dev/null 2>&1 || { echo >&2 "patchelf required but it's not installed.  Aborting."; exit 1; }


patchelf --set-rpath \$ORIGIN/lib $1/sack_gui.node
#bin=$(ls -p bin | grep -v /)
bins=$(ls -d $1/bin/*)
apps=$(ls -d $1/share/SACK/applicationCore/*)
plugins=$(ls -d $1/share/SACK/plugins/*)
libs=$(ls -d $1/lib/*.so);#$(find $1/share/SACK/plugins -name *.so -o -name *.nex -o -name *.isp)

#echo $apps
#echo $bins
#echo $plugins
#echo $libs

patchelf --set-rpath $PWD/$1/lib $bins; 
patchelf --set-rpath $PWD/$1/lib:$PWD/build/share/SACK/plugins $apps; 
patchelf --set-rpath $PWD/$1/lib:\$ORIGIN/../applicationCore:\$ORIGIN $plugins; 
patchelf --set-rpath \$ORIGIN $libs; 

<<skip-smart-patch
for lib in $libs; do
	#strip0=${lib//$1\/}
	#strip1=${strip0//[^\/]}
	#slashes=${#strip1}

	#echo $lib $strip1 $slashes

	if [ $slashes -eq 0 ]; then patchelf --set-rpath $pwd/build/:\$ORIGIN $lib ; fi
	if [ $slashes -eq 1 ]; then patchelf --set-rpath  $pwd:\$ORIGIN/..:\$ORIGIN $lib ; fi
	if [ $slashes -gt 1 ]; then echo $lib $slashes; fi
done

skip-smart-patch
