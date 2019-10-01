bash_pid=$$
children=`ps -eo ppid | grep -w $bash_pid`
num_children=`echo $children | wc -w`
let num_children=num_children-1

echo $num_children
