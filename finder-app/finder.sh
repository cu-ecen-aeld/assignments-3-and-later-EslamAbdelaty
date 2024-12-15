if [ $# -ne 2 ]
then
    echo invalid numeber of args!
    exit 1
else
    filesdir=$1
    searchstr=$2

    if [ ! -d "$filesdir" ]
    then 
        echo invalid directory!
        exit 1
    else
        echo "The number of files are $(cat $filesdir/*| wc -l)"
        echo "and the number of matching lines are $(cat "$filesdir"/* | grep "$searchstr" | wc -l)"
        exit 0
    fi
fi