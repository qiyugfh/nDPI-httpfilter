#!/bin/bash

process="ndpi-httpfilter"

while true; do
  cmd=`ps -aux | grep $process | grep -v grep`
  if [ ${#cmd} -eq 0 ]; then
    echo $process" process does not exist, exit ..."
    exit -1
  fi
  echo "[`date +'%Y-%m-%d %H:%M:%S'`] $cmd" >> top.log 2>&1
  sleep 10
done

exit 0

