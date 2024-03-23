echo "semi automatic programming script"
echo "repeatedly run the programming command"
count = 0
# counter to keep track of the number of programed chips
while true;
do
    read -t 1 -n 10000
    echo "program success " $count "times"
    echo -en "Press Q to exit or any key to continue\t\t: "
    # most regular keys should work
    read -n 1 -s input
    # read key as soon as it is pressed
    if [[ $input = "q" ]] || [[ $input = "Q" ]] 
    then
        break 
    else 
        echo "programming the next chip"
        # update command to program the chip
        avrdude -Pusb -v -p attiny85 -c usbasp -Ulock:w:0xff:m -Uhfuse:w:0xDF:m -Ulfuse:w:0x62:m -Uefuse:w:0xFF:m -e -D -U flash:w:firmware.hex:i
        avrdudeExitCode=$?
        echo "status: " $avrdudeExitCode
        if [ $avrdudeExitCode = 0 ]
        then
            echo "success"
            # or at least no avrdude error
            ((count++))
        fi
    fi
done
echo ""
