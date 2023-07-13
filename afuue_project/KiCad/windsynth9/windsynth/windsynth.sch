EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A3 16535 11693
encoding utf-8
Sheet 1 1
Title "AFUUE ver1.9 Main Board"
Date "2023-06-19"
Rev ""
Comp "OtooneDev"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Device:R_POT RV1
U 1 1 622D1E66
P 9950 7950
F 0 "RV1" H 9881 7996 50  0000 R CNN
F 1 "R_POT" H 9881 7905 50  0000 R CNN
F 2 "R1001-N16:R1001-N16" H 9950 7950 50  0001 C CNN
F 3 "~" H 9950 7950 50  0001 C CNN
	1    9950 7950
	1    0    0    -1  
$EndComp
Connection ~ 9200 7250
Wire Wire Line
	6850 8400 7900 8400
Wire Wire Line
	6850 9400 6850 9300
NoConn ~ 9000 8300
NoConn ~ 9000 8100
$Comp
L Connector_Generic:Conn_01x02 J5
U 1 1 61EAE0FF
P 11750 8300
F 0 "J5" H 11830 8292 50  0000 L CNN
F 1 "SpeakerOut" H 11830 8201 50  0000 L CNN
F 2 "UGSM30:USGM30" H 11750 8300 50  0001 C CNN
F 3 "~" H 11750 8300 50  0001 C CNN
	1    11750 8300
	1    0    0    -1  
$EndComp
NoConn ~ 10800 7900
NoConn ~ 11300 7800
Wire Wire Line
	10600 7800 10800 7800
Wire Wire Line
	10600 8300 10600 7800
Wire Wire Line
	11550 8300 10600 8300
Wire Wire Line
	11450 7900 11300 7900
Wire Wire Line
	11450 8400 11450 7900
Wire Wire Line
	11550 8400 11450 8400
Connection ~ 11550 7600
Wire Wire Line
	11550 7700 11550 7600
Wire Wire Line
	11300 7700 11550 7700
Wire Wire Line
	11550 7600 11550 7400
Wire Wire Line
	11300 7600 11550 7600
Wire Wire Line
	9000 7700 9550 7700
Wire Wire Line
	7900 7900 7900 8400
Wire Wire Line
	8400 7900 7900 7900
Wire Wire Line
	8400 7250 8400 7900
Wire Wire Line
	9200 7250 8400 7250
Wire Wire Line
	9200 7600 9200 7250
Wire Wire Line
	9000 7600 9200 7600
$Comp
L Connector_Generic:Conn_01x08 J3
U 1 1 61E679BE
P 8800 7900
F 0 "J3" H 8718 8417 50  0000 C CNN
F 1 "M5StickC" H 8718 8326 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Horizontal" H 8800 7900 50  0001 C CNN
F 3 "~" H 8800 7900 50  0001 C CNN
	1    8800 7900
	-1   0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_02x04_Odd_Even J4
U 1 1 61E66A1E
P 11000 7700
F 0 "J4" H 11050 8017 50  0000 C CNN
F 1 "PAM8012" H 11050 7926 50  0000 C CNN
F 2 "AE-PAM8012:AE-PAM8012" H 11000 7700 50  0001 C CNN
F 3 "~" H 11000 7700 50  0001 C CNN
	1    11000 7700
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x04 J2
U 1 1 61621232
P 7700 7100
F 0 "J2" H 7780 7092 50  0000 L CNN
F 1 "ExtraI2C" H 7780 7001 50  0000 L CNN
F 2 "Connector_PinHeader_2.00mm:PinHeader_1x04_P2.00mm_Vertical" H 7700 7100 50  0001 C CNN
F 3 "~" H 7700 7100 50  0001 C CNN
	1    7700 7100
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x04 J1
U 1 1 61621238
P 7700 8650
F 0 "J1" H 7618 8967 50  0000 C CNN
F 1 "I2C" H 7618 8876 50  0000 C CNN
F 2 "Connector_JST:JST_PH_B4B-PH-SM4-TB_1x04-1MP_P2.00mm_Vertical" H 7700 8650 50  0001 C CNN
F 3 "~" H 7700 8650 50  0001 C CNN
	1    7700 8650
	0    -1   1    0   
$EndComp
$Comp
L power:GND #PWR01
U 1 1 61621592
P 6350 9450
F 0 "#PWR01" H 6350 9200 50  0001 C CNN
F 1 "GND" H 6355 9277 50  0000 C CNN
F 2 "" H 6350 9450 50  0001 C CNN
F 3 "" H 6350 9450 50  0001 C CNN
	1    6350 9450
	1    0    0    -1  
$EndComp
Wire Wire Line
	7350 7700 7700 7700
Wire Wire Line
	7350 6700 7350 7700
Wire Wire Line
	7800 6700 7350 6700
Wire Wire Line
	7500 6600 7850 6600
Wire Wire Line
	7600 7600 7500 7600
Wire Wire Line
	7800 7600 8050 7600
Connection ~ 7900 8400
Wire Wire Line
	7600 8450 7600 7600
Wire Wire Line
	7700 8450 7700 7700
Wire Wire Line
	7800 8450 7800 7900
Wire Wire Line
	7900 8450 7900 8400
Wire Wire Line
	6350 9450 6350 9400
$Comp
L Connector_Generic:Conn_01x05 J8
U 1 1 62C8DF2A
P 12550 7600
F 0 "J8" H 12630 7642 50  0000 L CNN
F 1 "StereoMiniJack" H 12630 7551 50  0000 L CNN
F 2 "HPJack_PJ307:PJ-307 AudioJack" H 12550 7600 50  0001 C CNN
F 3 "~" H 12550 7600 50  0001 C CNN
	1    12550 7600
	1    0    0    -1  
$EndComp
Wire Wire Line
	12350 7800 12150 7800
Wire Wire Line
	12150 7800 12150 8150
Wire Wire Line
	10250 8150 10250 7950
Wire Wire Line
	10250 7950 10100 7950
Wire Wire Line
	12150 7800 12150 7500
Wire Wire Line
	12150 7500 12350 7500
Connection ~ 12150 7800
Wire Wire Line
	10800 7700 10500 7700
Wire Wire Line
	10500 7700 10500 8050
Wire Wire Line
	10500 8050 11750 8050
Wire Wire Line
	11750 8050 11750 7700
Wire Wire Line
	11750 7600 12350 7600
Wire Wire Line
	12350 7700 11750 7700
Connection ~ 11750 7700
Wire Wire Line
	11750 7700 11750 7600
Wire Wire Line
	12350 7400 11550 7400
Connection ~ 11550 7400
Wire Wire Line
	11550 7400 11550 7250
$Comp
L CRD:CRD D1
U 1 1 62D5FF00
P 2550 5000
F 0 "D1" H 2550 4783 50  0000 C CNN
F 1 "CRD" H 2550 4874 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric" H 2550 4825 50  0001 C CNN
F 3 "" H 2550 5000 50  0001 C CNN
	1    2550 5000
	1    0    0    1   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J7
U 1 1 62DA799C
P 5700 4000
F 0 "J7" V 5664 3812 50  0000 R CNN
F 1 "Conn_01x02" V 5573 3812 50  0000 R CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm" H 5700 4000 50  0001 C CNN
F 3 "~" H 5700 4000 50  0001 C CNN
	1    5700 4000
	0    -1   -1   0   
$EndComp
$Comp
L kailh_lowprofile_switch:SW_with_LED SW1
U 1 1 62F0395C
P 2100 4900
F 0 "SW1" H 2105 5267 50  0000 C CNN
F 1 "SW_with_LED" H 2105 5176 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 5000 50  0001 C CNN
F 3 "~" H 2100 5000 50  0001 C CNN
	1    2100 4900
	-1   0    0    -1  
$EndComp
Wire Wire Line
	1800 4800 1600 4800
Wire Wire Line
	1600 4800 1600 5000
Wire Wire Line
	1800 5000 1600 5000
Connection ~ 1600 5000
Wire Wire Line
	1600 5000 1600 5200
$Comp
L kailh_lowprofile_switch:SW_with_LED SW2
U 1 1 62F897C0
P 2100 5300
F 0 "SW2" H 2105 5667 50  0000 C CNN
F 1 "SW_with_LED" H 2105 5576 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 5400 50  0001 C CNN
F 3 "~" H 2100 5400 50  0001 C CNN
	1    2100 5300
	-1   0    0    -1  
$EndComp
Wire Wire Line
	1800 5200 1600 5200
Connection ~ 1600 5200
Wire Wire Line
	1600 5200 1600 5400
Wire Wire Line
	1800 5400 1600 5400
Connection ~ 1600 5400
Wire Wire Line
	1600 5400 1600 5600
$Comp
L kailh_lowprofile_switch:SW_with_LED SW3
U 1 1 62F9E4EF
P 2100 5700
F 0 "SW3" H 2105 6067 50  0000 C CNN
F 1 "SW_with_LED" H 2105 5976 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 5800 50  0001 C CNN
F 3 "~" H 2100 5800 50  0001 C CNN
	1    2100 5700
	-1   0    0    -1  
$EndComp
$Comp
L kailh_lowprofile_switch:SW_with_LED SW4
U 1 1 62F9ED94
P 2100 6100
F 0 "SW4" H 2105 6467 50  0000 C CNN
F 1 "SW_with_LED" H 2105 6376 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 6200 50  0001 C CNN
F 3 "~" H 2100 6200 50  0001 C CNN
	1    2100 6100
	-1   0    0    -1  
$EndComp
$Comp
L kailh_lowprofile_switch:SW_with_LED SW5
U 1 1 62F9F4E4
P 2100 6500
F 0 "SW5" H 2105 6867 50  0000 C CNN
F 1 "SW_with_LED" H 2105 6776 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 6600 50  0001 C CNN
F 3 "~" H 2100 6600 50  0001 C CNN
	1    2100 6500
	-1   0    0    -1  
$EndComp
$Comp
L kailh_lowprofile_switch:SW_with_LED SW6
U 1 1 62F9FE9C
P 2100 6900
F 0 "SW6" H 2105 7267 50  0000 C CNN
F 1 "SW_with_LED" H 2105 7176 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 7000 50  0001 C CNN
F 3 "~" H 2100 7000 50  0001 C CNN
	1    2100 6900
	-1   0    0    -1  
$EndComp
$Comp
L kailh_lowprofile_switch:SW_with_LED SW7
U 1 1 62FA059A
P 2100 7300
F 0 "SW7" H 2105 7667 50  0000 C CNN
F 1 "SW_with_LED" H 2105 7576 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 7400 50  0001 C CNN
F 3 "~" H 2100 7400 50  0001 C CNN
	1    2100 7300
	-1   0    0    -1  
$EndComp
$Comp
L kailh_lowprofile_switch:SW_with_LED SW8
U 1 1 62FA0C65
P 2100 7700
F 0 "SW8" H 2105 8067 50  0000 C CNN
F 1 "SW_with_LED" H 2105 7976 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 7800 50  0001 C CNN
F 3 "~" H 2100 7800 50  0001 C CNN
	1    2100 7700
	-1   0    0    -1  
$EndComp
$Comp
L kailh_lowprofile_switch:SW_with_LED SW9
U 1 1 62FA1561
P 2100 8100
F 0 "SW9" H 2105 8467 50  0000 C CNN
F 1 "SW_with_LED" H 2105 8376 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 8200 50  0001 C CNN
F 3 "~" H 2100 8200 50  0001 C CNN
	1    2100 8100
	-1   0    0    -1  
$EndComp
$Comp
L CRD:CRD D2
U 1 1 62FD4D40
P 2550 5400
F 0 "D2" H 2550 5183 50  0000 C CNN
F 1 "CRD" H 2550 5274 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric" H 2550 5225 50  0001 C CNN
F 3 "" H 2550 5400 50  0001 C CNN
	1    2550 5400
	1    0    0    1   
$EndComp
$Comp
L CRD:CRD D3
U 1 1 62FD5249
P 2550 5800
F 0 "D3" H 2550 5583 50  0000 C CNN
F 1 "CRD" H 2550 5674 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric" H 2550 5625 50  0001 C CNN
F 3 "" H 2550 5800 50  0001 C CNN
	1    2550 5800
	1    0    0    1   
$EndComp
$Comp
L CRD:CRD D4
U 1 1 62FD59E1
P 2550 6200
F 0 "D4" H 2550 5983 50  0000 C CNN
F 1 "CRD" H 2550 6074 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric" H 2550 6025 50  0001 C CNN
F 3 "" H 2550 6200 50  0001 C CNN
	1    2550 6200
	1    0    0    1   
$EndComp
$Comp
L CRD:CRD D5
U 1 1 62FD604D
P 2550 6600
F 0 "D5" H 2550 6383 50  0000 C CNN
F 1 "CRD" H 2550 6474 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric" H 2550 6425 50  0001 C CNN
F 3 "" H 2550 6600 50  0001 C CNN
	1    2550 6600
	1    0    0    1   
$EndComp
$Comp
L CRD:CRD D6
U 1 1 62FD67EA
P 2550 7000
F 0 "D6" H 2550 6783 50  0000 C CNN
F 1 "CRD" H 2550 6874 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric" H 2550 6825 50  0001 C CNN
F 3 "" H 2550 7000 50  0001 C CNN
	1    2550 7000
	1    0    0    1   
$EndComp
$Comp
L CRD:CRD D7
U 1 1 62FD6D7F
P 2550 7400
F 0 "D7" H 2550 7183 50  0000 C CNN
F 1 "CRD" H 2550 7274 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric" H 2550 7225 50  0001 C CNN
F 3 "" H 2550 7400 50  0001 C CNN
	1    2550 7400
	1    0    0    1   
$EndComp
$Comp
L CRD:CRD D8
U 1 1 62FD74C7
P 2550 7800
F 0 "D8" H 2550 7583 50  0000 C CNN
F 1 "CRD" H 2550 7674 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric" H 2550 7625 50  0001 C CNN
F 3 "" H 2550 7800 50  0001 C CNN
	1    2550 7800
	1    0    0    1   
$EndComp
$Comp
L CRD:CRD D9
U 1 1 62FD7F2F
P 2550 8200
F 0 "D9" H 2550 7983 50  0000 C CNN
F 1 "CRD" H 2550 8074 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric" H 2550 8025 50  0001 C CNN
F 3 "" H 2550 8200 50  0001 C CNN
	1    2550 8200
	1    0    0    1   
$EndComp
Wire Wire Line
	6350 9400 6850 9400
$Comp
L kailh_lowprofile_switch:SW_with_LED SW10
U 1 1 62FEC79A
P 2100 8500
F 0 "SW10" H 2105 8867 50  0000 C CNN
F 1 "SW_with_LED" H 2105 8776 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 8600 50  0001 C CNN
F 3 "~" H 2100 8600 50  0001 C CNN
	1    2100 8500
	-1   0    0    -1  
$EndComp
$Comp
L kailh_lowprofile_switch:SW_with_LED SW11
U 1 1 62FECF40
P 2100 9050
F 0 "SW11" H 2105 9417 50  0000 C CNN
F 1 "SW_with_LED" H 2105 9326 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 9150 50  0001 C CNN
F 3 "~" H 2100 9150 50  0001 C CNN
	1    2100 9050
	-1   0    0    -1  
$EndComp
$Comp
L kailh_lowprofile_switch:SW_with_LED SW12
U 1 1 62FED544
P 2100 9450
F 0 "SW12" H 2105 9817 50  0000 C CNN
F 1 "SW_with_LED" H 2105 9726 50  0000 C CNN
F 2 "kailh_lowprofile_switch:KailhLowProfileSW" H 2100 9550 50  0001 C CNN
F 3 "~" H 2100 9550 50  0001 C CNN
	1    2100 9450
	-1   0    0    -1  
$EndComp
$Comp
L CRD:CRD D10
U 1 1 630039E3
P 2550 8600
F 0 "D10" H 2550 8383 50  0000 C CNN
F 1 "CRD" H 2550 8474 50  0000 C CNN
F 2 "LED_SMD:LED_0805_2012Metric" H 2550 8425 50  0001 C CNN
F 3 "" H 2550 8600 50  0001 C CNN
	1    2550 8600
	1    0    0    1   
$EndComp
NoConn ~ 2400 9150
NoConn ~ 2400 9550
NoConn ~ 1800 9550
NoConn ~ 1800 9150
Wire Wire Line
	2700 8600 2850 8600
Wire Wire Line
	2850 8600 2850 8200
Wire Wire Line
	2700 5000 2850 5000
Connection ~ 2850 5000
Wire Wire Line
	2850 5000 2850 4450
Wire Wire Line
	2700 5400 2850 5400
Connection ~ 2850 5400
Wire Wire Line
	2850 5400 2850 5000
Wire Wire Line
	2700 5800 2850 5800
Connection ~ 2850 5800
Wire Wire Line
	2850 5800 2850 5400
Wire Wire Line
	2700 6200 2850 6200
Connection ~ 2850 6200
Wire Wire Line
	2850 6200 2850 5800
Wire Wire Line
	2700 6600 2850 6600
Connection ~ 2850 6600
Wire Wire Line
	2850 6600 2850 6200
Wire Wire Line
	2700 7000 2850 7000
Connection ~ 2850 7000
Wire Wire Line
	2850 7000 2850 6600
Wire Wire Line
	2700 7400 2850 7400
Connection ~ 2850 7400
Wire Wire Line
	2850 7400 2850 7000
Wire Wire Line
	2700 7800 2850 7800
Connection ~ 2850 7800
Wire Wire Line
	2850 7800 2850 7400
Wire Wire Line
	2700 8200 2850 8200
Connection ~ 2850 8200
Wire Wire Line
	2850 8200 2850 7800
Wire Wire Line
	1800 5600 1600 5600
Connection ~ 1600 5600
Wire Wire Line
	1600 5600 1600 5800
Wire Wire Line
	1800 5800 1600 5800
Connection ~ 1600 5800
Wire Wire Line
	1600 5800 1600 6000
Wire Wire Line
	1800 6000 1600 6000
Connection ~ 1600 6000
Wire Wire Line
	1600 6000 1600 6200
Wire Wire Line
	1800 6200 1600 6200
Connection ~ 1600 6200
Wire Wire Line
	1600 6200 1600 6400
Wire Wire Line
	1800 6400 1600 6400
Connection ~ 1600 6400
Wire Wire Line
	1600 6400 1600 6600
Wire Wire Line
	1800 6600 1600 6600
Connection ~ 1600 6600
Wire Wire Line
	1600 6600 1600 6800
Wire Wire Line
	1800 6800 1600 6800
Connection ~ 1600 6800
Wire Wire Line
	1600 6800 1600 7000
Wire Wire Line
	1800 7000 1600 7000
Connection ~ 1600 7000
Wire Wire Line
	1600 7000 1600 7200
Wire Wire Line
	1800 7200 1600 7200
Connection ~ 1600 7200
Wire Wire Line
	1600 7200 1600 7400
Wire Wire Line
	1800 7400 1600 7400
Connection ~ 1600 7400
Wire Wire Line
	1600 7400 1600 7600
Wire Wire Line
	1800 7600 1600 7600
Connection ~ 1600 7600
Wire Wire Line
	1600 7600 1600 7800
Wire Wire Line
	1800 7800 1600 7800
Connection ~ 1600 7800
Wire Wire Line
	1600 7800 1600 8000
Wire Wire Line
	1800 8000 1600 8000
Connection ~ 1600 8000
Wire Wire Line
	1600 8000 1600 8200
Wire Wire Line
	1800 8200 1600 8200
Connection ~ 1600 8200
Wire Wire Line
	1800 9350 1600 9350
Wire Wire Line
	1600 8200 1600 8400
Wire Wire Line
	1800 8400 1600 8400
Connection ~ 1600 8400
Wire Wire Line
	1600 8400 1600 8600
Wire Wire Line
	1800 8600 1600 8600
Connection ~ 1600 8600
Wire Wire Line
	1600 8600 1600 8950
Wire Wire Line
	1800 8950 1600 8950
Connection ~ 1600 8950
Wire Wire Line
	1600 8950 1600 9350
Wire Wire Line
	1600 9350 1600 9850
Wire Wire Line
	5950 9850 5950 9400
Wire Wire Line
	5950 9400 6350 9400
Connection ~ 1600 9350
Connection ~ 6350 9400
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 63157F7B
P 6850 9300
F 0 "#FLG0101" H 6850 9375 50  0001 C CNN
F 1 "PWR_FLAG" H 6850 9473 50  0000 C CNN
F 2 "" H 6850 9300 50  0001 C CNN
F 3 "~" H 6850 9300 50  0001 C CNN
	1    6850 9300
	0    1    -1   0   
$EndComp
Connection ~ 6850 9300
Wire Wire Line
	9950 7800 9950 7250
Connection ~ 9950 7250
Wire Wire Line
	9950 7250 10250 7250
Wire Wire Line
	9950 8200 9950 8100
$Comp
L Connector_Generic:Conn_02x03_Counter_Clockwise J9
U 1 1 62C9FDD8
P 9600 6550
F 0 "J9" H 9650 6867 50  0000 C CNN
F 1 "BleathSensor" H 9650 6776 50  0000 C CNN
F 2 "DIP6_Reverse:DIP-6_W7.62mm_Reverse" H 9600 6550 50  0001 C CNN
F 3 "~" H 9600 6550 50  0001 C CNN
	1    9600 6550
	1    0    0    -1  
$EndComp
Wire Wire Line
	9000 7900 9250 7900
Wire Wire Line
	10050 6550 9900 6550
Wire Wire Line
	9000 8200 9150 8200
Wire Wire Line
	9150 8200 9150 6900
Wire Wire Line
	9150 6900 9950 6900
Wire Wire Line
	9950 6900 9950 6650
Wire Wire Line
	9950 6650 9900 6650
Wire Wire Line
	9900 6450 10250 6450
Wire Wire Line
	10250 6450 10250 7250
Connection ~ 10250 7250
Wire Wire Line
	10250 7250 11550 7250
NoConn ~ 9400 6450
NoConn ~ 9400 6550
NoConn ~ 9400 6650
Wire Wire Line
	7500 6600 7500 7600
Wire Wire Line
	7900 7900 7900 7300
Connection ~ 7900 7900
Wire Wire Line
	7800 7600 7800 7300
Connection ~ 7800 7600
Wire Wire Line
	7700 7700 7700 7300
Connection ~ 7700 7700
Wire Wire Line
	7600 7600 7600 7300
Connection ~ 7600 7600
Wire Wire Line
	5700 4450 5700 4200
Wire Wire Line
	2850 4450 5700 4450
Wire Wire Line
	2400 4800 3800 4800
Wire Wire Line
	2400 5200 3700 5200
Wire Wire Line
	2400 6000 3500 6000
Wire Wire Line
	2400 6400 3400 6400
Wire Wire Line
	2400 6800 4600 6800
Wire Wire Line
	2400 7200 4500 7200
Wire Wire Line
	2400 7600 4400 7600
Wire Wire Line
	2400 8000 4300 8000
Wire Wire Line
	2400 8400 4200 8400
Wire Wire Line
	2400 8950 4100 8950
Wire Wire Line
	2400 9350 4000 9350
Wire Wire Line
	1600 9850 5250 9850
Wire Wire Line
	3800 4200 3800 4800
Wire Wire Line
	3700 4200 3700 5200
Wire Wire Line
	3600 4200 3600 5600
Wire Wire Line
	2400 5600 3600 5600
Wire Wire Line
	3500 4200 3500 6000
Wire Wire Line
	3400 4200 3400 6400
Wire Wire Line
	4600 4200 4600 6800
Wire Wire Line
	4500 4200 4500 7200
Wire Wire Line
	4400 4200 4400 7600
Wire Wire Line
	4300 4200 4300 8000
Wire Wire Line
	4200 8400 4200 4200
Wire Wire Line
	4100 4200 4100 8950
Wire Wire Line
	4000 9350 4000 4200
Wire Wire Line
	5000 3500 6950 3500
Wire Wire Line
	6950 3500 6950 4400
Wire Wire Line
	2800 3500 1600 3500
Wire Wire Line
	1600 3500 1600 4800
Connection ~ 1600 4800
$Comp
L Interface_Expansion:MCP23017_SO U3
U 1 1 630AFEA9
P 3900 3500
F 0 "U3" V 3854 4644 50  0000 L CNN
F 1 "MCP23017_SO" V 3945 4644 50  0000 L CNN
F 2 "Package_SO:SOIC-28W_7.5x17.9mm_P1.27mm" H 4100 2500 50  0001 L CNN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf" H 4100 2400 50  0001 L CNN
	1    3900 3500
	0    1    1    0   
$EndComp
Wire Wire Line
	3300 2800 3200 2800
Wire Wire Line
	1600 2800 1600 3500
Connection ~ 3100 2800
Wire Wire Line
	3100 2800 1600 2800
Connection ~ 3200 2800
Wire Wire Line
	3200 2800 3100 2800
Connection ~ 1600 3500
Wire Wire Line
	4700 2800 7400 2800
Wire Wire Line
	7400 2800 7400 4800
Wire Wire Line
	7400 4800 7800 4800
Wire Wire Line
	7500 4700 7500 2700
Wire Wire Line
	7500 2700 4600 2700
Wire Wire Line
	4600 2700 4600 2800
Wire Wire Line
	7500 4700 7850 4700
Wire Wire Line
	3800 2800 3800 2200
Wire Wire Line
	3800 2200 6950 2200
Wire Wire Line
	6950 2200 6950 3500
Connection ~ 6950 3500
NoConn ~ 4000 2800
NoConn ~ 4100 2800
NoConn ~ 4700 4200
NoConn ~ 3300 4200
NoConn ~ 3200 4200
NoConn ~ 3100 4200
Connection ~ 9150 8200
Wire Wire Line
	9200 7250 9950 7250
$Comp
L Device:CP C1
U 1 1 64A26840
P 9150 8700
F 0 "C1" H 9268 8746 50  0000 L CNN
F 1 "CP" H 9268 8655 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 9188 8550 50  0001 C CNN
F 3 "~" H 9150 8700 50  0001 C CNN
	1    9150 8700
	1    0    0    -1  
$EndComp
$Comp
L Device:CP_Small C2
U 1 1 64A9488F
P 11950 8150
F 0 "C2" V 12175 8150 50  0000 C CNN
F 1 "CP_Small" V 12084 8150 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 11950 8150 50  0001 C CNN
F 3 "~" H 11950 8150 50  0001 C CNN
	1    11950 8150
	0    -1   -1   0   
$EndComp
Wire Wire Line
	12050 8150 12150 8150
Wire Wire Line
	11850 8150 10250 8150
Wire Wire Line
	9000 7800 9350 7800
Wire Wire Line
	9350 7800 9350 8200
Wire Wire Line
	6850 8400 6850 9000
Wire Wire Line
	9150 8850 9150 9000
Wire Wire Line
	9150 9000 6850 9000
Connection ~ 6850 9000
Wire Wire Line
	6850 9000 6850 9300
Wire Wire Line
	9150 8200 9150 8550
$Comp
L Device:R R1
U 1 1 64B9D78D
P 9600 8200
F 0 "R1" V 9807 8200 50  0000 C CNN
F 1 "R" V 9716 8200 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" V 9530 8200 50  0001 C CNN
F 3 "~" H 9600 8200 50  0001 C CNN
	1    9600 8200
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R2
U 1 1 64BB2077
P 9600 8500
F 0 "R2" V 9807 8500 50  0000 C CNN
F 1 "R" V 9716 8500 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" V 9530 8500 50  0001 C CNN
F 3 "~" H 9600 8500 50  0001 C CNN
	1    9600 8500
	0    -1   -1   0   
$EndComp
Wire Wire Line
	9350 8200 9450 8200
Wire Wire Line
	9000 8000 9250 8000
Wire Wire Line
	9250 8000 9250 8500
Wire Wire Line
	9250 8500 9450 8500
Wire Wire Line
	9750 8200 9950 8200
Wire Wire Line
	9750 8500 9950 8500
Wire Wire Line
	9950 8500 9950 8200
Connection ~ 9950 8200
Wire Wire Line
	7850 4700 7850 5400
Wire Wire Line
	7800 4800 7800 5500
$Comp
L Analog_ADC:MCP3425A0T-ECH U1
U 1 1 64CB7050
P 6100 5500
F 0 "U1" H 6100 6081 50  0000 C CNN
F 1 "MCP3425A0T-ECH" H 6100 5990 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-23-6_Handsoldering" H 6100 5500 50  0001 C CIN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/22072b.pdf" H 6100 5500 50  0001 C CNN
	1    6100 5500
	1    0    0    -1  
$EndComp
$Comp
L Analog_ADC:MCP3425A0T-ECH U2
U 1 1 64CC420C
P 6100 6550
F 0 "U2" H 6100 7131 50  0000 C CNN
F 1 "MCP3425A0T-ECH" H 6100 7040 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-23-6_Handsoldering" H 6100 6550 50  0001 C CIN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/22072b.pdf" H 6100 6550 50  0001 C CNN
	1    6100 6550
	1    0    0    -1  
$EndComp
Wire Wire Line
	6600 5500 7800 5500
Connection ~ 7800 5500
Wire Wire Line
	7800 5500 7800 6250
Wire Wire Line
	6600 5400 7850 5400
Connection ~ 7850 5400
Wire Wire Line
	7850 5400 7850 6150
Wire Wire Line
	6600 6450 6950 6450
Wire Wire Line
	6950 6450 6950 6150
Wire Wire Line
	6950 6150 7850 6150
Connection ~ 7850 6150
Wire Wire Line
	7850 6150 7850 6600
Wire Wire Line
	6600 6550 7050 6550
Wire Wire Line
	7050 6550 7050 6250
Wire Wire Line
	7050 6250 7800 6250
Connection ~ 7800 6250
Wire Wire Line
	7800 6250 7800 6700
Wire Wire Line
	5600 6650 5250 6650
Wire Wire Line
	5250 6650 5250 7200
Connection ~ 5250 9850
Wire Wire Line
	5250 9850 5950 9850
Wire Wire Line
	5600 5600 5250 5600
Wire Wire Line
	5250 5600 5250 5900
Connection ~ 5250 6650
Wire Wire Line
	6100 6950 6100 7200
Wire Wire Line
	6100 7200 5250 7200
Connection ~ 5250 7200
Wire Wire Line
	5250 7200 5250 9850
Wire Wire Line
	6100 5900 5250 5900
Connection ~ 5250 5900
Wire Wire Line
	5250 5900 5250 6650
Wire Wire Line
	9150 6900 9150 5850
Wire Wire Line
	9150 5850 8950 5850
Wire Wire Line
	6750 5850 6750 6150
Wire Wire Line
	6750 6150 6100 6150
Connection ~ 9150 6900
Wire Wire Line
	6750 5850 6750 5100
Wire Wire Line
	6750 5100 6100 5100
Connection ~ 6750 5850
$Comp
L power:+3.3V #PWR02
U 1 1 64D60C04
P 8950 5850
F 0 "#PWR02" H 8950 5700 50  0001 C CNN
F 1 "+3.3V" H 8965 6023 50  0000 C CNN
F 2 "" H 8950 5850 50  0001 C CNN
F 3 "" H 8950 5850 50  0001 C CNN
	1    8950 5850
	1    0    0    -1  
$EndComp
Connection ~ 8950 5850
Wire Wire Line
	8950 5850 8500 5850
Wire Wire Line
	5600 5400 5250 5400
Wire Wire Line
	5250 5400 5250 4800
Wire Wire Line
	5250 4800 7100 4800
Wire Wire Line
	7100 4800 7100 5200
Wire Wire Line
	7100 5200 10050 5200
Wire Wire Line
	10050 5200 10050 6550
$Comp
L Connector_Generic:Conn_01x02 J6
U 1 1 64D72818
P 10700 6550
F 0 "J6" H 10780 6542 50  0000 L CNN
F 1 "Conn_01x02" H 10780 6451 50  0000 L CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm" H 10700 6550 50  0001 C CNN
F 3 "~" H 10700 6550 50  0001 C CNN
	1    10700 6550
	1    0    0    -1  
$EndComp
Wire Wire Line
	10050 6550 10500 6550
Connection ~ 10050 6550
Wire Wire Line
	10500 6650 10100 6650
Wire Wire Line
	10100 6650 10100 7050
Wire Wire Line
	9250 7050 9250 7900
Wire Wire Line
	9250 7050 10100 7050
$Comp
L power:PWR_FLAG #FLG02
U 1 1 64D96C76
P 8500 5850
F 0 "#FLG02" H 8500 5925 50  0001 C CNN
F 1 "PWR_FLAG" H 8500 6023 50  0000 C CNN
F 2 "" H 8500 5850 50  0001 C CNN
F 3 "~" H 8500 5850 50  0001 C CNN
	1    8500 5850
	1    0    0    -1  
$EndComp
Connection ~ 8500 5850
Wire Wire Line
	8500 5850 8050 5850
Wire Wire Line
	9550 7700 9550 7600
Wire Wire Line
	9550 7600 10250 7600
Wire Wire Line
	6950 4400 8050 4400
Wire Wire Line
	8050 4400 8050 5850
$Comp
L power:+5V #PWR03
U 1 1 64DE1B01
P 10550 7600
F 0 "#PWR03" H 10550 7450 50  0001 C CNN
F 1 "+5V" H 10565 7773 50  0000 C CNN
F 2 "" H 10550 7600 50  0001 C CNN
F 3 "" H 10550 7600 50  0001 C CNN
	1    10550 7600
	1    0    0    -1  
$EndComp
Connection ~ 10550 7600
Wire Wire Line
	10550 7600 10800 7600
$Comp
L power:PWR_FLAG #FLG01
U 1 1 64DE27A6
P 10250 7600
F 0 "#FLG01" H 10250 7675 50  0001 C CNN
F 1 "PWR_FLAG" H 10250 7773 50  0000 C CNN
F 2 "" H 10250 7600 50  0001 C CNN
F 3 "~" H 10250 7600 50  0001 C CNN
	1    10250 7600
	1    0    0    -1  
$EndComp
Connection ~ 10250 7600
Wire Wire Line
	10250 7600 10400 7600
Connection ~ 8050 5850
Wire Wire Line
	8050 5850 6750 5850
Wire Wire Line
	8050 5850 8050 7600
Wire Wire Line
	5800 4200 10400 4200
Wire Wire Line
	10400 4200 10400 7600
Connection ~ 10400 7600
Wire Wire Line
	10400 7600 10550 7600
$Comp
L Connector_Generic:Conn_02x03_Counter_Clockwise J10
U 1 1 64E4C1A2
P 6400 7800
F 0 "J10" H 6450 8117 50  0000 C CNN
F 1 "BleathSensor" H 6450 8026 50  0000 C CNN
F 2 "DIP6_Reverse:DIP-6_W7.62mm_Reverse" H 6400 7800 50  0001 C CNN
F 3 "~" H 6400 7800 50  0001 C CNN
	1    6400 7800
	1    0    0    -1  
$EndComp
NoConn ~ 6200 7700
NoConn ~ 6200 7800
NoConn ~ 6200 7900
Wire Wire Line
	5600 6450 5500 6450
Wire Wire Line
	5500 6450 5500 7400
Wire Wire Line
	5500 7400 6950 7400
Wire Wire Line
	6950 7400 6950 7800
Wire Wire Line
	6950 7800 6700 7800
Wire Wire Line
	6700 7700 6850 7700
Wire Wire Line
	6850 7700 6850 8400
Connection ~ 6850 8400
Wire Wire Line
	6700 7900 7800 7900
Connection ~ 7800 7900
Wire Wire Line
	7800 7900 7800 7600
$EndSCHEMATC
