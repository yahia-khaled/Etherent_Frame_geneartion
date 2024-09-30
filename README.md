there are two files used to generate output packets<br />
main.cpp<br />
here we just read configuration and generate output packets by calling funtions provided by Ethernet.h<br />
<br />
Etherent.h<br />
in this file we write code that generate the output packets in the following steps : <br />
1. read configuration file <br />
2. calculate the required  and initializing variables <br />
calculate number of bursts using Capturetime and numebr total numebr of IFGs between bursts, and crate payload array dynamically besed on the number of bursts and size of each burst.<br />
3. start generate header of Etherent packet<br />
4. generate the payload of the Packet<br />
payload is assumed here to be zeros for simplicity.<br />
5. genertae IFGs for aligning and minimum numebr of IFGs<br />


