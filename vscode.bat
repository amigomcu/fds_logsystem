rem Location of Nordic SDK
set NRF_SDK=C:/Users/steven/Downloads/nRF5_SDK_15.3.0_59ac345

rem Location of Nordic Command Line tools (nrfjprog) 
set NRF_TOOLS=C:/Program Files (x86)/Nordic Semiconductor/nrf5x/bin

rem location of GCC Cross-compiler https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
set GNU_GCC=C:/gnuarmemb/bin/

rem Location of Gnu Tools (make) https://github.com/gnu-mcu-eclipse/windows-build-tools/releases
set GNU_TOOLS=C:/nRF5x/2.12-20190422-1053/bin

rem Location of SEGGER JLink tools
set SEGGER_TOOLS=C:/Program Files (x86)/SEGGER/JLink_V644e

rem Location of java
set JAVA=C:/Program Files (x86)/Java/jre1.8.0_201/bin/java.exe

rem Serial numbers of nRF development boards
set PCA10056_SN=683662919
set PCA10040_SN=682645815

"C:/Users/steven/AppData/Local/Programs/Microsoft VS Code/Code.exe" app_blinky.code-workspace
