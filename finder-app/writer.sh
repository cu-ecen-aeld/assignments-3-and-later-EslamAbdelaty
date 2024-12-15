if [ $# -ne 2 ]
then
    echo invalid numeber of args!
    exit 1
else
    writefile=$1
    writestr=$2
    mkdir -p "$(dirname "$writefile")"
    touch "$writefile"
    echo "$writestr" > "$writefile"
    exit 0
    
fi