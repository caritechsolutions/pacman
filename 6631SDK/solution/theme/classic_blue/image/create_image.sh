#!/bin/bash

echo '<?xml version="1.0" encoding="ISO-8859-1" standalone="yes"?>'>./image.xml
echo "<images>">>./image.xml
#| awk '{print "\t<image id=\"" $0 "\">" $0 "</image>"}'>>./image.xml

NAME=`find . -name "*.[bjB][mpMP]?"`
for i in $NAME ; do
    tmp=${i##*/}
    ID=${tmp%.*}
    FILE=${i#*/}
	echo '	<image id= "'$ID'" >'$FILE'</image>'>>./image.xml
done

NAME=`find . -name "*.[Pp][Nn][Gg]"`
for i in $NAME ; do
    tmp=${i##*/}
    ID=${tmp%.*}
    FILE=${i#*/}
	echo '	<image id= "'$ID'" >'$FILE'</image>'>>./image.xml
done

echo "</images>">>./image.xml
