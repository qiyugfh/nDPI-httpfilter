### Introduce
This is an project that uses the nDPI library to filter http protocol.   
The nDPI source code link: https://github.com/ntop/nDPI  
Its functions include monitoring where the HTTP packets go, analyzing the packet's IP address, Host, user agent, and so on.  


### Compile
Before compiling, you need to ensure that "cmake", "libpcap-dev", "cronolog" are installed on the machine.  
```
cd nDPI-httpfilter/build
cmake ..
make
```

### Run
```
bash run.sh start
bash run.sh status
bash run.sh stop
bash run.sh restart
```

### Output
```
cat run.log
cat output/httpfilter.log
tail -n 100 output/201806/08/01.info
```
