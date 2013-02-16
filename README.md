bt_stone
========


1. Clone the repo

1. You need to get the Stonestreet One SDK which is available for free at http://www.ti.com/tool/stonestreetone-bt-sdk
   We used the version "CC256XMSPBTBLESW: Stonestreet One BT+BLE Stack on MSP430 v1.3"
   Other versions of the stack may also work but have not been tested.
   
2. From the Stonestreet One SDK folder (CC256x MSP430 Bluetopia SDK/v1.3/CC256x+MSP430_Applications/Source/MSP430_Experimentor/Bluetopia) you need to copy the following folders and files to PATH_TO_REPO/Bluetopia/
   * Folder btpskrnl to PATH_TO_REPO/Bluetopia/btpskrnl
   * Folder btpsvend to PATH_TO_REPO/Bluetopia/btpsvend
   * Folder hcitrans to PATH_TO_REPO/Bluetopia/hcitrans
   * Folder include to PATH_TO_REPO/Bluetopia/include
   * File lib/CCS/libBluetopia.a to PATH_TO_REPO/Bluetopia/lib/libBluetopia.a
     You may need to create the folder lib first
     
3. You should now be ready to build the project