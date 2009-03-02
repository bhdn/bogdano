#!/bin/sh
wget -c -r -l 1 --restrict-file-names=nocontrol "$@"
