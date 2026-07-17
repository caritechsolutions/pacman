#!bin/sh


for i in `ls *.xml`
do
sed -i "s/hcenter/hcentre/g" $i
sed -i "s/vcenter/vcentre/g" $i
sed -i "s/vcentre|hcentre/hcentre|vcentre/g" $i
done
