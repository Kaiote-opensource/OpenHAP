EESchema Schematic File Version 4
LIBS:sensor-node-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 3 3
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
L Battery_Management:MCP73812T-420I-OT U8
U 1 1 5C7F0980
P 4150 4100
F 0 "U8" H 4400 4500 50  0000 L CNN
F 1 "MCP73812T-420I-OT" H 4400 4400 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23-5" H 4200 3850 50  0001 L CNN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/22036b.pdf" H 3900 4350 50  0001 C CNN
F 4 "MCP73832T-2ACI/OTCT-ND" H 4150 4100 50  0001 C CNN "manf#"
F 5 "digikey" H 4150 4100 50  0001 C CNN "vendor"
	1    4150 4100
	1    0    0    -1  
$EndComp
$Comp
L power:GNDREF #PWR055
U 1 1 5C7F0A6B
P 4150 4600
F 0 "#PWR055" H 4150 4350 50  0001 C CNN
F 1 "GNDREF" H 4155 4427 50  0000 C CNN
F 2 "" H 4150 4600 50  0001 C CNN
F 3 "" H 4150 4600 50  0001 C CNN
	1    4150 4600
	1    0    0    -1  
$EndComp
$Comp
L Device:Battery_Cell BT2
U 1 1 5C7F0B26
P 5325 4250
F 0 "BT2" H 5325 4225 50  0000 L CNN
F 1 "Battery_Cell" V 5550 3975 50  0000 L CNN
F 2 "Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal" V 5325 4310 50  0001 C CNN
F 3 "http://www.jst-mfg.com/product/pdf/eng/ePH.pdf" V 5325 4310 50  0001 C CNN
F 4 "455-1749-1-ND" H 5325 4250 50  0001 C CNN "manf#"
F 5 "digikey" H 5325 4250 50  0001 C CNN "vendor"
	1    5325 4250
	1    0    0    -1  
$EndComp
$Comp
L Device:C C15
U 1 1 5C7F1416
P 3000 4200
F 0 "C15" H 2800 4275 50  0000 L CNN
F 1 "4.7uF/16v/10%/0805" H 2100 4100 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 3038 4050 50  0001 C CNN
F 3 "http://www.samsungsem.com/kr/support/product-search/mlcc/CL21A475KOFNNNE.jsp" H 3000 4200 50  0001 C CNN
F 4 "CL21A475KOFNNNE" H 3000 4200 50  0001 C CNN "manf#"
F 5 "digikey" H 3000 4200 50  0001 C CNN "vendor"
	1    3000 4200
	1    0    0    -1  
$EndComp
Wire Wire Line
	3000 4050 3000 3800
Wire Wire Line
	3000 3800 4150 3800
Connection ~ 4150 3800
$Comp
L Device:R_US R?
U 1 1 5C872014
P 3700 4350
AR Path="/5C872014" Ref="R?"  Part="1" 
AR Path="/5C7EC132/5C872014" Ref="R30"  Part="1" 
F 0 "R30" H 3550 4450 50  0000 L CNN
F 1 "2.4k(0.1%)" H 3250 4300 50  0000 L CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 3740 4340 50  0001 C CNN
F 3 "hhttps://industrial.panasonic.com/cdbs/www-data/pdf/RDM0000/AOA0000C307.pdf" H 3700 4350 50  0001 C CNN
F 4 "digikey" H -4950 1950 50  0001 C CNN "vendor"
F 5 "P2.4KDACT-ND" H -4950 1950 50  0001 C CNN "manf#"
	1    3700 4350
	1    0    0    -1  
$EndComp
Wire Wire Line
	3750 4200 3700 4200
Wire Wire Line
	3700 4500 4150 4500
Wire Wire Line
	4150 4500 4150 4400
Wire Wire Line
	4150 4600 4150 4500
Connection ~ 4150 4500
Connection ~ 3700 4500
Text HLabel 4050 3650 0    50   Input ~ 0
USB_5V
Wire Wire Line
	4150 3650 4050 3650
Wire Wire Line
	4150 3650 4150 3800
$Comp
L KAIOTE_VISHAY:SI2323DDS-T1-GE3 Q?
U 1 1 5C899412
P 6450 4000
AR Path="/5C899412" Ref="Q?"  Part="1" 
AR Path="/5C7EC132/5C899412" Ref="Q9"  Part="1" 
F 0 "Q9" V 6685 4000 50  0000 C CNN
F 1 "SI2323DDS-T1-GE3" V 6594 4000 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 6450 4000 50  0001 L BNN
F 3 "http://www.vishay.com/docs/64004/si2323dds.pdf" H 6450 4000 50  0001 L BNN
F 4 "SI2323DDS-T1-GE3 " H 6450 4000 50  0001 L BNN "manf#"
F 5 "digikey" V 6450 4000 50  0001 C CNN "vendor"
	1    6450 4000
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_US R?
U 1 1 5C89941C
P 6550 4450
AR Path="/5C89941C" Ref="R?"  Part="1" 
AR Path="/5C7EC132/5C89941C" Ref="R31"  Part="1" 
F 0 "R31" H 6650 4500 50  0000 L CNN
F 1 "100k(5%)" H 6600 4400 50  0000 L CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 6590 4440 50  0001 C CNN
F 3 "https://www.seielect.com/Catalog/SEI-RMCF_RMCP.pdf" H 6550 4450 50  0001 C CNN
F 4 "digikey" H -2100 2050 50  0001 C CNN "vendor"
F 5 "RMCF0805FT100KCT-ND " H -2100 2050 50  0001 C CNN "manf#"
	1    6550 4450
	1    0    0    -1  
$EndComp
$Comp
L power:GNDREF #PWR056
U 1 1 5C899423
P 6550 4650
F 0 "#PWR056" H 6550 4400 50  0001 C CNN
F 1 "GNDREF" H 6555 4477 50  0000 C CNN
F 2 "" H 6550 4650 50  0001 C CNN
F 3 "" H 6550 4650 50  0001 C CNN
	1    6550 4650
	1    0    0    -1  
$EndComp
Wire Wire Line
	6550 4650 6550 4600
Wire Wire Line
	6650 4000 6950 4000
Connection ~ 6950 4000
Text HLabel 7200 4000 2    50   Output ~ 0
VDD
Wire Wire Line
	6950 4000 7200 4000
Wire Wire Line
	6550 4200 6550 4250
Connection ~ 6550 4250
Wire Wire Line
	6550 4250 6550 4300
Wire Wire Line
	6950 3650 6950 4000
Connection ~ 4150 3650
Wire Wire Line
	5850 4250 5850 3650
Wire Wire Line
	5850 4250 6550 4250
Wire Wire Line
	5850 3650 4150 3650
Text HLabel 3700 4000 0    50   Input ~ 0
CHARGE_LED
Wire Wire Line
	3700 4000 3750 4000
Wire Wire Line
	3000 4500 3000 4350
Wire Wire Line
	3000 4500 3700 4500
Wire Wire Line
	5700 4000 5700 4550
Wire Wire Line
	5700 4550 5850 4550
Connection ~ 5700 4000
Wire Wire Line
	5700 4000 6250 4000
Text HLabel 5850 4550 2    50   Output ~ 0
VBATT_PWR
Wire Wire Line
	4550 4000 5100 4000
Wire Wire Line
	5325 4500 5325 4350
Wire Wire Line
	4150 4500 5100 4500
Wire Wire Line
	5325 4050 5325 4000
Wire Wire Line
	5325 4000 5700 4000
Connection ~ 5325 4000
Wire Wire Line
	5100 4050 5100 4000
Connection ~ 5100 4000
Wire Wire Line
	5100 4000 5325 4000
Wire Wire Line
	5100 4350 5100 4500
Connection ~ 5100 4500
Wire Wire Line
	5100 4500 5325 4500
$Comp
L Device:C C16
U 1 1 5CE7426E
P 5100 4200
F 0 "C16" H 4900 4275 50  0000 L CNN
F 1 "4.7uF/16v/10%/0805" H 4200 4025 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 5138 4050 50  0001 C CNN
F 3 "http://www.samsungsem.com/kr/support/product-search/mlcc/CL21A475KOFNNNE.jsp" H 5100 4200 50  0001 C CNN
F 4 "CL21A475KOFNNNE" H 5100 4200 50  0001 C CNN "manf#"
F 5 "digikey" H 5100 4200 50  0001 C CNN "vendor"
	1    5100 4200
	1    0    0    -1  
$EndComp
Wire Notes Line
	7600 3750 7600 4650
Wire Notes Line
	8550 4650 8550 3750
Text Notes 7650 4650 0    50   ~ 0
Battery mounting holes
Wire Notes Line
	7600 3750 8550 3750
Wire Notes Line
	8550 4650 7600 4650
$Comp
L Device:D_Schottky D?
U 1 1 5CAA287B
P 6400 3650
AR Path="/5CAA287B" Ref="D?"  Part="1" 
AR Path="/5C7EC132/5CAA287B" Ref="D17"  Part="1" 
F 0 "D17" H 6550 3800 50  0000 C CNN
F 1 "D_Schottky" H 6550 3750 50  0000 C CNN
F 2 "Diode_SMD:D_SMA_Handsoldering" H 6400 3650 50  0001 C CNN
F 3 "https://www.diodes.com/assets/Datasheets/ds13002.pdf" H 6400 3650 50  0001 C CNN
F 4 "B120-FDICT-ND" H 6400 3650 50  0001 C CNN "manf#"
F 5 "digikey" H 6400 3650 50  0001 C CNN "vendor"
	1    6400 3650
	-1   0    0    -1  
$EndComp
Wire Wire Line
	6250 3650 5850 3650
Connection ~ 5850 3650
Wire Wire Line
	6550 3650 6950 3650
Wire Wire Line
	8400 4500 8400 4450
$Comp
L power:GNDREF #PWR0106
U 1 1 5C7851E3
P 8400 4500
F 0 "#PWR0106" H 8400 4250 50  0001 C CNN
F 1 "GNDREF" H 8200 4450 50  0000 C CNN
F 2 "" H 8400 4500 50  0001 C CNN
F 3 "" H 8400 4500 50  0001 C CNN
	1    8400 4500
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole_Pad H2
U 1 1 5C78431B
P 8400 4350
F 0 "H2" H 8200 4350 50  0000 L CNN
F 1 "MountingHole_Pad" H 7700 4250 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.7mm_M2.5_Pad_Via" H 8400 4350 50  0001 C CNN
F 3 "~" H 8400 4350 50  0001 C CNN
	1    8400 4350
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H1
U 1 1 5C788445
P 7800 4000
F 0 "H1" H 7900 4046 50  0000 L CNN
F 1 "MountingHole" H 7900 3955 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.7mm_M2.5" H 7800 4000 50  0001 C CNN
F 3 "~" H 7800 4000 50  0001 C CNN
	1    7800 4000
	1    0    0    -1  
$EndComp
$EndSCHEMATC
