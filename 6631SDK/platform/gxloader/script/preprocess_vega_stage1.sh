#!/bin/bash

input_file=$1
output_file=$2
openssl enc -d -aes-128-cbc -in ${input_file} -out ${output_file} -K fd700bd8d3b1e8527efecb2164437eb8 -iv 00000000000000000000000000000000 -nopad -nosalt
