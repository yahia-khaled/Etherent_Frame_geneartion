There are two files used to generate output packets<br
main.cpp<br />
Here we just read configuration and generate output packets by calling functions provided by Ethernet. h<br />
<br />
Etherent.h<br />
In this file we write code that generates the output packets in the following steps: <br />
1. read configuration file <br />
2. Calculate the required and initializing variables.
Calculate the number of bursts using CaptureTime and the total number of IFGs between bursts, and create a payload array dynamically based on the number of bursts and size of each burst. <br />
3. Start generating the header of the Ethernet packet<br />
4. generate the payload of the packet<br />
payload is assumed here to be zeros for simplicity.<br />
5. Generate IFGs for aligning and minimum number of IFGs<br />
generate IFGs such that output packets are 4-byte aligned, have a minimum number of IFGs, and send IFGs until the next burst.
