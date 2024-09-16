#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>
#include <cmath>
#include <unordered_map>

// CRC polynomial and table
const int CRC32_POLY = 0x04C11DB7;
const int CRC32_INIT = 0xFFFFFFFF;
const int CRC32_XOR_OUT = 0xFFFFFFFF;

#define IFG 0x07

using namespace std;

ofstream outFile;

class Ethernet
{
private:
    int LineRate;
    int CaptureSizeMs;
    int MinNumOfIFGsPerPacket;
    unsigned long long int  DestAddress;
    unsigned long long int SourceAddress;
    long long int Preamble_SFD_header;
    int MaxPacketSize;
    int BurstSize;
    int BurstPeriodicity_us;
    int * payload;
    int number_of_burst;
    int PacketSize;
    void createBurst(int BurstNumber);
    void cratePacket(int BurstNumber, int PackeNumber);
    int *CalculateCRC(int start_index, int end_index);
    int CalculatePacketSize();
    void SendIFGs(int);

public : 

    Ethernet();
    void ReadConfig(string filename);
    void createoutput();
    ~Ethernet();
};

Ethernet::Ethernet()
{
    cout << "Default Constructor called!" << endl;
}

void Ethernet::ReadConfig(string filename){

    //set configuration of Etherent
    std::ifstream file(filename);
    std::unordered_map<std::string, std::string> configMap;
    std::string line;

    // Read the file line by line
    while (std::getline(file, line))
    {
        // Ignore comments (starting with '//')
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos)
        {
            line = line.substr(0, commentPos); // Remove comment part
        }

        // Trim any leading/trailing spaces
        line.erase(0, line.find_first_not_of(" \t")); // Left trim
        line.erase(line.find_last_not_of(" \t") + 1); // Right trim

        // Find the position of the '=' character
        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos)
        {
            std::string key = line.substr(0, delimiterPos);    // Extract key
            std::string value = line.substr(delimiterPos + 1); // Extract value

            // Trim spaces around key and value
            key.erase(0, key.find_first_not_of(" \t"));     // Left trim key
            key.erase(key.find_last_not_of(" \t") + 1);     // Right trim key
            value.erase(0, value.find_first_not_of(" \t")); // Left trim value
            value.erase(value.find_last_not_of(" \t") + 1); // Right trim value

            // Store key-value pair in the map
            configMap[key] = value;
        }
    }

    // set valuse in member of class
    LineRate = stoi(configMap["Eth.LineRate"]);
    CaptureSizeMs = stoi(configMap["Eth.CaptureSizeMs"]);
    MinNumOfIFGsPerPacket = stoi(configMap["Eth.MinNumOfIFGsPerPacket"]);
    DestAddress = stoull(configMap["Eth.DestAddress"], nullptr, 16);
    SourceAddress = stoull(configMap["Eth.SourceAddress"], nullptr, 16);
    MaxPacketSize = stoi(configMap["Eth.MaxPacketSize"]);
    BurstSize = stoi(configMap["Eth.BurstSize"]);
    BurstPeriodicity_us = stoi(configMap["Eth.BurstPeriodicity_us"]);
    Preamble_SFD_header = 0xFB555555555555D5;

}

int Ethernet ::CalculatePacketSize() {

    int totalSize = PacketSize + 6 + 6 + 8 + 4;
    return totalSize;
}

void Ethernet ::createoutput(void){

    // assumptions for payload
    // initialize payload
    number_of_burst = (CaptureSizeMs * 1000) / BurstPeriodicity_us;
    int numebr_of_IFG = (LineRate * 1000 * BurstPeriodicity_us) - BurstSize * MaxPacketSize * 8;

    cout<<"IFGS numebr is :"<<numebr_of_IFG<<endl;

    PacketSize = 1476;
    int payload_size = PacketSize * number_of_burst * BurstSize;
    cout << "Payload size" << payload_size << endl;
    payload = new int[payload_size];

    for (int i = 0; i < payload_size; i++)
    {
        payload[i] = 0;
    }

    ///////////////////////////////////////////////////

    outFile.open("output.txt", std::ios::trunc);

    // Capture the current time
    auto start_time = std::chrono::system_clock::now();

    auto delay = std::chrono::microseconds(BurstPeriodicity_us);
    auto target_time = start_time + delay;

    for (int i = 0; i < number_of_burst; i++)
    {
        createBurst(i);

        SendIFGs(numebr_of_IFG);

        std::this_thread::sleep_until(target_time);

        auto now = std::chrono::system_clock::now();
        
        auto delay = std::chrono::microseconds(BurstPeriodicity_us);
        auto target_time = now + delay;
    }
    outFile.close();      
}

void Ethernet ::createBurst(int BurstNumber)
{
    float total_time_of_Packet_us = (PacketSize * 8) / (LineRate * 1000);

    for (int i = 0; i < BurstSize; i++)
    {
        cratePacket(BurstNumber,i);
    }
    
}

void Ethernet ::cratePacket(int BurstNumber, int PackeNumber){

    int start_index = BurstNumber * BurstSize * PacketSize + PackeNumber * PacketSize;
    int end_index = start_index + PacketSize;

    outFile << hex << 0xFB555555 << endl;
    outFile << hex << 0x555555D5 <<endl;


    //send Src and Des Address
    outFile << setw(8) << setfill('0');
    outFile<<hex<<(DestAddress>>16)<<endl;
    outFile << setw(4) << setfill('0');
    outFile << hex << ((0xFFFF & DestAddress));
    outFile << setw(4) << setfill('0');
    outFile << hex << (SourceAddress >> 32) << endl;
    outFile << setw(8) << setfill('0');
    outFile << hex << ((0xFFFFFFFF & SourceAddress));
    outFile<<"\n";

    int iterations = PacketSize/4;

    for (int i = 0; i < iterations; i++)
    {

        int start = start_index + i*4;
        int end_byte = start + 4 ;

        int data[4];
        for (int j = 0; j < 4; j++)
        {
            data[j] = payload[start+ j];
        }
        
        outFile << setw(8) << setfill(' '); 
        for (int k = 0; k < 4; k++)
        {
            outFile << hex << setw(2) << setfill('0') << data[k];
        }
        outFile<<"\n";
        
    }

    //calculation of CRC

    int * CRC = CalculateCRC(start_index , end_index);

    // Calculate the padding needed for 4-byte alignment
    int packetSizeIncludingCRC = CalculatePacketSize();
    int alignmentOffset = packetSizeIncludingCRC % 4;
    int extraIFGs = (alignmentOffset != 0) ? (4 - alignmentOffset) / 4 : 0;
    int numIFG = extraIFGs + MinNumOfIFGsPerPacket;

    if (alignmentOffset == 0)
    {
        outFile << setw(8) << setfill(' ');
        for (int k = 0; k < 4; k++)
        {
            outFile << hex << setw(2) << setfill('0') << CRC[k];
        }
        outFile << "\n";

        if((numIFG % 4) != 0)
        {
            numIFG = numIFG + 4 - (numIFG % 4);
        }

        for (int i = 0; i < (numIFG/4); i++)
        {
            outFile << setw(8) << setfill(' ');
            for (int k = 0; k < 4; k++)
            {
                outFile << hex << setw(2) << setfill('0') << to_string(IFG);
            }
            outFile<<"\n";
        }
        
    }
    else
    {
        int offset[3];
        for (int i = 0; i < alignmentOffset; i++)
        {
            offset[i] = payload[end_index- alignmentOffset + i];
        }

        outFile << setw(8) << setfill(' ');
        for (int i = 0; i < alignmentOffset; i++)
        {
            outFile << hex << setw(2) << setfill('0') << offset[i];
        }
        for (int i = 0; i < (4 - alignmentOffset); i++)
        {
            outFile << hex << setw(2) << setfill('0') << CRC[i];
        }
        outFile << "\n";

        outFile << setw(8) << setfill(' ');
        for (int i = (4 - alignmentOffset); i <4 ; i++)
        {
            outFile << hex << setw(2) << setfill('0') << CRC[i];
        }

        for (int i = 0; i < (4 - alignmentOffset); i++)
        {
            outFile << hex << setw(2) << setfill('0') << IFG;
        }

        outFile << "\n";

        if ((MinNumOfIFGsPerPacket % 4) != 0)
        {
            numIFG = MinNumOfIFGsPerPacket + 4 - (MinNumOfIFGsPerPacket % 4);
        }

        for (int i = 0; i < (numIFG / 4); i++)
        {
            outFile << setw(8) << setfill(' ');
            for (int k = 0; k < 4; k++)
            {
                outFile << hex << setw(2) << setfill('0') << to_string(IFG);
            }
            outFile << "\n";
        }
    }    
    
}

int * Ethernet ::CalculateCRC(int start_index, int end_index)
{
    // CRC32 table
    static int table[256];
    static bool tableComputed = false;
    

    if (!tableComputed)
    {
        for (int i = 0; i < 256; ++i)
        {
            int crc = i;
            for (int j = 8; j > 0; --j)
            {
                if (crc & 1)
                {
                    crc = (crc >> 1) ^ CRC32_POLY;
                }
                else
                {
                    crc = crc >> 1;
                }
            }
            table[i] = crc;
        }
        tableComputed = true;
    }

    int crc = CRC32_INIT;
    for (int i = start_index; i < end_index; ++i)
    {
        int byte = payload[i];
        int tableIndex = (crc ^ byte) & 0xFF;
        crc = (crc >> 8) ^ table[tableIndex];
    }

    int output = crc ^ CRC32_XOR_OUT;

    int *CRCResult = new int[4];

    for (int i = 0; i < 4; i++)
    {
        int shift = i * 8;
        CRCResult[i] = (output >> shift) & 0xFF;
    }
    return CRCResult;
}

void Ethernet ::SendIFGs(int total_IFGs_size){

    int number_of_IFG = total_IFGs_size/8;

    for (int i = 0; i < (number_of_IFG/4); i++)
    {
        outFile<<setw(8)<<setfill(' ');
        for (int k = 0; k < 4; k++)
        {
            outFile<<hex<<setw(2)<<setfill('0')<<IFG;
        }
        outFile<<endl;
    }
}

Ethernet::~Ethernet()
{
    cout << "Default deConstructor called!" << endl;
    delete[] payload;
}
