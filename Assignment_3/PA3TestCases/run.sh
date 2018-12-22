#!/bin/bash

for (( i=1; i<=9; i++))
	do
		echo "processing  TestCase0$i" 
		cp clean.sh ./"TestCase0$i"/
		cd ./"TestCase0$i"/
		sh clean.sh
		cd ..
		cp ./"TestCase0$i"/input.txt ./
		cp -r ./"TestCase0$i"/inputdir ./
		timeout -s KILL 2 ./votecounter  ./"TestCase0$i"/input.txt  ./"TestCase0$i"/inputdir ./"TestCase0$i"/outputdir > ./"TestCase0$i"/"logstdout_0$i".out 2> ./"TestCase0$i"/"logstdout_0$i".error

	done

for (( i=10; i<=16; i++))
	do
		echo "processing  TestCase$i" 
		cp clean.sh ./"TestCase$i"/
		cd ./"TestCase$i"/
		sh clean.sh
		cd ..
		cp ./"TestCase$i"/input.txt ./
		cp -r ./"TestCase$i"/inputdir ./
		timeout -s KILL 2 ./votecounter  ./"TestCase$i"/input.txt  ./"TestCase$i"/inputdir ./"TestCase$i"/outputdir > ./"TestCase$i"/"logstdout_$i".out 2> ./"TestCase$i"/"logstdout_$i".error
	done

#Extra Credits

j=2
for (( i=17; i<=18; i++))
	do
		echo "processing  TestCase$i - j=$j" 
		cp clean.sh ./"TestCase$i"/
		cd ./"TestCase$i"/
		sh clean.sh
		cd ..
		cp ./"TestCase$i"/input.txt ./
		cp -r ./"TestCase$i"/inputdir ./
		timeout -s KILL 2 ./votecounter  ./"TestCase$i"/input.txt  ./"TestCase$i"/inputdir ./"TestCase$i"/outputdir $j > ./"TestCase$i"/"logstdout_$i".out 2> ./"TestCase$i"/"logstdout_$i".error
		j=$((j-1))
	done


echo "Done!"
