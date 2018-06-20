#!/bin/bash
valgrind -v --log-file=vldcheck.log --tool=memcheck --leak-check=full --show-mismatched-frees=yes --show-reachable=yes --trace-children=yes ./ndpi-httpfilter

~                                                                                                            
~                                               
