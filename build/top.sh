#!/bin/bash

process="ndpi-httpfilter"
log_file="top.log"
cat /dev/null >${log_file}


function write_log() {
	echo "[$( date +'%Y-%m-%d %H:%M:%S' )] $1" >> ${log_file} 2>&1
}

write_log "start top.sh to moniter ${process} process memory status ..."

while true; do
  cmd=$( ps aux | grep $process | grep -v grep )
  if [ ${#cmd} -eq 0 ]; then
    write_log "${process} process does not exist, exit ..."
    exit -1
  fi
  write_log "${cmd}"
  sleep 10
done

exit 0

