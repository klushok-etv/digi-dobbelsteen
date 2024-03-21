echo "semi automatic programing script"
echo "repeatedly run the programing command"
count = 0
# counter to keep track of the number of programed chips
while true;
do
    echo "program sucsess " $count "times"
    echo -en "Press Q to exit or any key to continue\t\t: "
    # most regular keys should work
    read -n 1 -s input
    # read key as soon as it is pressed
    if [[ $input = "q" ]] || [[ $input = "Q" ]] 
    then
        break 
    else 
        echo "programing the next chip"
        # update command to program the chip
        avrdude -p t85 -c usbasp-clone
        avrdudeExitCode=$?
        echo "status: " $avrdudeExitCode
        if [ $avrdudeExitCode = 0 ]
        then
            echo "sucsess"
            # or at least no avrdude error
            ((count++))
        fi
    fi
done
echo ""
