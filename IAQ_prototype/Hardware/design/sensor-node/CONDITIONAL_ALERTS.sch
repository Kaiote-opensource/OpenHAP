EESchema Schematic File Version 4
LIBS:sensor-node-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 2 3
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Wire Notes Line
	475  2605 11220 2605
Wire Notes Line
	470  4860 11220 4860
Wire Notes Line
	11220 4860 11220 4855
$Comp
L Transistor_FET:BSS138 Q?
U 1 1 5CE53565
P 4000 3900
AR Path="/5CE53565" Ref="Q?"  Part="1" 
AR Path="/5C79219D/5CE53565" Ref="Q5"  Part="1" 
F 0 "Q5" H 4205 3946 50  0000 L CNN
F 1 "BSS138" H 4205 3855 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 4200 3825 50  0001 L CIN
F 3 "https://www.onsemi.com/pub/Collateral/BSS138-D.PDF" H 4000 3900 50  0001 L CNN
F 4 "digikey" H 50  100 50  0001 C CNN "vendor"
F 5 "BSS138CT-ND" H 50  100 50  0001 C CNN "manf#"
	1    4000 3900
	1    0    0    -1  
$EndComp
Wire Wire Line
	3800 3900 3750 3900
$Comp
L power:GND #PWR043
U 1 1 5CE535D0
P 4100 4300
F 0 "#PWR043" H 4100 4050 50  0001 C CNN
F 1 "GND" H 4105 4127 50  0000 C CNN
F 2 "" H 4100 4300 50  0001 C CNN
F 3 "" H 4100 4300 50  0001 C CNN
	1    4100 4300
	1    0    0    -1  
$EndComp
Wire Wire Line
	4100 4300 4100 4200
Text HLabel 3100 3900 0    50   Input ~ 0
I_BUZZER
$Comp
L Transistor_FET:BSS138 Q?
U 1 1 5CE53CCE
P 7850 3850
AR Path="/5CE53CCE" Ref="Q?"  Part="1" 
AR Path="/5C79219D/5CE53CCE" Ref="Q6"  Part="1" 
F 0 "Q6" H 8055 3896 50  0000 L CNN
F 1 "BSS138" H 8055 3805 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 8050 3775 50  0001 L CIN
F 3 "https://www.onsemi.com/pub/Collateral/BSS138-D.PDF" H 7850 3850 50  0001 L CNN
F 4 "digikey" H 1900 -50 50  0001 C CNN "vendor"
F 5 "BSS138CT-ND" H 1900 -50 50  0001 C CNN "manf#"
	1    7850 3850
	1    0    0    -1  
$EndComp
Wire Wire Line
	7650 3850 7600 3850
$Comp
L power:GND #PWR044
U 1 1 5CE53CD6
P 7950 4150
F 0 "#PWR044" H 7950 3900 50  0001 C CNN
F 1 "GND" H 7955 3977 50  0000 C CNN
F 2 "" H 7950 4150 50  0001 C CNN
F 3 "" H 7950 4150 50  0001 C CNN
	1    7950 4150
	1    0    0    -1  
$EndComp
Wire Wire Line
	7950 3650 7950 3400
Wire Wire Line
	7950 3400 7800 3400
Text HLabel 7800 3400 0    50   Input ~ 0
WARN_LED
Text HLabel 7000 3850 0    50   Input ~ 0
I_WARN
$Comp
L Device:R_US R?
U 1 1 5CE6A313
P 3200 4050
AR Path="/5CE6A313" Ref="R?"  Part="1" 
AR Path="/5C79219D/5CE6A313" Ref="R24"  Part="1" 
F 0 "R24" H 3000 4100 50  0000 L CNN
F 1 "10k(5%)" H 2850 4000 50  0000 L CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 3240 4040 50  0001 C CNN
F 3 "http://www.yageo.com/documents/recent/PYu-RC_Group_51_RoHS_L_10.pdf" H 3200 4050 50  0001 C CNN
F 4 "digikey" H 50  50  50  0001 C CNN "vendor"
F 5 "311-10KARCT-ND" H 50  50  50  0001 C CNN "manf#"
	1    3200 4050
	1    0    0    -1  
$EndComp
Connection ~ 3750 3900
Wire Wire Line
	3750 3900 3200 3900
Connection ~ 4100 4200
Wire Wire Line
	4100 4200 4100 4100
$Comp
L Device:R_US R?
U 1 1 5CE6A639
P 7050 4000
AR Path="/5CE6A639" Ref="R?"  Part="1" 
AR Path="/5C79219D/5CE6A639" Ref="R25"  Part="1" 
F 0 "R25" H 6850 4050 50  0000 L CNN
F 1 "10k(5%)" H 6700 3950 50  0000 L CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 7090 3990 50  0001 C CNN
F 3 "http://www.yageo.com/documents/recent/PYu-RC_Group_51_RoHS_L_10.pdf" H 7050 4000 50  0001 C CNN
F 4 "digikey" H 1900 -100 50  0001 C CNN "vendor"
F 5 "311-10KARCT-ND" H 1900 -100 50  0001 C CNN "manf#"
	1    7050 4000
	1    0    0    -1  
$EndComp
Connection ~ 7600 3850
Wire Wire Line
	7600 3850 7050 3850
Connection ~ 7950 4150
Wire Wire Line
	7950 4050 7950 4150
Connection ~ 3200 3900
Wire Wire Line
	3200 3900 3100 3900
Connection ~ 7050 3850
Wire Wire Line
	7050 3850 7000 3850
$Comp
L Device:Buzzer BZ1
U 1 1 5C88C7FA
P 4200 3350
F 0 "BZ1" H 4353 3379 50  0000 L CNN
F 1 "Buzzer" H 4353 3288 50  0000 L CNN
F 2 "lib_kaiote:Buzzer_12x9.5RM6.5" V 4175 3450 50  0001 C CNN
F 3 "http://www.puiaudio.com/pdf/AT-1224-TWT-5V-2-R.pdf" V 4175 3450 50  0001 C CNN
F 4 "433-1028-ND " H 4200 3350 50  0001 C CNN "manf#"
F 5 "digikey" H 4200 3350 50  0001 C CNN "vendor"
	1    4200 3350
	1    0    0    -1  
$EndComp
Wire Wire Line
	4100 3250 4100 3200
Wire Wire Line
	4100 3450 4100 3700
Wire Wire Line
	3200 4200 3750 4200
Wire Wire Line
	7050 4150 7600 4150
$Comp
L Device:D_Zener D?
U 1 1 5CB29962
P 3750 4050
AR Path="/5CB29962" Ref="D?"  Part="1" 
AR Path="/5C79219D/5CB29962" Ref="D15"  Part="1" 
F 0 "D15" V 3704 4129 50  0000 L CNN
F 1 "TVS_3V3" V 3800 4100 50  0000 L CNN
F 2 "Diode_SMD:D_SMB" H 3750 4050 50  0001 C CNN
F 3 "http://www.vishay.com/docs/88940/smbj3v3.pdf" H 3750 4050 50  0001 C CNN
F 4 "SMBJ3V3-E3/52GITR-ND" V 3750 4050 50  0001 C CNN "manf#"
F 5 "digikey" V 3750 4050 50  0001 C CNN "vendor"
	1    3750 4050
	0    1    1    0   
$EndComp
$Comp
L Device:D_Zener D?
U 1 1 5CB29C23
P 7600 4000
AR Path="/5CB29C23" Ref="D?"  Part="1" 
AR Path="/5C79219D/5CB29C23" Ref="D16"  Part="1" 
F 0 "D16" V 7554 4079 50  0000 L CNN
F 1 "TVS_3V3" V 7650 4050 50  0000 L CNN
F 2 "Diode_SMD:D_SMB" H 7600 4000 50  0001 C CNN
F 3 "http://www.vishay.com/docs/88940/smbj3v3.pdf" H 7600 4000 50  0001 C CNN
F 4 "SMBJ3V3-E3/52GITR-ND" V 7600 4000 50  0001 C CNN "manf#"
F 5 "digikey" V 7600 4000 50  0001 C CNN "vendor"
	1    7600 4000
	0    1    1    0   
$EndComp
Connection ~ 7600 4150
Wire Wire Line
	7600 4150 7950 4150
Connection ~ 3750 4200
Wire Wire Line
	3750 4200 4100 4200
$Comp
L power:VDD #PWR?
U 1 1 5C896F1D
P 4100 3200
AR Path="/5C896F1D" Ref="#PWR?"  Part="1" 
AR Path="/5C79219D/5C896F1D" Ref="#PWR054"  Part="1" 
F 0 "#PWR054" H 4100 3050 50  0001 C CNN
F 1 "VDD" H 4117 3373 50  0000 C CNN
F 2 "" H 4100 3200 50  0001 C CNN
F 3 "" H 4100 3200 50  0001 C CNN
	1    4100 3200
	1    0    0    -1  
$EndComp
$EndSCHEMATC
