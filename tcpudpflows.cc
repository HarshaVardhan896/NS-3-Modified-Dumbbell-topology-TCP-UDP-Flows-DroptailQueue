#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/gnuplot.h"
#include "ns3/drop-tail-queue.h" 

using namespace ns3;

class MyApp : public Application 
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

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
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0), 
    m_peer (), 
    m_packetSize (0), 
    m_nPackets (0), 
    m_dataRate (0), 
    m_sendEvent (), 
    m_running (false), 
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
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

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
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

static void
RxDrop (Ptr<const Packet> p)
{
   std::cout<<"RxDrop at " << Simulator::Now ().GetSeconds ();
}



int main (int argc, char *argv[])
{
  std :: string fileNameWithNoExtension = "Packetdrop";
  std :: string graphicsFileName  = fileNameWithNoExtension + ".png";
  std :: string plotFileName  = fileNameWithNoExtension + ".plt";
  std :: string plotTitle  = "Graph-3";
  std :: string dataTitle1  = "TCP Packetdrop rate varied over Time";
  std :: string dataTitle2  = "UDP Packetdrop rate varied over Time";

  // Instantiate the plot and set its title.
  Gnuplot plot(graphicsFileName);
  plot.SetTitle(plotTitle);

  // Make the graphics file, which the plot file will create when it
  // is used with Gnuplot, be a PNG file.
  plot.SetTerminal("png");
  // Set the labels for each axis.
  plot.SetLegend("Time", "Packets dropped");
  // Set the range for the x axis.
  plot.AppendExtra("set xrange [0:10]");
  // Instantiate the dataset, set its title, and make the points be
  // plotted along with connecting lines.
  Gnuplot2dDataset dataset1;
  Gnuplot2dDataset dataset2;
  dataset1.SetTitle(dataTitle1);
  dataset2.SetTitle(dataTitle2);
  dataset1.SetStyle(Gnuplot2dDataset::LINES_POINTS);
  dataset2.SetStyle(Gnuplot2dDataset::LINES_POINTS);
 for(int simtime=1;simtime<=10;simtime+=1)
{
 
  uint32_t    nLeftLeaf = 4;
  uint32_t    nRightLeaf = 2;
  uint32_t    nLeaf = 0; // If non-zero, number of both left and right

  if (nLeaf > 0)
    {
      nLeftLeaf = nLeaf;
      nRightLeaf = nLeaf;
    }

  // Create the point-to-point link helpers
  PointToPointHelper pointToPointRouter;
  pointToPointRouter.SetQueue ("ns3::DropTailQueue");
  pointToPointRouter.SetDeviceAttribute  ("DataRate", StringValue ("800Kbps"));
  pointToPointRouter.SetChannelAttribute ("Delay", StringValue ("100ms"));
  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("100ms"));
  
  PointToPointDumbbellHelper d (nLeftLeaf, pointToPointLeaf,nRightLeaf, pointToPointLeaf,pointToPointRouter);
                                
  // Install Stack
  InternetStackHelper stack;
  d.InstallStack (stack);

  // Assign IP Addresses
  d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));
                         
   // Set up the acutal simulation
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();                       
                         
 //UDP connections code      
  uint16_t sinkPortc = 8003;
  Address sinkAddressc(InetSocketAddress (d.GetRightIpv4Address(1), sinkPortc));
  PacketSinkHelper packetSinkHelperc("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPortc));
  ApplicationContainer sinkAppsc = packetSinkHelperc.Install (d.GetRight(1));
  sinkAppsc.Start (Seconds (0.0));
  sinkAppsc.Stop (Seconds (10.0));
  

  Ptr<Socket> ns3TcpSocketc = Socket::CreateSocket (d.GetLeft(2), UdpSocketFactory::GetTypeId ());

  Ptr<MyApp> appc = CreateObject<MyApp> ();
  appc->Setup (ns3TcpSocketc, sinkAddressc, 100, 100000, DataRate ("800Kbps"));
  d.GetLeft(2)->AddApplication (appc);
  appc->SetStartTime (Seconds (5.0));
  appc->SetStopTime (Seconds (10.0)); 



  uint16_t sinkPortd = 8088;
  Address sinkAddressd(InetSocketAddress (d.GetRightIpv4Address(1), sinkPortd));
  PacketSinkHelper packetSinkHelperd("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPortd));
  ApplicationContainer sinkAppsd = packetSinkHelperd.Install (d.GetRight(1));
  sinkAppsd.Start (Seconds (0.0));
  sinkAppsd.Stop (Seconds (10.0));
  

  Ptr<Socket> ns3TcpSocketd = Socket::CreateSocket (d.GetLeft(3), UdpSocketFactory::GetTypeId ());

  Ptr<MyApp> appd = CreateObject<MyApp> ();
  appd->Setup (ns3TcpSocketd, sinkAddressd, 100, 100000,DataRate ("800Kbps"));
  d.GetLeft(3)->AddApplication (appd);
  appd->SetStartTime (Seconds (5.0));
  appd->SetStopTime (Seconds (10.0)); 
  
   //TCP code
  uint16_t sinkPorta = 8080;
  Address sinkAddressa(InetSocketAddress (d.GetRightIpv4Address(0), sinkPorta));
  PacketSinkHelper packetSinkHelpera("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPorta));
  ApplicationContainer sinkAppsa = packetSinkHelpera.Install (d.GetRight(0));
  sinkAppsa.Start (Seconds (0.0));
  sinkAppsa.Stop (Seconds (10.0));
  

  Ptr<Socket> ns3TcpSocketa = Socket::CreateSocket (d.GetLeft(0), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> appa = CreateObject<MyApp> ();
  appa->Setup (ns3TcpSocketa, sinkAddressa, 100, 100000,DataRate ("800Kbps"));
  d.GetLeft(0)->AddApplication (appa);
  appa->SetStartTime (Seconds (1.0));
  appa->SetStopTime (Seconds (10.0));
 

  uint16_t sinkPortb = 8000;
  Address sinkAddressb(InetSocketAddress (d.GetRightIpv4Address(0), sinkPortb));
  PacketSinkHelper packetSinkHelperb("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPortb));
  ApplicationContainer sinkAppsb = packetSinkHelperb.Install (d.GetRight(0));
  sinkAppsb.Start (Seconds (0.0));
  sinkAppsb.Stop (Seconds (10.0));

  Ptr<Socket> ns3TcpSocketb = Socket::CreateSocket (d.GetLeft(1), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> appb = CreateObject<MyApp> ();
  appb->Setup (ns3TcpSocketb, sinkAddressb, 100, 100000,DataRate ("800Kbps"));
  d.GetLeft(1)->AddApplication (appb);
  appb->SetStartTime (Seconds (1.0));
  appb->SetStopTime (Seconds (10.0));                 
                         

  d.GetLeft(0)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop)); 
 //
// Calculate Throughput using Flowmonitor
//
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
 
  Simulator::Stop (Seconds(simtime));
  Simulator::Run ();

	monitor->CheckForLostPackets ();

  std::cout << "\nTime  " <<Simulator::Now ().GetSeconds ()<< "\n";
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  int Sumt = 0;
  int p=1;
  int Sumu=0;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      //Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      int pd=i->second.lostPackets;
      if(p<5)
      Sumt += pd;
      else 
      Sumu+=pd;
      p++;
    }

    dataset1.Add(Simulator::Now ().GetSeconds (), Sumt);
    dataset2.Add(Simulator::Now ().GetSeconds (), Sumu);
    std :: cout << " TCPPacketsdropped: " << Sumt << std :: endl;
    std :: cout << " UDPPacketsdropped: " << Sumu << std :: endl;


  monitor->SerializeToXmlFile("lab-1.flowmon", true, true);

  
  Simulator::Destroy ();
}

  plot.AddDataset(dataset1);
  plot.AddDataset(dataset2);
  // Open the plot file.
  std :: ofstream plotFile(plotFileName.c_str());
  // Write the plot file.
  plot.GenerateOutput(plotFile);
  // Close the plot file.
  plotFile.close();
  return 0;
}
