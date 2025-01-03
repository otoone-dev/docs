EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Switch:SW_Push SW2
U 1 1 61610A52
P 1950 2300
F 0 "SW2" H 1950 2585 50  0000 C CNN
F 1 "SW_Push" H 1950 2494 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 1950 2500 50  0001 C CNN
F 3 "~" H 1950 2500 50  0001 C CNN
	1    1950 2300
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push SW3
U 1 1 6161124B
P 1950 2700
F 0 "SW3" H 1950 2985 50  0000 C CNN
F 1 "SW_Push" H 1950 2894 50  0000 C CNN
F 2 "CherryMX-Switch:CherryMX-Switch" H 1950 2900 50  0001 C CNN
F 3 "~" H 1950 2900 50  0001 C CNN
	1    1950 2700
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push SW4
U 1 1 616117FA
P 1950 3100
F 0 "SW4" H 1950 3385 50  0000 C CNN
F 1 "SW_Push" H 1950 3294 50  0000 C CNN
F 2 "CherryMX-Switch:CherryMX-Switch" H 1950 3300 50  0001 C CNN
F 3 "~" H 1950 3300 50  0001 C CNN
	1    1950 3100
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push SW5
U 1 1 61611D65
P 1950 3500
F 0 "SW5" H 1950 3785 50  0000 C CNN
F 1 "SW_Push" H 1950 3694 50  0000 C CNN
F 2 "CherryMX-Switch:CherryMX-Switch" H 1950 3700 50  0001 C CNN
F 3 "~" H 1950 3700 50  0001 C CNN
	1    1950 3500
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push SW1
U 1 1 61612297
P 1950 1900
F 0 "SW1" H 1950 2185 50  0000 C CNN
F 1 "SW_Push" H 1950 2094 50  0000 C CNN
F 2 "CherryMX-Switch:CherryMX-Switch" H 1950 2100 50  0001 C CNN
F 3 "~" H 1950 2100 50  0001 C CNN
	1    1950 1900
	1    0    0    -1  
$EndComp
Wire Wire Line
	1450 1900 1550 1900
Wire Wire Line
	1750 2300 1450 2300
Connection ~ 1450 2300
Wire Wire Line
	1450 2300 1450 1900
Wire Wire Line
	1750 2700 1450 2700
Connection ~ 1450 2700
Wire Wire Line
	1450 2700 1450 2300
Wire Wire Line
	1750 3100 1450 3100
Connection ~ 1450 3100
Wire Wire Line
	1450 3100 1450 2700
Wire Wire Line
	1750 3500 1450 3500
Connection ~ 1450 3500
Wire Wire Line
	1450 3500 1450 3100
$Comp
L Switch:SW_Push SW7
U 1 1 6161CF6F
P 1950 4400
F 0 "SW7" H 1950 4685 50  0000 C CNN
F 1 "SW_Push" H 1950 4594 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 1950 4600 50  0001 C CNN
F 3 "~" H 1950 4600 50  0001 C CNN
	1    1950 4400
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push SW8
U 1 1 6161CF75
P 1950 4800
F 0 "SW8" H 1950 5085 50  0000 C CNN
F 1 "SW_Push" H 1950 4994 50  0000 C CNN
F 2 "CherryMX-Switch:CherryMX-Switch" H 1950 5000 50  0001 C CNN
F 3 "~" H 1950 5000 50  0001 C CNN
	1    1950 4800
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push SW9
U 1 1 6161CF7B
P 1950 5200
F 0 "SW9" H 1950 5485 50  0000 C CNN
F 1 "SW_Push" H 1950 5394 50  0000 C CNN
F 2 "CherryMX-Switch:CherryMX-Switch" H 1950 5400 50  0001 C CNN
F 3 "~" H 1950 5400 50  0001 C CNN
	1    1950 5200
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push SW10
U 1 1 6161CF81
P 1950 5600
F 0 "SW10" H 1950 5885 50  0000 C CNN
F 1 "SW_Push" H 1950 5794 50  0000 C CNN
F 2 "CherryMX-Switch:CherryMX-Switch" H 1950 5800 50  0001 C CNN
F 3 "~" H 1950 5800 50  0001 C CNN
	1    1950 5600
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push SW6
U 1 1 6161CF87
P 1950 4000
F 0 "SW6" H 1950 4285 50  0000 C CNN
F 1 "SW_Push" H 1950 4194 50  0000 C CNN
F 2 "CherryMX-Switch:CherryMX-Switch" H 1950 4200 50  0001 C CNN
F 3 "~" H 1950 4200 50  0001 C CNN
	1    1950 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	1450 5700 1450 5650
Wire Wire Line
	1750 4400 1450 4400
Connection ~ 1450 4400
Wire Wire Line
	1450 4400 1450 4000
Wire Wire Line
	1750 4800 1450 4800
Connection ~ 1450 4800
Wire Wire Line
	1450 4800 1450 4400
Wire Wire Line
	1750 5200 1450 5200
Connection ~ 1450 5200
Wire Wire Line
	1450 5200 1450 4800
Wire Wire Line
	1750 5600 1450 5600
Connection ~ 1450 5600
Wire Wire Line
	1450 5600 1450 5200
Connection ~ 1450 5650
Wire Wire Line
	1450 5650 1450 5600
$Comp
L Switch:SW_Push SW12
U 1 1 6162120E
P 1950 6400
F 0 "SW12" H 1950 6685 50  0000 C CNN
F 1 "SW_Push" H 1950 6594 50  0000 C CNN
F 2 "CherryMX-Switch:CherryMX-Switch" H 1950 6600 50  0001 C CNN
F 3 "~" H 1950 6600 50  0001 C CNN
	1    1950 6400
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push SW11
U 1 1 61621226
P 1950 6000
F 0 "SW11" H 1950 6285 50  0000 C CNN
F 1 "SW_Push" H 1950 6194 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 1950 6200 50  0001 C CNN
F 3 "~" H 1950 6200 50  0001 C CNN
	1    1950 6000
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x04 J1
U 1 1 61621238
P 5350 5750
F 0 "J1" H 5268 6067 50  0000 C CNN
F 1 "Conn_01x04" H 5268 5976 50  0000 C CNN
F 2 "Connector_PinHeader_2.00mm:PinHeader_1x04_P2.00mm_Horizontal" H 5350 5750 50  0001 C CNN
F 3 "~" H 5350 5750 50  0001 C CNN
	1    5350 5750
	0    1    1    0   
$EndComp
Wire Wire Line
	5450 5550 5450 5500
Wire Wire Line
	5350 5550 5350 4700
Wire Wire Line
	5250 5550 5250 4800
Wire Wire Line
	5150 5550 5150 4700
Wire Wire Line
	5450 5500 5000 5500
Connection ~ 5450 5500
$Comp
L Interface_Expansion:MCP23S17_SP U1
U 1 1 61623C1B
P 4400 2600
F 0 "U1" V 4354 3744 50  0000 L CNN
F 1 "MCP23S17_SP" V 4445 3744 50  0000 L CNN
F 2 "Package_SO:SOIC-28W_7.5x17.9mm_P1.27mm" H 4600 1600 50  0001 L CNN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf" H 4600 1500 50  0001 L CNN
	1    4400 2600
	-1   0    0    -1  
$EndComp
Wire Wire Line
	4400 3700 5000 3700
Wire Wire Line
	5350 4700 5600 4700
Wire Wire Line
	5600 1500 4400 1500
Connection ~ 5350 4700
Wire Wire Line
	5350 4700 5350 4650
Wire Wire Line
	5150 4700 5050 4700
Wire Wire Line
	5050 4700 5050 3700
Wire Wire Line
	5050 3700 5400 3700
Wire Wire Line
	5400 3700 5400 1900
Wire Wire Line
	5400 1900 5100 1900
Connection ~ 5150 4700
Wire Wire Line
	5150 4700 5150 4650
Wire Wire Line
	5100 2000 5350 2000
Wire Wire Line
	5350 2000 5350 3800
Wire Wire Line
	5350 3800 4900 3800
Wire Wire Line
	4900 3800 4900 4800
Wire Wire Line
	4900 4800 5250 4800
Connection ~ 5250 4800
Wire Wire Line
	5250 4800 5250 4650
Wire Wire Line
	5100 3400 5100 3600
Wire Wire Line
	5100 3600 5000 3600
Wire Wire Line
	5000 3600 5000 3700
Connection ~ 5000 3700
Wire Wire Line
	5100 3300 5200 3300
Wire Wire Line
	5200 3300 5200 3600
Wire Wire Line
	5200 3600 5100 3600
Connection ~ 5100 3600
Wire Wire Line
	5100 3200 5200 3200
Wire Wire Line
	5200 3200 5200 3300
Connection ~ 5200 3300
Wire Wire Line
	5100 2700 5600 2700
NoConn ~ 3700 3400
NoConn ~ 3700 3300
NoConn ~ 3700 3200
NoConn ~ 5100 2500
NoConn ~ 5100 2400
NoConn ~ 5100 2100
NoConn ~ 5100 1800
Wire Wire Line
	1750 6000 1750 5650
Wire Wire Line
	1750 5650 1650 5650
Wire Wire Line
	1750 6400 1650 6400
Wire Wire Line
	1650 6400 1650 5650
Connection ~ 1650 5650
Wire Wire Line
	1650 5650 1450 5650
Connection ~ 5600 2700
Wire Wire Line
	5600 2700 5600 2250
$Comp
L power:+3V3 #PWR04
U 1 1 61758287
P 5600 2250
F 0 "#PWR04" H 5600 2100 50  0001 C CNN
F 1 "+3V3" H 5615 2423 50  0000 C CNN
F 2 "" H 5600 2250 50  0001 C CNN
F 3 "" H 5600 2250 50  0001 C CNN
	1    5600 2250
	0    1    -1   0   
$EndComp
Connection ~ 5600 2250
Wire Wire Line
	5600 2250 5600 1900
$Comp
L Connector_Generic:Conn_01x04 J2
U 1 1 61621232
P 5350 4450
F 0 "J2" H 5430 4442 50  0000 L CNN
F 1 "Conn_01x04" H 5430 4351 50  0000 L CNN
F 2 "Connector_PinHeader_2.00mm:PinHeader_1x04_P2.00mm_Horizontal" H 5350 4450 50  0001 C CNN
F 3 "~" H 5350 4450 50  0001 C CNN
	1    5350 4450
	0    1    -1   0   
$EndComp
$Comp
L power:PWR_FLAG #FLG01
U 1 1 617FB038
P 5600 1900
F 0 "#FLG01" H 5600 1975 50  0001 C CNN
F 1 "PWR_FLAG" H 5600 2073 50  0000 C CNN
F 2 "" H 5600 1900 50  0001 C CNN
F 3 "~" H 5600 1900 50  0001 C CNN
	1    5600 1900
	0    1    -1   0   
$EndComp
Connection ~ 5600 1900
Wire Wire Line
	5600 1900 5600 1500
$Comp
L power:PWR_FLAG #FLG03
U 1 1 617FDF2C
P 1550 1900
F 0 "#FLG03" H 1550 1975 50  0001 C CNN
F 1 "PWR_FLAG" H 1550 2073 50  0000 C CNN
F 2 "" H 1550 1900 50  0001 C CNN
F 3 "~" H 1550 1900 50  0001 C CNN
	1    1550 1900
	1    0    0    -1  
$EndComp
Connection ~ 1550 1900
$Comp
L power:GND #PWR01
U 1 1 61621592
P 1450 5700
F 0 "#PWR01" H 1450 5450 50  0001 C CNN
F 1 "GND" H 1455 5527 50  0000 C CNN
F 2 "" H 1450 5700 50  0001 C CNN
F 3 "" H 1450 5700 50  0001 C CNN
	1    1450 5700
	1    0    0    -1  
$EndComp
Wire Wire Line
	1450 4000 1750 4000
Wire Wire Line
	1450 3500 1450 4000
Connection ~ 1450 4000
Wire Wire Line
	2700 2700 2700 1900
Wire Wire Line
	2700 1900 2150 1900
Wire Wire Line
	2700 2700 3700 2700
Wire Wire Line
	2650 2800 2650 2300
Wire Wire Line
	2650 2300 2150 2300
Wire Wire Line
	2650 2800 3700 2800
Wire Wire Line
	2600 2900 2600 2700
Wire Wire Line
	2600 2700 2150 2700
Wire Wire Line
	2600 2900 3700 2900
Wire Wire Line
	2600 3000 2600 3100
Wire Wire Line
	2600 3100 2150 3100
Wire Wire Line
	2600 3000 3700 3000
Wire Wire Line
	2650 3100 2650 3500
Wire Wire Line
	2650 3500 2150 3500
Wire Wire Line
	2650 3100 3700 3100
Wire Wire Line
	2800 4000 2150 4000
Wire Wire Line
	2850 4400 2150 4400
Wire Wire Line
	2900 4800 2150 4800
Wire Wire Line
	2950 5200 2150 5200
Wire Wire Line
	3000 5600 2150 5600
Wire Wire Line
	3050 6000 2150 6000
Wire Wire Line
	3100 6400 2150 6400
Wire Wire Line
	5600 2700 5600 4700
Wire Wire Line
	5450 4650 5450 5500
Wire Wire Line
	1650 6400 1650 6500
Wire Wire Line
	1650 6500 4400 6500
Wire Wire Line
	4400 6500 4400 3700
Connection ~ 1650 6400
Connection ~ 4400 3700
Wire Wire Line
	1550 1900 1750 1900
Wire Wire Line
	5000 3700 5000 5500
Wire Wire Line
	3100 6400 3100 2500
Wire Wire Line
	3100 2500 3700 2500
Wire Wire Line
	3050 6000 3050 2400
Wire Wire Line
	3050 2400 3700 2400
Wire Wire Line
	3000 5600 3000 2300
Wire Wire Line
	3000 2300 3700 2300
Wire Wire Line
	2950 5200 2950 2200
Wire Wire Line
	2950 2200 3700 2200
Wire Wire Line
	2900 4800 2900 2100
Wire Wire Line
	2900 2100 3700 2100
Wire Wire Line
	2850 4400 2850 2000
Wire Wire Line
	2850 2000 3700 2000
Wire Wire Line
	2800 4000 2800 1900
Wire Wire Line
	2800 1900 3700 1900
NoConn ~ 3700 1800
$EndSCHEMATC
