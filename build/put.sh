#!/bin/bash

# put the previous day's log to hdfs
# crontab file contents :
# @reboot /bin/bash /home/loocha/fanghua/nDPI-httpfilter/build/start.sh >/home/loocha/fanghua/nDPI-httpfilter/build/start.log 2>&1
# 1 * * * * /bin/bash /home/loocha/fanghua/nDPI-httpfilter/build/put.sh >/home/loocha/fanghua/nDPI-httpfilter/build/put.log 2>&1



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


date=$(date -d "-1 day" +'%Y%m%d')

previous_day_log="/var/log/ndpilogs/${date}_*.info"
hdfs_path="/data/ndpilogs"
hdfs_file="${hdfs_path}/${date}_*.info"

write_log "put the previous day's log file to hdfs"
write_log "local file: ${previous_day_log}"
write_log "hdfs file: ${hdfs_file}"

hdfs dfs -put -f ${previous_day_log} ${hdfs_path}

spark-submit "${path}/httptraffic.py" "${hdfs_file}" >> ${log_file} 2>&1
save_file_postfix_list=("_src_traffic"
			"_agent_url_traffic")

for file_postfix  in ${save_file_postfix_list[@]}; do
	hfile="/results/httptraffic/${date}${file_postfix}.csv"
	lpath="/usr/local/src/httptraffic"
	lfile="${lpath}/${date}${file_postfix}.csv"
	hdfs dfs -getmerge ${hfile} ${lfile}
	cd ${lpath}
	tar cvf "${date}${file_postfix}.tar" "${date}${file_postfix}.csv" --remove-files
	rm ".${date}${file_postfix}.csv.crc"
done

write_log "exit the script ..."

