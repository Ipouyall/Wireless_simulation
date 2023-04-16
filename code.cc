#include <cstdlib>
#include <time.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <map>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/error-model.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

#define WORKER_COUNT 3

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("WifiTopology");

void
ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon, double em)
{
    uint16_t i = 0;

    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats ();

    Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
        Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);

        std::cout << "Flow ID			: "<< stats->first << " ; " << fiveTuple.sourceAddress << " -----> " << fiveTuple.destinationAddress << std::endl;
        std::cout << "Tx Packets = " << stats->second.txPackets << std::endl;
        std::cout << "Rx Packets = " << stats->second.rxPackets << std::endl;
        std::cout << "Duration		: "<< (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) << std::endl;
        std::cout << "Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds () << " Seconds" << std::endl;
        std::cout << "Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024  << " Mbps" << std::endl;
    
        i++;

        std::cout << "---------------------------------------------------------------------------" << std::endl;
    }

    Simulator::Schedule (Seconds (10),&ThroughputMonitor, fmhelper, flowMon, em);
}

void
AverageDelayMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon, double em)
{
    uint16_t i = 0;

    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats ();
    Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
        Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
        std::cout << "Flow ID			: "<< stats->first << " ; " << fiveTuple.sourceAddress << " -----> " << fiveTuple.destinationAddress << std::endl;
        std::cout << "Tx Packets = " << stats->second.txPackets << std::endl;
        std::cout << "Rx Packets = " << stats->second.rxPackets << std::endl;
        std::cout << "Duration		: "<< (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) << std::endl;
        std::cout << "Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds () << " Seconds" << std::endl;
        std::cout << "Sum of e2e Delay: " << stats->second.delaySum.GetSeconds () << " s" << std::endl;
        std::cout << "Average of e2e Delay: " << stats->second.delaySum.GetSeconds () / stats->second.rxPackets << " s" << std::endl;
    
        i++;

        std::cout << "---------------------------------------------------------------------------" << std::endl;
    }

    Simulator::Schedule (Seconds (10),&AverageDelayMonitor, fmhelper, flowMon, em);
}

string get_from_map (map<uint16_t, string> m, uint16_t key) {
    if (m.find(key) == m.end())
        return "";
    return m[key];
}

class MyHeader : public Header 
{
public:
    MyHeader ();
    virtual ~MyHeader ();
    void SetData (uint16_t data);
    uint16_t GetData (void) const;
    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
    virtual uint32_t GetSerializedSize (void) const;
private:
    uint16_t m_data;
};

MyHeader::MyHeader ()
{
}

MyHeader::~MyHeader ()
{
}

TypeId
MyHeader::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::MyHeader")
        .SetParent<Header> ()
        .AddConstructor<MyHeader> ()
    ;
    return tid;
}

TypeId
MyHeader::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}

void
MyHeader::Print (std::ostream &os) const
{
    os << "data = " << m_data << endl;
}

uint32_t
MyHeader::GetSerializedSize (void) const
{
    return 2;
}

void
MyHeader::Serialize (Buffer::Iterator start) const
{
    start.WriteHtonU16 (m_data);
}

uint32_t
MyHeader::Deserialize (Buffer::Iterator start)
{
    m_data = start.ReadNtohU16 ();

    return 2;
}

void 
MyHeader::SetData (uint16_t data)
{
    m_data = data;
}

uint16_t 
MyHeader::GetData (void) const
{
    return m_data;
}

class EncodedHeader : public Header 
{
public:
    EncodedHeader ();
    virtual ~EncodedHeader ();
    void SetData (std::string data);
    std::string GetData (void) const;
    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
    virtual uint32_t GetSerializedSize (void) const;
private:
    std::string m_data;
};

EncodedHeader::EncodedHeader ()
{
}

EncodedHeader::~EncodedHeader ()
{
}

TypeId
EncodedHeader::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::EncodedHeader")
        .SetParent<Header> ()
        .AddConstructor<EncodedHeader> ()
    ;
    return tid;
}

TypeId
EncodedHeader::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}

void
EncodedHeader::Print (std::ostream &os) const
{
    os << "data = " << m_data << endl;
}

uint32_t
EncodedHeader::GetSerializedSize (void) const
{
    return sizeof(m_data) / sizeof(std::string);
}

void
EncodedHeader::Serialize (Buffer::Iterator start) const
{
    // start.WriteHtonU16 (m_data);
    Buffer::Iterator it = start;
    int length = m_data.length();
    const char* str = m_data.c_str();
    for (int i = 0;i < length;i++){
        it.WriteU8 (str[i]);
    }
}

uint32_t
EncodedHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator it = start;

    int length = GetSerializedSize();
    char str[length + 1] = {0};
    for (int i = 0;i < length;i++){
      str[i] = it.ReadU8 ();
    }
    str[length] = '\0';
    m_data = string (str);
    return GetSerializedSize();
}

void 
EncodedHeader::SetData (std::string data)
{
    m_data = data;
}

std::string 
EncodedHeader::GetData (void) const
{
    return m_data;
}

class DecodedHeader : public Header 
{
public:
    DecodedHeader ();
    virtual ~DecodedHeader ();
    void SetData (uint16_t data);
    void SetPort (uint16_t port);
    void SetIpv4 (Ipv4Address ipv4);
    uint16_t GetData (void) const;
    uint16_t GetPort (void) const;
    Ipv4Address GetIpv4 (void) const;
    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
    virtual uint32_t GetSerializedSize (void) const;
private:
    uint16_t m_data;
    uint16_t m_port;
    Ipv4Address m_ipv4;
};

DecodedHeader::DecodedHeader ()
{
}

DecodedHeader::~DecodedHeader ()
{
}

TypeId
DecodedHeader::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::DecodedHeader")
        .SetParent<Header> ()
        .AddConstructor<EncodedHeader> ()
    ;
    return tid;
}

TypeId
DecodedHeader::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}

void
DecodedHeader::Print (std::ostream &os) const
{
    os << "data = " << m_data << endl;
    os << "port = " << m_port << endl;
    os << "ip = "; m_ipv4.Print(os); os << endl;
}

uint32_t
DecodedHeader::GetSerializedSize (void) const
{
    return 2 + 2 + 4;
}

void
DecodedHeader::Serialize (Buffer::Iterator start) const
{
    // start.WriteHtonU16 (m_data);
    Buffer::Iterator it = start;
    it.WriteHtonU16 (m_data);
    it.WriteHtonU16 (m_port);
    WriteTo (it, m_ipv4);
}

uint32_t
DecodedHeader::Deserialize (Buffer::Iterator start)
{
    // m_data = start.ReadNtohU16 ();

    // return 2;
    Buffer::Iterator it = start;
    m_data = it.ReadNtohU16 ();
    m_port = it.ReadNtohU16 ();
    ReadFrom (it, m_ipv4);
    return GetSerializedSize();
}

void 
DecodedHeader::SetData (uint16_t data)
{
    m_data = data;
}
void
DecodedHeader::SetPort (uint16_t port)
{
    m_port = port;
}
void
DecodedHeader::SetIpv4 (Ipv4Address ipv4)
{
    m_ipv4 = ipv4;
}

uint16_t
DecodedHeader::GetData (void) const
{
    return m_data;
}
uint16_t
DecodedHeader::GetPort (void) const
{
    return m_port;
}

Ipv4Address
DecodedHeader::GetIpv4 (void) const
{
    return m_ipv4;
}


class master : public Application
{
public:
    master (uint16_t port, Ipv4InterfaceContainer& ip, Ipv4Address cip, uint16_t c_port);
    virtual ~master ();
    void add_worker(InetSocketAddress waddr);
private:
    virtual void StartApplication (void);
    void HandleRead (Ptr<Socket> socket);

    uint16_t port;
    Ipv4InterfaceContainer ip;
    Ipv4Address cip;
    uint16_t c_port;
    vector<Ptr<Socket>> worker_sockets;
    Ptr<Socket> socket;
};

class client : public Application
{
public:
    client (uint16_t port, uint16_t s_port, Ipv4InterfaceContainer& ip);
    virtual ~client ();
    Ipv4Address getIP();
    uint16_t getPort();

private:
    virtual void StartApplication (void);
    void HandleIncoming (Ptr<Socket> socket);

    uint16_t port;
    uint16_t s_port;
    Ptr<Socket> socket;
    Ptr<Socket> server;
    Ipv4InterfaceContainer ip;
    std::string encodedData;
};

class worker : public Application
{
public:
    worker (uint16_t tcpPort, Ipv4InterfaceContainer& ip, map<uint16_t, string> m);
    virtual ~worker ();
    InetSocketAddress get_server_address();

private:
    virtual void StartApplication (void);
    void HandleRead (Ptr<Socket> socket);
    void ProcessData (Ptr<Packet> packet);
    void HandleAccept (Ptr<Socket> sock, const Address &from);

    uint16_t tcpPort;
    Ipv4InterfaceContainer ip;
    Ptr<Socket> tcpSocket;
    map<uint16_t, string> mapping;
};


int
main (int argc, char *argv[])
{
    double error = 0.000001;
    string bandwidth = "1Mbps";
    bool verbose = true;
    double duration = 60.0;
    bool tracing = false;

    srand(time(NULL));

    CommandLine cmd (__FILE__);
    cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

    cmd.Parse (argc,argv);

    if (verbose)
    {
        LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    // map<uint16_t, string> mapping1, mapping2, mapping3;
    // mapping1 = {{0,"a"}, {3,"b"}, {4,"c"}, {11,"d"}, {19,"e"}, {22,"f"}, {23,"g"}, {24,"h"}};
    // mapping2 = {{5,"i"}, {6,"j"}, {9,"k"}, {13,"l"}, {15,"m"}, {1,"n"}, {7,"o"}, {21,"p"}, {25,"q"}};
    // mapping3 = {{2,"r"}, {8,"s"}, {10,"t"}, {14,"u"}, {12,"v"}, {16,"w"}, {17,"x"}, {18,"y"}, {20,"z"}};

    NodeContainer wifiStaNodeClient;
    wifiStaNodeClient.Create (1);

    NodeContainer wifiStaNodeMaster;
    wifiStaNodeMaster.Create (1);

    NodeContainer wifiStaNodeWorker;
    wifiStaNodeWorker.Create (WORKER_COUNT);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();

    YansWifiPhyHelper phy;
    phy.SetChannel (channel.Create ());

    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("ns-3-ssid");

    mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDeviceClient;
    staDeviceClient = wifi.Install (phy, mac, wifiStaNodeClient);
    mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));

    NetDeviceContainer staDeviceMaster;
    staDeviceMaster = wifi.Install (phy, mac, wifiStaNodeMaster);
    mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDeviceWorker;
    staDeviceWorker = wifi.Install (phy, mac, wifiStaNodeWorker);
    // mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (error));
    phy.SetErrorRateModel("ns3::YansErrorRateModel");

    MobilityHelper mobility;

    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (5.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));

    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                               "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
    mobility.Install (wifiStaNodeClient);

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiStaNodeMaster);

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiStaNodeWorker);

    InternetStackHelper stack;
    stack.Install (wifiStaNodeClient);
    stack.Install (wifiStaNodeMaster);
    stack.Install (wifiStaNodeWorker);

    Ipv4AddressHelper address;

    Ipv4InterfaceContainer staNodeClientInterface;
    Ipv4InterfaceContainer staNodesMasterInterface;
    Ipv4InterfaceContainer staNodesWorkerInterface;

    address.SetBase ("10.1.3.0", "255.255.255.0");
    staNodeClientInterface = address.Assign (staDeviceClient);
    staNodesMasterInterface = address.Assign (staDeviceMaster);
    staNodesWorkerInterface = address.Assign (staDeviceWorker);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // TODO: Check from here, get workers' sockAddr to master
    uint16_t client_master_port = 1102, worker_client_port = 1104;
    // uint16_t worker_ports[WORKER_COUNT]= {5050, 5051, 5052};

    Ptr<client> clientApp = CreateObject<client> (client_master_port, worker_client_port, staNodesMasterInterface);
    wifiStaNodeClient.Get (0)->AddApplication (clientApp);
    clientApp->SetStartTime (Seconds (0.0));
    clientApp->SetStopTime (Seconds (duration));  
    uint16_t client_port = clientApp->getPort();
    Ipv4Address client_ip = clientApp->getIP();

    Ptr<master> masterApp = CreateObject<master> (client_master_port, staNodesMasterInterface, client_ip, client_port);
    wifiStaNodeMaster.Get (0)->AddApplication (masterApp);
    masterApp->SetStartTime (Seconds (0.0));
    masterApp->SetStopTime (Seconds (duration));  

    NS_LOG_INFO ("Run Simulation");

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    ThroughputMonitor (&flowHelper, flowMonitor, error);
    AverageDelayMonitor (&flowHelper, flowMonitor, error);

    Simulator::Stop (Seconds (duration));
    Simulator::Run ();

    return 0;
}

client::client (uint16_t port, uint16_t s_port, Ipv4InterfaceContainer& ip)
        : port (port),
          s_port (s_port),
          ip (ip),
          encodedData("")
{
    std::srand (time(0));
}

client::~client ()
{
}

static void GenerateTraffic (Ptr<Socket> socket, uint16_t data)
{
    Ptr<Packet> packet = new Packet();
    MyHeader m;
    m.SetData(data);

    packet->AddHeader (m);
    packet->Print (std::cout);
    socket->Send(packet);

    Simulator::Schedule (Seconds (0.1), &GenerateTraffic, socket, rand() % 26);
}

void
client::StartApplication (void)
{
    Ptr<Socket> sock = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    InetSocketAddress sockAddr (ip.GetAddress(0), port);
    sock->Connect (sockAddr);

    server = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    InetSocketAddress local = InetSocketAddress (ip.GetAddress(0), s_port);
    server->Bind (local);
    server->SetRecvCallback (MakeCallback (&client::HandleIncoming, this));

    GenerateTraffic(sock, 0);
}

Ipv4Address client::getIP()
{
    return ip.GetAddress(0);
}

uint16_t client::getPort()
{
    return s_port;
}

void
client::HandleIncoming (Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    while ((packet = socket->Recv ()))
    {
        if (packet->GetSize () == 0)
        {
            break;
        }

        EncodedHeader encoded;
        packet->RemoveHeader (encoded);
        encoded.Print(std::cout);

        string ed = encoded.GetData();
        encodedData += ed;
    }
}

master::master (uint16_t port, Ipv4InterfaceContainer& ip, Ipv4Address cip, uint16_t c_port)
        : port (port),
          ip (ip),
          cip(cip),
          c_port(c_port)
{
    std::srand (time(0));
}

master::~master ()
{
}

void 
master::add_worker(InetSocketAddress waddr)
{
    Ptr<Socket> remote = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    remote->Connect (waddr);
    worker_sockets.push_back(remote);
}

void
master::StartApplication (void)
{
    socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    InetSocketAddress local = InetSocketAddress (ip.GetAddress(0), port);
    socket->Bind (local);

    socket->SetRecvCallback (MakeCallback (&master::HandleRead, this));
}

void 
master::HandleRead (Ptr<Socket> socket)
{
    Ptr<Packet> packet;

    while ((packet = socket->Recv ()))
    {
        if (packet->GetSize () == 0)
        {
            break;
        }
        MyHeader m_header;
        packet->RemoveHeader (m_header);
        m_header.Print(std::cout);

        DecodedHeader d_header;
        d_header.SetData(m_header.GetData());
        d_header.SetPort(c_port);
        d_header.SetIpv4(cip);

        for (auto& w : worker_sockets)
        {
            Ptr<Packet> packet = new Packet();
            packet->AddHeader(d_header);
            w->Send (packet);
        }
    }
}

worker::worker (uint16_t tcpPort, Ipv4InterfaceContainer& ip, map<uint16_t, string> m)
  : tcpPort(tcpPort),
    ip(ip),
    mapping(m)
{
}

worker::~worker ()
{
}

void 
worker::StartApplication (void)
{
    // Create TCP socket and set the options
    tcpSocket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
    tcpSocket->Bind (InetSocketAddress (ip.GetAddress (0), tcpPort));
    tcpSocket->Listen ();
    tcpSocket->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
                             MakeCallback (&worker::HandleAccept, this));
    tcpSocket->SetRecvCallback (MakeCallback (&worker::HandleRead, this));
}

void
worker::HandleAccept (Ptr<Socket> sock, const Address &from)
{
  sock->SetRecvCallback (MakeCallback (&worker::HandleRead, this));
}

void 
worker::HandleRead (Ptr<Socket> socket)
{
    Ptr<Packet> packet;

    while ((packet = socket->Recv ()))
    {
        if (packet->GetSize () == 0) break;
        ProcessData(packet);
    }
}

void worker::ProcessData(Ptr<Packet> packet) 
{
    DecodedHeader d_header;
    packet->RemoveHeader (d_header);

    uint16_t key = d_header.GetData(),
             c_port = d_header.GetPort();
    Ipv4Address c_ipv4 = d_header.GetIpv4();
    string encoded = get_from_map (mapping, key);
    if (encoded == "") return;

    Ptr<Packet> e_packet = new Packet();
    EncodedHeader e_header;
    e_header.SetData(encoded);
    e_packet->AddHeader(e_header);

    Ptr<Socket> udpSendSocket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    InetSocketAddress remote_client (c_ipv4, c_port);
    udpSendSocket->Connect (remote_client);
    udpSendSocket->Send (e_packet);
    udpSendSocket->Close();
}


InetSocketAddress worker::get_server_address()
{
    return InetSocketAddress (ip.GetAddress (0), tcpPort);
}
