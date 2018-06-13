#!/bin/bash
project_name="ndpi-httpfilter"
path=`pwd`
log_file=$path"/run.log"

function write_log() {
  local output=$1
  echo "[`date +'%Y-%m-%d %H:%M:%S'`] $output"
  echo "[`date +'%Y-%m-%d %H:%M:%S'`] $output" >> $log_file 2>&1
}

function status() {
  ps -ef | grep -w $project_name | grep -v grep
  if [ $? -eq 0 ]; then
    write_log "$project_name process is started ..."
  else
    write_log "$project_name process is stopped ..."
  fi
}

function start() {
  while true; do
    ps -ef | grep -w $project_name | grep -v grep
    if [ $? -eq 0 ]; then
      write_log "$project_name process is running ..."
      exit 0
    else
      write_log "no $project_name process, now start it ..."
      ./$project_name &
    fi
    sleep 5
  done
}
  
function stop() {
  write_log "$project_name process is stopping ..."
  killall -s TERM $project_name
}

function restart() {
  write_log "$project_name process is restarting, please wait ..."
  stop
  start
}

case "$1" in
  status)
    status
    ;;
  start)
    start
    ;;
  stop)
    stop
    ;;
  restart)
    restart
    ;;
  *)
    write_log "usage: $0 {status|start|stop|restart}"
    exit 1
esac

# exit and return execute result
exit $?


