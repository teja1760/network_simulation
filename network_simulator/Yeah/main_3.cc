#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("Lab4-3");

ofstream cwnd[5]; // Congestion Windows filestream
ofstream queueFile; // Queue size filestream
ofstream drop;
ofstream recvfile[5]; // Receiver Rates filesteam
int packetCount=0; //Current Packet Count
int totalLength=0;
int packetSize[10];
int sumPacketSize,queueSize;
double initialTime[10],finalTime,diffTime;
static const double totalTime = 1.8;
static double node1BytesRcv = 0.0;
ofstream throughput_data;


void
ReceiveNode1Packet (string context, Ptr<const Packet> p, const Address& addr)
{
  //NS_LOG_INFO (context <<
            //" Packet Received from Node 2 at " << Simulator::Now ().GetSeconds() << "from " << InetSocketAddress::ConvertFrom(addr).GetIpv4 ());
  node1BytesRcv += p->GetSize ();
 // cout << node1BytesRcv << " sd" << endl;
  throughput_data << Simulator::Now ().GetSeconds() << " " << node1BytesRcv << endl;
}

static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}

static void
TxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("TxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}


/**
 * Class which will act as a tcp source. We will hook a congestion tracer with its tcp connection.
 */
class MyApp : public Application
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);
  ScheduleTx ();
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}


/**
 * Following 5 functions are congestion windows hooks for each of the tcp connections
 */
static void
CwndTracer1(uint32_t oldval, uint32_t newval)
{
  if ( newval > oldval ){
          cwnd[0]<<Simulator::Now().GetSeconds()<<  " " << newval <<endl;
      }
      else {
          drop <<Simulator::Now().GetSeconds()<<" " << oldval <<  " " <<newval<<endl;
      }
}

static void
CwndTracer2(uint32_t oldval, uint32_t newval)
{
    if ( newval > oldval ){
            cwnd[1]<<Simulator::Now().GetSeconds()<<  " " << newval <<endl;
        }
        else {
            drop <<Simulator::Now().GetSeconds()<<" " << oldval <<  " " <<newval<<endl;
        }
}

static void
CwndTracer3(uint32_t oldval, uint32_t newval)
{
    if ( newval > oldval ){
            cwnd[2]<<Simulator::Now().GetSeconds()<<  " " << newval <<endl;
        }
        else {
            drop <<Simulator::Now().GetSeconds()<<" " << oldval <<  " " <<newval<<endl;
        }
}

static void
CwndTracer4(uint32_t oldval, uint32_t newval)
{
    if ( newval > oldval ){
            cwnd[3]<<Simulator::Now().GetSeconds()<<  " " << newval <<endl;
        }
        else {
            drop <<Simulator::Now().GetSeconds()<<" " << oldval <<  " " <<newval<<endl;
        }
}

static void
CwndTracer5(uint32_t oldval, uint32_t newval)
{   if ( newval > oldval ){
        cwnd[4]<<Simulator::Now().GetSeconds()<<  " " << newval <<endl;
    }
    else {
        drop <<Simulator::Now().GetSeconds()<<" " << oldval <<  " " <<newval<<endl;
    }
}

/**
 * Node 0 Enque Hook
 */
static void
Enqueue(string context, Ptr<const Packet> p)
{
  queueSize++;
  queueFile<<Simulator::Now ().GetSeconds()<<"\t EQ \t"<<queueSize<<endl;
}

/**
 * Node 0 Deque Hook
 */
static void
Dequeue(string context, Ptr<const Packet> p)
{
  queueSize--;
  queueFile<<Simulator::Now ().GetSeconds()<<"\t DQ \t"<<queueSize<<endl;

}

static void
Drop(string context, Ptr<const Packet> p)
{
  queueFile<<Simulator::Now ().GetSeconds()<<"\t DR \t"<<queueSize<<endl;
}


/**
 * Node - 1 Receive Packet Hook
 */
/*static void
ReceivePacket (string context, Ptr<const Packet> p, const Address& addr)
{
        int currentPacketPort;
        currentPacketPort = InetSocketAddress::ConvertFrom(addr).GetPort();
        packetCount++;
        if(packetCount<10) // To handle the initial 10 packets
        {
                initialTime[packetCount]= Simulator::Now ().GetSeconds();
                packetSize[packetCount]= p->GetSize();
        }
        else
        {
                finalTime = Simulator::Now ().GetSeconds();
                diffTime = finalTime - initialTime[packetCount%10];
                sumPacketSize=0;
                for(int i=0;i<10;i++) sumPacketSize+=packetSize[i];

                totalLength=sumPacketSize/diffTime;


              switch(currentPacketPort){
                case 49153:
                  recvfile[0]<<finalTime<<"\t"<<totalLength<<endl;
                  break;
                case 49154:
                  recvfile[1]<<finalTime<<"\t"<<totalLength<<endl;
                  break;
                 case 49155:
                  recvfile[2]<<finalTime<<"\t"<<totalLength<<endl;
                  break;
                case 49156:
                  recvfile[3]<<finalTime<<"\t"<<totalLength<<endl;
                  break;
                case 49157:
                  recvfile[4]<<finalTime<<"\t"<<totalLength<<endl;
                  break;
              }

                initialTime[packetCount%10]=finalTime;
                packetSize[packetCount%10]=p->GetSize();
        }
}*/


int
main (int argc, char *argv[])
{

  /**
   * File Input Output Code
   */
  string fileprefix = "Cwnd";
  string recvprefix = "Recv";
  for (int i = 0; i < 5; ++i)
  {
    stringstream ss;
    ss << i;
    string str = ss.str();
    string filename = fileprefix + str + ".dat";
    cwnd[i].open(filename.c_str());


    filename = recvprefix + str + ".dat";
    recvfile[i].open(filename.c_str());
  }
  queueFile.open("queue.dat");
  drop.open("drop.dat");
  throughput_data.open ("throughput.dat");


  /**
   * Preparing the simulator
   */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Set the size of the sending queue

  // Uncomment the below statement to enable logging
  LogComponentEnable("Lab4-3", LOG_LEVEL_INFO);

  //string tcpType = "NewReno";
  //string tcpType = "Vegas";
  //string tcpType = "Westwood";
  //string tcpType = "Veno";
  //string tcpType = "Hybla";
  //string tcpType = "Bic";
  string tcpType = "Yeah";


  // Command Line parsing
  CommandLine cmd;
  cmd.AddValue ("Tcp", "Tcp type: 'NewReno' or 'Tahoe'", tcpType);
  cmd.Parse (argc, argv);

  // Set the TCP Socket Type
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName ("ns3::Tcp" + tcpType)));
  //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpReno"));
  NS_LOG_INFO ("Creating Topology");

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
  pointToPoint.SetChannelAttribute("Delay", TimeValue(Time("10ms")));
  pointToPoint.SetQueue("ns3::DropTailQueue","MaxPackets",UintegerValue(100));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("172.16.24.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);
  uint16_t sinkPort = 9000;

  /**
   * Creating 5 tcp sinks at node 1
   */
  double a[5] = { 0.2, 0.4, 0.6, 0.8, 1.0};
  double b[5] = { 1.8, 1.8, 1.2, 1.4, 1.6};
  for (int i = 0; i < 5; ++i)
  {
      PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort+i));
      ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
      sinkApps.Start (Seconds (a[i]));
      sinkApps.Stop (Seconds (b[i]));
  }

  uint16_t ftp_port = 3128;
  Address TxAddress(InetSocketAddress(interfaces.GetAddress(1),ftp_port));
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", TxAddress);

  // setting up attributes for onoff application helper
  clientHelper.SetAttribute("DataRate",StringValue("1Mbps"));
  clientHelper.SetAttribute("PacketSize",UintegerValue(1000));
  ApplicationContainer ftp = clientHelper.Install (nodes.Get (0));
  ftp.Start (Seconds (0.0));
  ftp.Stop (Seconds (1.8));


  // First tcp source
  Address sinkAddress1 (InetSocketAddress(interfaces.GetAddress (1), sinkPort));
  Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  ns3TcpSocket1->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndTracer1));
  Ptr<MyApp> app1 = CreateObject<MyApp> ();
  app1->Setup (ns3TcpSocket1, sinkAddress1, 1000, DataRate ("300Kbps"));
  nodes.Get (0)->AddApplication (app1);
  app1->SetStartTime (Seconds (0.2));
  app1->SetStopTime (Seconds (1.8));


  // Second tcp source
  Address sinkAddress2 (InetSocketAddress(interfaces.GetAddress (1), sinkPort+1));
  Ptr<Socket> ns3TcpSocket2 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  ns3TcpSocket2->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndTracer2));
  Ptr<MyApp> app2 = CreateObject<MyApp> ();
  app2->Setup (ns3TcpSocket2, sinkAddress2, 1000, DataRate ("300Kbps"));
  nodes.Get (0)->AddApplication (app2);
  app2->SetStartTime (Seconds ( 0.4));
  app2->SetStopTime (Seconds (1.8));


  // third tcp source
  Address sinkAddress3 (InetSocketAddress(interfaces.GetAddress (1), sinkPort+2));
  Ptr<Socket> ns3TcpSocket3 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  ns3TcpSocket3->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndTracer3));
  Ptr<MyApp> app3 = CreateObject<MyApp> ();
  app3->Setup (ns3TcpSocket3, sinkAddress3, 1000, DataRate ("300Kbps"));
  nodes.Get (0)->AddApplication (app3);
  app3->SetStartTime (Seconds (0.6));
  app3->SetStopTime (Seconds (1.2));

  // fourth tcp source
  Address sinkAddress4 (InetSocketAddress(interfaces.GetAddress (1), sinkPort+3));
  Ptr<Socket> ns3TcpSocket4 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  ns3TcpSocket4->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndTracer4));
  Ptr<MyApp> app4 = CreateObject<MyApp> ();
  app4->Setup (ns3TcpSocket4, sinkAddress4, 1000, DataRate ("300Kbps"));
  nodes.Get (0)->AddApplication (app4);
  app4->SetStartTime (Seconds (0.8));
  app4->SetStopTime (Seconds (1.4));

  // fifth tcp source
  Address sinkAddress5 (InetSocketAddress(interfaces.GetAddress (1), sinkPort+4));
  Ptr<Socket> ns3TcpSocket5 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  ns3TcpSocket5->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndTracer5));
  Ptr<MyApp> app5 = CreateObject<MyApp> ();
  app5->Setup (ns3TcpSocket5, sinkAddress5, 1000, DataRate ("300Kbps"));
  nodes.Get (0)->AddApplication (app5);
  app5->SetStartTime (Seconds (1.0));
  app5->SetStopTime (Seconds (1.6));

  // Node 0 p2p device tx queue context
  string context = "/NodeList/0/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/";

  // Attaching hooks.
  Config::Connect (context + "Enqueue", MakeCallback (&Enqueue));
  Config::Connect (context + "Dequeue", MakeCallback (&Dequeue));
  Config::Connect (context + "Drop", MakeCallback (&Drop));

  // Node 1 all tcp receiver hook
  //context = "/NodeList/1/ApplicationList/*/$ns3::PacketSink/Rx";
  //Config::Connect (context, MakeCallback(&ReceivePacket));

  context = "/NodeList/1/ApplicationList/0/$ns3::PacketSink/Rx";
  Config::Connect (context, MakeCallback(&ReceiveNode1Packet));

  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("sixth.pcap", std::ios::out, PcapHelper::DLT_PPP);

  devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));
  devices.Get (1)->TraceConnectWithoutContext ("MacTxDrop", MakeBoundCallback (&TxDrop, file));

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop (Seconds(1.8));
  Simulator::Run ();

  // Flowmonitor Analysis
  monitor->CheckForLostPackets ();

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
      {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        if ((t.sourceAddress=="172.16.24.1" && t.destinationAddress == "172.16.24.2"))
        {
            cout << "Flow " << (i->first)/2 + 1  << " (" << t.sourceAddress<<":"<<t.sourcePort << " -> " << t.destinationAddress <<":"<<t.destinationPort<< ")\n";
            cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
            cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
            cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
            //cout << " lostpackets:" << i->second.bytesDropped <<"\n";
        }
      }
     monitor->SerializeToXmlFile("lab-2.flowmon", true, true);

  Simulator::Destroy ();

  // Closing Down files.
  for (int i = 0; i < 5; ++i)
  {
    cwnd[i].close();
    recvfile[i].close();
  }

  queueFile.close();
  return 0;
}
