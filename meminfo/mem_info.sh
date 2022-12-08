#!/bin/sh
filename=`date "+%Y-%m-%d_%H:%M:%S"`
mkdir -p /data/processlog
count='0'
tag='='
result="/data/processlog/${filename}.log"
pidnum=`ps|grep -Ev "\[[a-zA-Z].*\]|COMMAND"|awk '{print $1}'`
totalnum=`echo $pidnum | wc -w`
rm  -f $result
for num in $pidnum
do
	let count+=1
	pidpath="/proc/$num"
	tag="${tag}=="
	rate=$((count*100/totalnum))
	flag=`printf "[%-80s%s]" $tag "${rate}%"`
	echo -ne "$flag\r"

	smaps="$pidpath/smaps"
	cmd="$pidpath/cmdline"
	maps="$pidpath/maps"
	status="$pidpath/status"
	oom_score="$pidpath/oom_score"
	! [ -f $smaps -a -f $maps -a -f $status ] && continue 
	echo "===============================================" >> $result
	name=`cat $status | grep "Name:" | awk '{print $2}'`
	path=`cat $maps | head -1 | awk '{print $6}'`
	cmdline=`cat $cmd`
	printf "%-12s%20s\n" "Name:" "$name" >> $result
	printf "%-12s%20s\n" "PATH:" "$path" >> $result
	printf "%-12s%20s\n" "CMD:" "$cmdline" >> $result

	rss=`cat $status | grep VmRSS | awk '{print $2}'`
	pss=`cat $smaps | grep Pss | awk '{a+=$2}END{print a}'`
	vss=`cat $status | grep VmSize | awk '{print $2}'`
	score=`cat $oom_score`
	printf "%-12s%17s KB\n" "RSS:" $rss >> $result
	printf "%-12s%17s KB\n" "PSS:" $pss >> $result
	printf "%-12s%17s KB\n" "VSS:" $vss >> $result
	printf "%-12s%20s\n" "OOM SCORE:" $score >> $result
	
	echo "SharedLib:" >> $result
	cat $maps | grep '\.so' | awk '{print $6}' | sort -u >> $result

done
echo ""
echo "done"
echo "the result file is locate at ${result}"
