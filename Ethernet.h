#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>
#include <unordered_map>


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
    long int EigthBytes;
    int * payload;
    int number_of_burst;
    int PacketSize;
    void createBurst(int BurstNumber);
    void cratePacket(int BurstNumber, int PackeNumber);
    int *CRC();
    int CalculatePacketSize();
    short int mergeTwoByte(int First, int second);

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

    cout << configMap["Eth.SourceAddress"]<<endl;
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

    cout<<DestAddress<<endl;

    //initialize payload

    number_of_burst = (CaptureSizeMs * 1000) / BurstPeriodicity_us;
    PacketSize = 1470;
    int payload_size = PacketSize * number_of_burst * BurstSize;
    cout << "Payload size" << payload_size << endl;
    payload = new int[payload_size];

    for (int i = 0; i < payload_size; i++)
    {
        payload[i] = 0;
    }
}

int Ethernet ::CalculatePacketSize() {

    int totalSize = PacketSize + 6 + 6 + 8 + 4;
    return totalSize;
}

void Ethernet ::createoutput(void){

    outFile.open("output.txt", std::ios::app);

    // Capture the current time
    auto start_time = std::chrono::system_clock::now();

    auto delay = std::chrono::microseconds(BurstPeriodicity_us);
    auto target_time = start_time + delay;

    for (int i = 0; i < number_of_burst; i++)
    {
        createBurst(i);

        std::this_thread::sleep_until(target_time);

        auto now = std::chrono::system_clock::now();
        
        auto delay = std::chrono::microseconds(BurstPeriodicity_us);
        auto target_time = start_time + delay;
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
            outFile << hex << setw(2) << setfill('0') << to_string(data[k]);
        }
        outFile<<"\n";
        
    }

    // Calculate the padding needed for 4-byte alignment
    int packetSizeIncludingCRC = CalculatePacketSize();
    int alignmentOffset = packetSizeIncludingCRC % 4;
    int extraIFGs = (alignmentOffset != 0) ? (4 - alignmentOffset) / 4 : 0;
    int numIFG = extraIFGs + MinNumOfIFGsPerPacket;

    if (alignmentOffset == 0)
    {
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
            offset[i] = payload[end_index-alignmentOffset+i];
        }

        outFile << setw(8) << setfill(' ');
        for (int i = 0; i < alignmentOffset; i++)
        {
            outFile << hex << setw(2) << setfill('0') << to_string(offset[i]);
        }
        for (int i = 0; i < (4 - alignmentOffset); i++)
        {
            outFile << hex << setw(2) << setfill('0') << to_string(IFG);
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

Ethernet::~Ethernet()
{
    cout << "Default deConstructor called!" << endl;
    delete[] payload;
}
