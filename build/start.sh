#!/bin/bash

# If you want run the ndpi-httpfilter once at startup, you should add the following string to root user's crontab file
# @reboot /bin/bash /home/loocha/fanghua/nDPI-httpfilter/build/start.sh >/home/loocha/fanghua/nDPI-httpfilter/build/start.log 2>&1
#


path=$(cd $(dirname $0); pwd)
log_file=${path}"/start_detail.log"
cat /dev/null >${log_file}

function write_log(){
    local output=$1
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] ${output}" >> ${log_file} 2>&1
}

write_log "start the script to process start.sh ..."

write_log "restart activemq ..."
/etc/init.d/activemq restart

export ZLOG_PROFILE_DEBUG=/home/loocha/fanghua/nDPI-httpfilter/build/zlog_debug.log
export ZLOG_PROFILE_ERROR=/home/loocha/fanghua/nDPI-httpfilter/build/zlog_error.log

write_log "start run script: ${path}/run.sh"
/bin/bash "${path}/run.sh" start

write_log "exit the script to process start.sh ..."
