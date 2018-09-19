# coding: utf-8


from pyspark import SparkContext
from pyspark.sql import SQLContext
from pyspark.sql.types import *
from pyspark.sql import functions as F
from datetime import datetime, timedelta
import sys
import re


reload(sys)
sys.setdefaultencoding('utf-8')

# "2 pkts/592 bytes -> 0 pkts/0 bytes"
# "1 pkts/66 bytes <-> 2 pkts/299 bytes"
def cal_flow_size(str):
    flow_size = 0L
    if str == None or str == '':
        return flow_size
    strlst = str.split('-> ')
    for i in range(len(strlst)):
        if strlst[i] == '':
            return flow_size
        tmplst = strlst[i].split('/')
        pkt_size = 1L
        for j in range(len(tmplst)):
            val = long(tmplst[j].split(' ')[0])
            pkt_size = pkt_size * val
        flow_size = flow_size + pkt_size
    return flow_size

# "http://short.weixin.qq.com/mmtls/7470d608"
def get_url_host(str):
    if str != None:
        return str[:str.find('/', 7)]


if __name__ != "__main__":
    print("not main function, exit ...")
    exit(0)


log_file = ''
if len(sys.argv) == 2:
    log_file = sys.argv[1]
else:
    date = (datetime.today() - timedelta(days=1)).strftime("%Y%m%d")
    log_file = "/data/ndpilogs/%s_*.info" % date

print("parse log file: %s" % log_file)

date = re.search(r'\/\d{8}_', log_file).group(0).split('_')[0][1:]
save_file = "/results/httptraffic/%s" % date

hdfs_host = "hdfs://localhost:9000"
file1 = "%s%s_src_traffic.csv" % (hdfs_host, save_file)
file2 = "%s%s_agent_url_traffic.csv" % (hdfs_host, save_file)
print("save src traffic file: %s" % file1)
print("save agent url traffic file: %s" % file2)

sc = SparkContext(appName = "HttpTraffic")
sqlContext = SQLContext(sc)

df = sqlContext.read.text(hdfs_host + log_file)
df = df.select(F.split(df.value, ' - ').alias('infos'))
df = df.select(df.infos[0].alias('date_str'), df.infos[1].alias('json_str'))
df = df.select(F.json_tuple(df.json_str, 'src_name', 'flow_size', 'user_agent', 'url'))

# flow_size bytes
df = df.select(df.c0.alias('src_name'), \
               F.udf(cal_flow_size, LongType())(df.c1).alias('flow_size'), \
               df.c2.alias('user_agent'), \
               F.udf(get_url_host, StringType())(df.c3).alias('url'))

df1 = df.groupBy('src_name').sum('flow_size').withColumnRenamed('sum(flow_size)', 'flow_size')
df1 = df1.sort(df1.flow_size.desc())
df1.write.csv(path = file1, mode = "overwrite")

df2 = df.groupBy('user_agent', 'url').sum('flow_size').withColumnRenamed('sum(flow_size)', 'flow_size')
df2 = df2.sort(df2.flow_size.desc())
df2.write.csv(path = file2, mode = "overwrite")

sc.stop()

