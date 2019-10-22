EESchema Schematic File Version 4
LIBS:FIR-sensor-addon-cache
EELAYER 26 0
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
L KAIOTE_MELEXIS:MLX90640 U1
U 1 1 5C74E312
P 7200 3750
F 0 "U1" H 7600 3500 50  0000 C CNN
F 1 "MLX90640" H 7750 3400 50  0000 C CNN
F 2 "lib_kaiote:MLX90640" H 7200 3750 50  0001 C CNN
F 3 "https://www.mouser.com/datasheet/2/734/MLX90640-Datasheet-Melexis-1324357.pdf" H 7200 3750 50  0001 C CNN
F 4 "MLX90640ESF-BAA-000-SP" H 7200 3750 50  0001 C CNN "manf#"
F 5 "mouser" H 0   0   50  0001 C CNN "vendor"
	1    7200 3750
	1    0    0    -1  
$EndComp
Wire Wire Line
	4300 3700 4150 3700
Text Label 4150 3700 0    50   ~ 0
SCL
$Comp
L Connector_Generic:Conn_02x03_Odd_Even J1
U 1 1 5C772666
P 4500 3800
F 0 "J1" H 4550 4117 50  0000 C CNN
F 1 "Conn_02x03_Odd_Even" H 4550 4026 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Vertical" H 4500 3800 50  0001 C CNN
F 3 "https://cdn.amphenol-icc.com/media/wysiwyg/files/drawing/68000.pdf" H 4500 3800 50  0001 C CNN
F 4 "609-3263-ND" H 4500 3800 50  0001 C CNN "manf#"
F 5 "digikey" H 0   0   50  0001 C CNN "vendor"
	1    4500 3800
	1    0    0    -1  
$EndComp
Wire Wire Line
	4800 3700 4950 3700
Text Label 4950 3700 2    50   ~ 0
SDA
$Comp
L power:+3.3V #PWR0101
U 1 1 5C7726EA
P 4300 3800
F 0 "#PWR0101" H 4300 3650 50  0001 C CNN
F 1 "+3.3V" H 4150 3850 50  0000 C CNN
F 2 "" H 4300 3800 50  0001 C CNN
F 3 "" H 4300 3800 50  0001 C CNN
	1    4300 3800
	1    0    0    -1  
$EndComp
NoConn ~ 4800 3800
NoConn ~ 4300 3900
$Comp
L power:GNDREF #PWR0102
U 1 1 5C772799
P 6400 4000
F 0 "#PWR0102" H 6400 3750 50  0001 C CNN
F 1 "GNDREF" H 6405 3827 50  0000 C CNN
F 2 "" H 6400 4000 50  0001 C CNN
F 3 "" H 6400 4000 50  0001 C CNN
	1    6400 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	4900 3950 4900 3900
Wire Wire Line
	4900 3900 4800 3900
Wire Wire Line
	6750 3650 6400 3650
Wire Wire Line
	6400 4000 6400 3950
Wire Wire Line
	6400 3950 6750 3950
$Comp
L Device:C C2
U 1 1 5C7728F1
P 6400 3800
F 0 "C2" H 6515 3846 50  0000 L CNN
F 1 "1uF/6.3v(10%)/0805" H 6400 4100 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 6438 3650 50  0001 C CNN
F 3 "http://www.yageo.com/documents/recent/UPY-GPHC_X7R_6.3V-to-50V_18.pdf" H 6400 3800 50  0001 C CNN
F 4 " 311-3418-1-ND" H 6400 3800 50  0001 C CNN "manf#"
F 5 "digikey" H 6400 3800 50  0001 C CNN "vendor"
	1    6400 3800
	1    0    0    -1  
$EndComp
Connection ~ 6400 3950
Wire Wire Line
	6400 3650 6100 3650
Connection ~ 6400 3650
$Comp
L Device:C C1
U 1 1 5C772950
P 6100 3800
F 0 "C1" H 6215 3846 50  0000 L CNN
F 1 "0.1uF/25V(10%)/0805" H 5450 3600 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 6138 3650 50  0001 C CNN
F 3 "http://www.yageo.com/documents/recent/UPY-GPHC_X7R_6.3V-to-50V_18.pdf" H 6100 3800 50  0001 C CNN
F 4 "digikey" H 6100 3800 50  0001 C CNN "vendor"
F 5 "311-1141-1-ND" H 6100 3800 50  0001 C CNN "manf#"
	1    6100 3800
	1    0    0    -1  
$EndComp
Wire Wire Line
	6400 3950 6100 3950
Wire Wire Line
	6100 3650 5900 3650
Connection ~ 6100 3650
$Comp
L power:+3.3V #PWR0103
U 1 1 5C772A28
P 5900 3650
F 0 "#PWR0103" H 5900 3500 50  0001 C CNN
F 1 "+3.3V" H 5750 3700 50  0000 C CNN
F 2 "" H 5900 3650 50  0001 C CNN
F 3 "" H 5900 3650 50  0001 C CNN
	1    5900 3650
	1    0    0    -1  
$EndComp
$Comp
L power:GNDREF #PWR0104
U 1 1 5C772A3B
P 4900 3950
F 0 "#PWR0104" H 4900 3700 50  0001 C CNN
F 1 "GNDREF" H 4905 3777 50  0000 C CNN
F 2 "" H 4900 3950 50  0001 C CNN
F 3 "" H 4900 3950 50  0001 C CNN
	1    4900 3950
	1    0    0    -1  
$EndComp
Wire Wire Line
	7650 3650 7850 3650
Text Label 7850 3650 2    50   ~ 0
SDA
Wire Wire Line
	7650 3950 7850 3950
Text Label 7850 3950 2    50   ~ 0
SCL
$Comp
L Mechanical:MountingHole H1
U 1 1 5C7DF950
P 8650 3450
F 0 "H1" H 8750 3496 50  0000 L CNN
F 1 "MountingHole" H 8750 3405 50  0000 L CNN
F 2 "MountingHole:MountingHole_3.2mm_M3" H 8650 3450 50  0001 C CNN
F 3 "~" H 8650 3450 50  0001 C CNN
	1    8650 3450
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H2
U 1 1 5C7DF986
P 8650 3650
F 0 "H2" H 8750 3696 50  0000 L CNN
F 1 "MountingHole" H 8750 3605 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.2mm_M2" H 8650 3650 50  0001 C CNN
F 3 "~" H 8650 3650 50  0001 C CNN
	1    8650 3650
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H3
U 1 1 5C7DF9B4
P 8650 3850
F 0 "H3" H 8750 3896 50  0000 L CNN
F 1 "MountingHole" H 8750 3805 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.2mm_M2" H 8650 3850 50  0001 C CNN
F 3 "~" H 8650 3850 50  0001 C CNN
	1    8650 3850
	1    0    0    -1  
$EndComp
$EndSCHEMATC
