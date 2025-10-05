CAN DATA Sniffer Full Stack Project <br>
<br>
This project provides a means to send/receive the CAN-Bus traffic in a car using Arduino ESP32 by Lilygo T-CAN485. <br>
First problem was with logging the date I get from the CAN-Bus, there is a lot of it, it is different, and need to be displayed and sorted correctly. <br>
<br>
To implement the GUI, I looked at similar devices for working with CAN-bus, and learned their operating principle, date display, and assembled my own working version from other Python ready-made open-source options. <br>
Main for Visualising is this one - https://github.com/adamtheone/canDrive, but, "from the box" this tool is not working and need to adjustment, what i did. <br>
<br>
For Arduino I used library that came with the board, twai.h, I found the easiest way to communicate with the PC to CAN-bus on YouTube, combined everything together, ran several tests and got an almost finished version. <br>
<br>
<img width="1919" height="1079" alt="sniffer" src="https://github.com/user-attachments/assets/b85686ac-e265-47ad-a871-640b41a2549c" />
