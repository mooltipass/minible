The goal of this test was to measure the Voled step-up efficiency, average output voltage and ripple at different loads.  
Tests were done on MiniBLE v2 breakout board, with all switches off except 3V oled, discharge, bat->3V3 and BAT->8V on.  
R3 & Q4 node was soldered to GND (to always enable 3V3), R18 & R2 node was soldered to 3V3 (to allow 1V2 to reach Voled stepup).  
Iin measured on the power supply (calibrated and as accurate as a 4.5 digits fluke 45 multimeter), Vin & Vout measured using multimeters on the test points, Voutpkpk measured using oscilloscope, Iload was checked using a fluke 45 DMM.  
1.42V was measured on the main MCU 3V3, which seems to indicate that the 3V3 domain was not consuming power.   

### [](#header-3)1.00V Vin

| Load | Iin avg | Actual Vin avg | Vout avg | Vout pkpk | Efficiency |
|:-----|:--------|:---------------|:---------|:----------|:-----------|
| 0mA  | 5.8mA    |    1.01V       | 8.16V    |   35mV    |            |
| 5mA  | 247mA    |    1.00V       | 6.78V    |   650mV    |   14%      |

### [](#header-3)1.05V Vin

| Load | Iin avg | Actual Vin avg | Vout avg | Vout pkpk | Efficiency |
|:-----|:--------|:---------------|:---------|:----------|:-----------|
| 0mA  | 5.7mA    |    1.05V       | 8.16V    |   32mV    |            |
| 5mA  | 180mA    |    1.05V       | 8.00V    |   480mV    |   21%      |
| 10mA | 263mA    |    1.05V       | 6.29V    |   552mV    |   23%      |

### [](#header-3)1.10V Vin

| Load | Iin avg | Actual Vin avg | Vout avg | Vout pkpk | Efficiency |
|:-----|:--------|:---------------|:---------|:----------|:-----------|
| 0mA  | 5.6mA    |    1.10V       | 8.16V    |   32mV    |            |
| 5mA  | 78mA    |    1.10V       | 8.14V    |   48mV    |   47%      |
| 10mA | 232mA    |    1.10V       | 7.97V    |   450mV    |   31%      |

### [](#header-3)1.15V Vin

| Load | Iin avg | Actual Vin avg | Vout avg | Vout pkpk | Efficiency |
|:-----|:--------|:---------------|:---------|:----------|:-----------|
| 0mA  | 5.4mA    |    1.15V       | 8.16V    |   32mV    |            |
| 5mA  | 71mA    |    1.15V       | 8.15V    |   48mV    |   50%      |
| 10mA | 160mA    |    1.15V       | 8.12V    |   90mV    |   44%      |
| 15mA | 277mA    |    1.15V       | 8.00V    |   322mV    |   38%      |

### [](#header-3)1.20V Vin

| Load | Iin avg | Actual Vin avg | Vout avg | Vout pkpk | Efficiency |
|:-----|:--------|:---------------|:---------|:----------|:-----------|
| 0mA  | 5.3mA    |    1.20V       | 8.16V    |   32mV    |            |
| 5mA  | 67mA    |    1.20V       | 8.15V    |   50mV    |   51%      |
| 10mA | 134mA    |    1.20V       | 8.13V    |   53mV    |   51%      |
| 15mA | 222mA    |    1.20V       | 8.08V    |   180mV    |   45%      |
| 20mA | 291mA    |    1.20V       | 7.46V    |   340mV    |   43%      |

### [](#header-3)1.25V Vin

| Load | Iin avg | Actual Vin avg | Vout avg | Vout pkpk | Efficiency |
|:-----|:--------|:---------------|:---------|:----------|:-----------|
| 0mA  | 5.1mA    |    1.25V       | 8.16V    |   32mV    |           |
| 5mA  | 62mA    |    1.25V       | 8.15V    |   50mV    |   53%      |
| 10mA | 123mA    |    1.25V       | 8.13V    |   52mV    |   53%      |
| 15mA | 195mA    |    1.25V       | 8.11V    |   52mV    |   50%      |
| 20mA | 276mA    |    1.25V       | 7.83V    |   350mV    |   45%      |
| 25mA | 283mA    |    1.25V       | 6.83V    |   510mV    |   48%      |

### [](#header-3)1.30V Vin

| Load | Iin avg | Actual Vin avg | Vout avg | Vout pkpk | Efficiency |
|:-----|:--------|:---------------|:---------|:----------|:-----------|
| 0mA  | 5.0mA    |    1.30V       | 8.16V    |   32mV    |            |
| 5mA  | 57mA    |    1.30V       | 8.15V    |   50mV    |   55%      |
| 10mA | 114mA    |    1.30V       | 8.13V    |   55mV    |   55%      |
| 15mA | 180mA    |    1.30V       | 8.11V    |   56mV    |   52%      |
| 20mA | 255mA    |    1.30V       | 8.10V    |   44mV    |   49%      |
| 25mA | 312mA    |    1.30V       | 7.66V    |   270mV    |   47%      |
| 30mA | 287mA    |    1.30V       | 6.3V    |   400mV    |   51%      |

### [](#header-3)1.40V Vin

| Load | Iin avg | Actual Vin avg | Vout avg | Vout pkpk | Efficiency |
|:-----|:--------|:---------------|:---------|:----------|:-----------|
| 0mA  | 4.8mA    |    1.40V       | 8.16V    |   32mV    |            |
| 5mA  | 62mA    |    1.40V       | 8.15V    |   50mV    |   47%      |
| 10mA | 106mA    |    1.40V       | 8.13V    |   52mV    |   55%      |
| 15mA | 161mA    |    1.40V       | 8.11V    |   54mV    |   54%      |
| 20mA | 221mA    |    1.40V       | 8.09V    |   54mV    |   52%      |
| 25mA | 290mA    |    1.40V       | 8.08V    |   40mV    |   50%      |
| 30mA | 298mA    |    1.40V       | 7.07V    |   480mV    |   51%      |
