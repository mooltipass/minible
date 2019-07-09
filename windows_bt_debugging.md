Low Level Bluetooth LE Debugging on Windows
===========================================

Inspired from https://social.msdn.microsoft.com/Forums/vstudio/en-US/f6a42282-28c2-4e4f-a66a-5e174404456d/bluetooth-le-hid-device-collection-error?forum=wdk
    
Requirements Installation
-------------------------
1) Windows Driver Kit : https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
2) Frontline Protocol Analysis System: http://www.fte.com/support/CPAS-download.aspx?demo=Sodera%20LE&iid=SLE

Capture File Generation
-----------------------
1) Start an administrator command prompt: type `cmd` in the windows menu, right click on "command prompt" and select "Run as Administrator"
2) `logman create trace "bth_hci" -ow -o c:\bth_hci.etl -p {8a1f9517-3a8c-4a9e-a018-4f17a200f277} 0xffffffffffffffff 0xff -nb 16 16 -bs 1024 -mode Circular -f bincirc -max 4096 -ets`
3) reproduce your issue
4) `logman stop "bth_hci" -ets`
5) a file named "bth_hci.etl" will be stored in C:
6) `"C:\Program Files (x86)\Windows Kits\10\Tools\x64\Bluetooth\BETLParse\btetlparse.exe" C:\bth_hci.etl`
7) a file named "bth_hci.cfa" will be stored in C:

Capture File Viewing
--------------------
1) Start "Viewer" in the "Frontline" menu in Windows' menu
2) Open bth_hci.cfa
3) In the "View" menu, select "Frame Display"
