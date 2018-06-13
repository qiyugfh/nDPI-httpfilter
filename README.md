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
chmod 777 run.sh
sudo ./run.sh
sudo ./run.sh start
sudo ./run.sh status
sudo ./run.sh stop
```

### Output
```
cat run.log
cat output/201806/08.log
tail -n 100 output/201806/08/01.info
```
