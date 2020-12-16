#! /bin/sh
set -u
#set -x

# scripts for lvds
function help_lvds()
{
	cat << EOF
lvds helpers:
1. find pdb pin directory: 		find /sys/ -name *pdb*
2. 947 pdb control of da directory:	/sys/devices/platform/57247000.i2c/i2c-3/3-000c/ds947_pdb
3. 947 device address on i2c: 0x0c
4. 948 device address on i2c: 0x2c
EOF
}

# dump ds947 and ds948 registers
function dump_ds9478()
{
	local i2c_bus=3
	local value=0
	echo "start read ds947 register"

	#for((i=0x00; i<=0xff;i++)); 
	for i in $(seq 0 255)
	do
		value=$(i2cget -f -y $i2c_bus 0x0c $i)
		printf "add:0x%x = 0x%x\n" $i $value
		#i2cget -f -y $i2c_bus 0x0c $i
	done

	echo "start read ds948 register"
	for i in $(seq 0 255)
	do  
		#echo "obase=16;$i"|bc	
		value=$(i2cget -f -y $i2c_bus 0x2c $i)
		printf "add:0x%x = 0x%x\n" $i $value
		#i2cget -f -y $i2c_bus 0x2c $i
	done 
}

#scripts for reset ds947
function reset_947()
{
	echo 1 > /sys/devices/platform/57247000.i2c/i2c-3/3-000c/ds947_pdb/value
	sleep 1
	echo 0 > /sys/devices/platform/57247000.i2c/i2c-3/3-000c/ds947_pdb/value
}

#scripts for display pannel of mcu
function help_mcu()
{
	cat << EOF
mcu helpers:
1. debug mcu logs:			echo 1 > /sys/module/mcu_driver/parameters/mcu_debug
2. cat last mcu status:			cat /proc/tcs_mcu/mcu
3. debug mcu i2c communication:		cat /proc/tcs_mcu/test_i2c
EOF
}

#scripts for tja110x phy
function help_tja1101x()
{
	cat << EOF
tja1101x helpers:
1. tja1101x have two phys on d532, phy address are 0x4 and 0x6:
	5b040000.ethernet-1:06 <--> phy address 0x6 <--> SJA1105P_p2 <--> tcu
	5b040000.ethernet-1:04 <--> phy address 0x4 <--> SJA1105P_p1 <--> dvr

2. change to master config: 	
	/sys/bus/mdio_bus/devices/5b040000.ethernet-1:06/configuration/master_cfg
	/sys/bus/mdio_bus/devices/5b040000.ethernet-1:04/configuration/master_cfg

2.1 set phy address 6 to master:
	echo 1 > /sys/bus/mdio_bus/devices/5b040000.ethernet-1:06/configuration/master_cfg
2.2 set phy address 6 to slave:
	echo 0 > /sys/bus/mdio_bus/devices/5b040000.ethernet-1:06/configuration/master_cfg

2.3 set phy address 4 to master:
	echo 1 > /sys/bus/mdio_bus/devices/5b040000.ethernet-1:04/configuration/master_cfg
2.4 set phy address 4 to slave:
	echo 0 > /sys/bus/mdio_bus/devices/5b040000.ethernet-1:04/configuration/master_cfg

3. access phy registers: refers mii -h
	mii -d 6 -r 0: read register 0 of phy address 6(SJA1105P_p2)
	mii -d 4 -r 0: read register 0 of phy address 4(SJA1105P_p1)

4. watch ethernet interface link status: refer mii-tool -h
	mii-tool -w SJA1105P_p2
	mii-tool -w SJA1105P_p1
EOF
}

#script to dump tja1101x phy registers
function dump_tja1101x()
{
	echo "dump phyaddress 0x6:"
	for i in $(seq 0 28); do mii -d6 -r $i; done
	echo "dump phyaddress 0x4:"
	for i in $(seq 0 28); do mii -d4 -r $i; done
}

#scripts for switch sja1105p
function help_sja1105p()
{
	cat << EOF
sja1105p helpers:
1. static mac level errors:
	cat /sys/kernel/debug/sja1105p-0/ethernet/mac-level
2. static stack level errors:
	cat /sys/kernel/debug/sja1105p-0/ethernet/high-level
3. other infomations refer ethtool -h
EOF
}

function help_i2ctool()
{
	cat << EOF
i2ctransfer usage: (refer i2ctransfer -h)
./i2ctransfer                                     
Usage: i2ctransfer [-f] [-y] [-v] [-V] [-a] I2CBUS DESC [DATA] [DESC [DATA]]...
  I2CBUS is an integer or an I2C bus name
  DESC describes the transfer in the form: {r|w}LENGTH[@address]
    1) read/write-flag 2) LENGTH (range 0-65535) 3) I2C address (use last one if omitted)
  DATA are LENGTH bytes for a write message. They can be shortened by a suffix:
    = (keep value constant until LENGTH)
    + (increase value by 1 until LENGTH)
    - (decrease value by 1 until LENGTH)
    p (use pseudo random generator until LENGTH with value as seed)

Example (bus 0, read 8 byte at offset 0x64 from EEPROM at 0x50):
  # i2ctransfer 0 w1@0x50 0x64 r8
Example (same EEPROM, at offset 0x42 write 0xff 0xfe ... 0xf0):
  # i2ctransfer 0 w17@0x50 0x42 0xff-

Example read mcu of d532:
	./i2ctransfer -f -y 3 w2@0x28  0x00 0x00 r1
			    | ||  |     |    |   ||
			    | ||  |	|    |	 |-:number of byte to read 
			    | ||  |	|    |   r: read number of bytes.
			    | ||  |     |    ----register address sequence 2
			    | ||  |     ---------register address sequence 1
			    | ||  ---------------slave address on i2c bus.
			    | |------------------how many bytes to read or write, 
			    | |			 here 2 means write 0x00 0x00 after parameter w2@0x28.
			    | -------------------read or write operation of bytes after w2@0x28 format,
			    | 			 here is write, and write 2 bytes register address data 0x00,0x00
			    ---------------------i2c bus number
	e.g: reading backlight brightness of register 0x0100, mcu will return 1 byte data:
	./i2ctransfer -f -y 3 w2@0x28  0x01 0x00 r1
Example write backlight briteness of register 0x0100 to 0x5a(90):
	./i2ctransfer -f -y 3 w3@0x28  0x01 0x00 0x5a
			       |
			       -----------------write 3bytes: 2bytes reigster address(0x01 0x00), 1bytes data(0x5a)
	and read out brightness value again:
	/i2ctransfer -f -y 3 w2@0x28  0x01 0x00 r1
EOF
}
cat << EOF
usage: . this_file_name
and then you can use script functions defined by this scripts, like:
panda_imx8qm:/ $ su
panda_imx8qm:/ # dump_tja1101x
EOF
