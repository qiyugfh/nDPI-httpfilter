#!/bin/bash

# put the previous hour's log to hdfs



path=$(cd $(dirname $0); pwd)
log_file=${path}"/put_detail.log"
cat /dev/null > ${log_file}

function write_log(){	
	local output=$1
#	echo "[$(date +'%Y-%m-%d %H:%M:S')] ${output}"
	echo "[$(date +'%Y-%m-%d %H:%M:S')] ${output}" >> ${log_file} 2>&1
}

write_log "start the script ..."

export JAVA_HOME=/usr/java/jdk1.8.0_161
export JRE_HOME=${JAVA_HOME}/jre
export CLASSPATH=${JAVA_HOME}/lib
export HADOOP_HOME=/opt/distribution/hadoop
export SPARK_HOME=/opt/distribution/spark
export PATH=${JAVA_HOME}/bin:${HADOOP_HOME}/bin:${SPARK_HOME}/bin:${SPARK_HOME}/sbin:$PATH
write_log "$(echo $PATH)"


date=$(date -d "-1 hour" +'%Y%m%d %H')
month=${date:0:6}
day=${date:6:2}
hour=${date:0-2:2}

previous_hour_log="/var/log/ndpilogs/${month}/${day}/${hour}.info"
hdfs_file="/data/ndpilogs/${month}/${day}/${hour}.info"

write_log "put the previous hour's log file to hdfs"
write_log "local file: ${previous_hour_log}"
write_log "hdfs file: ${hdfs_file}"

hdfs dfs -put ${previous_hour_log} ${hdfs_file}

spark-submit "${path}/httptraffic.py" "${hdfs_file}" >> ${log_file} 2>&1
save_file_postfix_list=("_src_traffic"
			"_agent_url_traffic")

for file_postfix  in ${save_file_postfix_list[@]}; do
	hfile="/results/httptraffic/${month}/${day}/${hour}${file_postfix}.csv"
	lpath="/usr/local/src/httptraffic/${month}/${day}"
	lfile="${lpath}/${hour}${file_postfix}.csv"
	hdfs dfs -getmerge ${hfile} ${lfile}
	cd ${lpath}
	tar cvf "${hour}${file_postfix}.tar" "${hour}${file_postfix}.csv" --remove-files
	rm ".${hour}${file_postfix}.csv.crc"
done

write_log "exit the script ..."

