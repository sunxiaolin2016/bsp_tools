#!/bin/sh

#addr_bus=1
#addr_bus=3
addr_ds947=0x0c
addr_ds948=0x2c
read_num=0

function usage()
{
	echo "ds9478 [mode]"
	printf "mode:\n 1: read all register ds947/948\n 2: Reset FPD PLL\n 3: Reset OLDI PLL\n 4: Reset OLDI block\n 5: Rest ds947\n "
	printf "6: Disable color bars test on ds947\n 7: Set color bars with external timing clock on 947\n 8: Set color bars with internal clock timing.(1920*720)\n "
	printf "9: Set color bars with internal clock timing.(1920*1080)\n 10: Set color bars with internal clock timing.(960*1280)\n"
}

function read_all_register()
{
	echo "========================================================"
	echo "Start read ds947 register"
#	for((i=0x00; i<=0xff;i++))
	for i in $(seq 0x00 0xff)
	do
		printf "add:0x%x\n" $i
		i2cget -f -y $addr_bus $addr_ds947 $i
	done

	echo "start read ds948 register"
#	for((i=0x00; i<=0xff;i++));
	for i in $(seq 0x00 0xff)
	do
		#echo "obase=16;$i"|bc
		printf "add:0x%x\n" $i
		i2cget -f -y $addr_bus $addr_ds948 $i
	done
	echo "========================================================"
}

function set_external_color_timing_color()
{
	echo "========================================================"
	echo "Enable color bars with external timing clock on 947"
	i2cset -f -y $addr_bus $addr_ds947 0x64 0x05
	echo "========================================================"
}

function Disable_color_bars_test()
{
	echo "========================================================"
	echo "Disable color bars test on 947"
	i2cset -f -y $addr_bus $addr_ds947 0x65 0x00
	i2cset -f -y $addr_bus $addr_ds947 0x64 0x00
	echo "========================================================"
}

function Reset_FPD_pll()
{
	echo "========================================================"
	echo "Reset FPD PLL"
	i2cset -f -y $addr_bus $addr_ds947 0x40 0x14
	i2cset -f -y $addr_bus $addr_ds947 0x41 0x49
	i2cset -f -y $addr_bus $addr_ds947 0x42 0x10
	i2cset -f -y $addr_bus $addr_ds947 0x42 0x00
	echo "========================================================"
}

function Reset_OLDI_pll()
{
	echo "========================================================"
	echo "Reset OLDI PLL"
	i2cset -f -y $addr_bus $addr_ds947 0x40 0x10
	i2cset -f -y $addr_bus $addr_ds947 0x41 0x49
	i2cset -f -y $addr_bus $addr_ds947 0x42 0x10
	i2cset -f -y $addr_bus $addr_ds947 0x42 0x00
	echo "========================================================"
}

function Reset_OLDI_block()
{
	echo "========================================================"
	echo "Reset OLDI block"
	i2cset -f -y $addr_bus $addr_ds947 0x40 0x10
	i2cset -f -y $addr_bus $addr_ds947 0x41 0x49
	i2cset -f -y $addr_bus $addr_ds947 0x42 0x16
	i2cset -f -y $addr_bus $addr_ds947 0x41 0x47
	i2cset -f -y $addr_bus $addr_ds947 0x42 0x20
	i2cset -f -y $addr_bus $addr_ds947 0x42 0xa0
	i2cset -f -y $addr_bus $addr_ds947 0x42 0x20
	i2cset -f -y $addr_bus $addr_ds947 0x42 0x00
	i2cset -f -y $addr_bus $addr_ds947 0x41 0x49
	i2cset -f -y $addr_bus $addr_ds947 0x42 0x00
	echo "========================================================"
}

function Reset_Ds947()
{
	echo "========================================================"
	#patch=`find  /sys/ -name *pdb* `
	#echo "$patch"
	echo "Reset Ds947 pdb"
	echo 1 > "$patch/value"
	sleep 1
	echo 0 > "$patch/value"
	echo "========================================================"
}

function set_internal_clock_timing_1920_720()
{
	echo "========================================================"
	echo "Internal clock timing Pattern(1920 * 720)"
	# 1920*720 test pattern
	#200M / div = pixclk
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x03
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x02

	#h total lowest 8bit 2000= 0x7D0
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x04
	i2cset -f -y $addr_bus $addr_ds947 0x67  0xd0
	# ht most 4bit is 0x7   vt lowest 4bit is  4  vt is 756=0x2f4
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x05
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x47
	#vt most 8bit       756=0x2f4
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x06
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x2f

	#h a lowest 8bit 1920= 0x780
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x07
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x80
	# ha most 4bit is 0x7   vt lowest 4bit is  4  vt is 720=0x2d0
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x08
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x07
	#vt most 8bit       756=0x2d0
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x09
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x2d

	#hsw
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0a
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x48
	#vsw
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0b
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x07
	#hbp
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0c
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x60
	#vbp
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0d
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x0a

	#external clock:0x0c  internal clock:0x04
	i2cset -f -y $addr_bus $addr_ds947 0x65  0x04
	i2cset -f -y $addr_bus $addr_ds947 0x64  0x05
	echo "========================================================"
}

function set_internal_clock_timing_1920_1080()
{
	echo "========================================================"
	echo "Internal clock timing Pattern(1920 * 1080)"
	# 1920*1080 test pattern
	#200M / div = pixclk
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x03
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x02

	#h total lowest 8bit 2000= 0x7D0
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x04
	i2cset -f -y $addr_bus $addr_ds947 0x67  0xd0
	# ht most 4bit is 0x7   vt lowest 4bit is  4  vt is 1188=0x4a4
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x05
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x47
	#vt most 8bit       1188=0x4a4 //756=0x2f4
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x06
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x4a

	#h a lowest 8bit 1920= 0x780
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x07
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x80
	# ha most 4bit is 0x7   vt lowest 4bit is  0  vt is 1080=0x438
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x08
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x87
	#vt most 8bit      1188=0x4a4
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x09
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x4a

	#hsw
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0a
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x0a
	#vsw
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0b
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x08
	#hbp
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0c
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x1e
	#vbp
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0d
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x4e

	#external clock:0x0c  internal clock:0x04
	i2cset -f -y $addr_bus $addr_ds947 0x65  0x04
	i2cset -f -y $addr_bus $addr_ds947 0x64  0x05
	echo "========================================================"
}

function set_internal_clock_timing_960_1280()
{
	echo "========================================================"
	echo "Internal clock timing Pattern(960 * 1280)"
	# 960*1280 test pattern
	#200M / div = pixclk
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x03
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x02

	#h total lowest 8bit 1040= 0x410 //0x7D0
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x04
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x10
	# ht most 4bit is 0x4   vt lowest 4bit is  c  vt is 1388=0x56c
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x05
	i2cset -f -y $addr_bus $addr_ds947 0x67  0xc4
	#vt most 8bit       1388=0x56c  //1188=0x4a4
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x06
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x56

	#h a lowest 8bit 960=0x3c0
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x07
	i2cset -f -y $addr_bus $addr_ds947 0x67  0xc0
	# ha most 4bit is 0x3   vt lowest 4bit is  0  vt is 1280=0x500
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x08
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x03
	#vt most 8bit     1388=0x56c
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x09
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x56

	#hsw
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0a
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x0a
	#vsw
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0b
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x08
	#hbp
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0c
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x1e
	#vbp
	i2cset -f -y $addr_bus $addr_ds947 0x66  0x0d
	i2cset -f -y $addr_bus $addr_ds947 0x67  0x4e

	#external clock:0x0c  internal clock:0x04
	i2cset -f -y $addr_bus $addr_ds947 0x65  0x04
	i2cset -f -y $addr_bus $addr_ds947 0x64  0x05
	echo "========================================================"
}

usage
patch=`find  /sys/ -name *pdb* `
#echo "$patch"
addr_bus=`echo "${patch:39:1}" `
#echo "$addr_bus"

#read -p "enter you choice [1-5]:" choice
case $1 in
1)
	read_all_register;;
#	read_register > ds947-948.txt
#	read -p "press [enter] key to continue..." Key
#	read -n1 "press [enter] key to continue..." ;;
2)
	Reset_FPD_pll;;
#	read -p "press [enter] key to continue..." Key
#	read "press [enter] key to continue..." ;;
3)
	Reset_OLDI_pll;;
#	read -p "press [enter] key to continue..." Key
#	read "press [enter] key to continue..." ;;
4)
	Reset_OLDI_block;;
#	read -p "press [enter] key to continue..." Key
#	read "press [enter] key to continue..." ;;
5)
	Reset_Ds947;;
#	read -p "press [enter] key to continue..." Key
#	read "press [enter] key to continue..." ;;
6)
	Disable_color_bars_test;;
#	read -p "press [enter] key to continue..." Key
#	read "press [enter] key to continue..." ;;
7)
	set_external_color_timing_color;;
#	read -p "press [enter] key to continue..." Key
#	read "press [enter] key to continue..." ;;
8)
	set_internal_clock_timing_1920_720;;
#	read -p "press [enter] key to continue..." Key
#	read "press [enter] key to continue..." ;;
9)
	set_internal_clock_timing_1920_1080;;
#	read -p "press [enter] key to continue..." Key
#	read "press [enter] key to continue..." ;;
10)
	set_internal_clock_timing_960_1280;;
#	read -p "press [enter] key to continue..." Key
#	read "press [enter] key to continue..." ;;
*)
	echo "please enter num[1-10]";;
esac



