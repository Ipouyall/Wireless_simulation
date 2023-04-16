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

#define InetSocketAddress 3

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

class master : public Application
{
public:
    master (uint16_t port, Ipv4InterfaceContainer& ip);
    virtual ~master ();
    void add_worker(InetSocketAddress waddr);
private:
    virtual void StartApplication (void);
    void HandleRead (Ptr<Socket> socket);

    uint16_t port;
    Ipv4InterfaceContainer ip;
    vector<Ptr<Socket>> worker_sockets;
    Ptr<Socket> socket;
};

class client : public Application
{
public:
    client (uint16_t port, uint16_t s_port, Ipv4InterfaceContainer& ip);
    virtual ~client ();

private:
    virtual void StartApplication (void);
    void HandleIncoming (Ptr<Socket> socket);

    uint16_t port;
    uint16_t s_port;
    Ptr<Socket> socket;
    Ptr<Socket> server;
    Ipv4InterfaceContainer ip;
};

class worker : public Application
{
public:
    worker (uint16_t tcpPort, uint16_t udpPort, Ipv4InterfaceContainer& ip, map<uint16_t, string> m;);
    virtual ~worker ();
    InetSocketAddress get_server_address();

private:
    virtual void StartApplication (void);
    void HandleRead (Ptr<Socket> socket);
    void ProcessData (Ptr<Packet> packet);

    uint16_t tcpPort;
    uint16_t udpPort;
    bool connected;
    Ipv4InterfaceContainer ip;
    Ptr<Socket> tcpSocket;
    Ptr<Socket> udpSocket;
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

    map<uint16_t, string> mapping1, mapping2, mapping3;
    mapping1 = {{0,"a"}, {3,"b"}, {4,"c"}, {11,"d"}, {19,"e"}, {22,"f"}, {23,"g"}, {24,"h"}};
    mapping2 = {{5,"i"}, {6,"j"}, {9,"k"}, {13,"l"}, {15,"m"}, {1,"n"}, {7,"o"}, {21,"p"}, {25,"q"}};
    mapping3 = {{2,"r"}, {8,"s"}, {10,"t"}, {14,"u"}, {12,"v"}, {16,"w"}, {17,"x"}, {18,"y"}, {20,"z"}};

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
    mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));

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
    uint16_t client_master_port = 1102;
    uint16_t worker_ports[WORKER_COUNT]= {5050, 5051, 5052};

    Ptr<client> clientApp = CreateObject<client> (client_master_port, staNodesMasterInterface);
    wifiStaNodeClient.Get (0)->AddApplication (clientApp);
    clientApp->SetStartTime (Seconds (0.0));
    clientApp->SetStopTime (Seconds (duration));  

    Ptr<master> masterApp = CreateObject<master> (client_master_port, staNodesMasterInterface);
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
          s_port (s_port)
          ip (ip)
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

void
client::HandleIncoming (void)
{
    // TODO: implement
}

master::master (uint16_t port, Ipv4InterfaceContainer& ip)
        : port (port),
          ip (ip)
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
        // TODO: change following to send to workers
        MyHeader destinationHeader;
        packet->RemoveHeader (destinationHeader);
        destinationHeader.Print(std::cout);
    }
}

worker::worker (uint16_t tcpPort, uint16_t udpPort, Ipv4InterfaceContainer& ip, map<uint16_t, string> m)
  : tcpPort(tcpPort),
    udpPort(udpPort),
    ip(ip),
    mapping(m),
    connected(false)
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
    tcpSocket->SetAcceptCallback (MakeCallback (&worker::HandleRead, this));

    // Create UDP socket and set the options
    udpSocket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    udpSocket->Bind (InetSocketAddress (ip.GetAddress (0), udpPort));
}

void 
worker::HandleRead (Ptr<Socket> socket)
{
    if (!connected) {
        Ptr<Socket> childSocket = socket->Accept ();
        childSocket->SetRecvCallback (MakeCallback (&worker::ProcessData, this));
        connected = true;
    } else {
        ProcessData(socket->Recv());
    }
}

void worker::ProcessData(Ptr<Packet> packet) {
    std::string message = GetStringFromPacket(packet);
    if (message == "END_COMMUNICATION") {
        tcpSocket->Close();
        return;
    }

    // Process the data here
    // TODO: implement

    // Send the response to the client using UDP
    Ptr<Socket> udpSendSocket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    udpSendSocket->Connect (InetSocketAddress (clientAddress, udpPort));
    udpSendSocket->Send (packet);
    udpSendSocket->Close();
}


InetSocketAddress worker::get_server_address(){
    return InetSocketAddress (ip.GetAddress (0), tcpPort);
}
