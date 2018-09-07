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


export JAVA_HOME=/usr/java/jdk1.8.0_161
export JRE_HOME=${JAVA_HOME}/jre
export CLASSPATH=${JAVA_HOME}/lib
export HADOOP_HOME=/opt/distribution/hadoop
export SPARK_HOME=/opt/distribution/spark
export PATH=${JAVA_HOME}/bin:${HADOOP_HOME}/bin:${SPARK_HOME}/bin:${SPARK_HOME}/sbin:$PATH

export ZLOG_PROFILE_DEBUG="${path}/zlog_debug.log"
export ZLOG_PROFILE_ERROR="${path}/zlog_error.log"


rm *.log
write_log "start the script to process start.sh ..."

/sbin/ifconfig eno1 promisc

/bin/bash /opt/distribution/hadoop/sbin/start-dfs.sh

/bin/bash /home/loocha/fanghua/nDPI-httpfilter/build/put.sh

write_log "restart activemq ..."
/etc/init.d/activemq restart

write_log "start run script: ${path}/run.sh"
/bin/bash "${path}/run.sh" start

write_log "exit the script to process start.sh ..."
