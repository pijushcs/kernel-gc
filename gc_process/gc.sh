nT=190
n=0
pid=$1
while [ $n -lt $nT ]
do
	pmap $1 | grep 'total' | gawk '{print $2}' | gawk -v var=$n 'BEGIN {FS="K"}; {print var" "$1}'
	n=`expr $n + 2`
	sleep 2
done



