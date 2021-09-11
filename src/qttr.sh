#!/bin/sh

# This is a simple script which extracts translation data from all source files and generates/updates *.ts files for them.
# This is done via a call to Qt's lupdate tool.
# This script will search for lupdate under various aliases, preferring lupdate-qt4. Alternatively, you can pass the name of the lupdate
# executable as first argument to this script.

ROOT_DIR="`dirname "$0"`/electronicsdb"
OUT_DIR="`dirname "$0"`"

if [ ! -d "$ROOT_DIR" ];
then
	echo 'Error: Root directory does not exist!'
	exit 1
fi

if [ -n "$1" ];
then
	LUPDATE="$1"
else
	# Search for lupdate under various names. Prefer the tool with '-qt4' suffix to allow coexistance of lupdate for Qt3
	LUPDATE_ALIASES="lupdate-qt4 lupdate"
	
	for cmdname in $LUPDATE_ALIASES;
	do
		if which "$cmdname" > /dev/null;
		then
			LUPDATE="`which $cmdname`"
			break
		fi
	done

	if [ -z "$LUPDATE" ];
	then
		echo 'Error: lupdate not found!'
		echo "Searched in \$PATH for programs with the following names: $LUPDATE_ALIASES"
		exit 2
	fi
fi

echo "Using lupdate executable $LUPDATE"

$LUPDATE -noobsolete "$ROOT_DIR" -ts "$OUT_DIR/electronics_en.ts" "$OUT_DIR/electronics_de.ts"
