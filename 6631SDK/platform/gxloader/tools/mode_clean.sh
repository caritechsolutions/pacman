FILE_LIST="*.c *.S *.h *.lds *.config"

for FILE in ${FILE_LIST}
do 
	find * -name "${FILE}"|xargs chmod -x
done
