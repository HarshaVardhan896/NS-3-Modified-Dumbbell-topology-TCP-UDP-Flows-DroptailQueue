/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: George F. Riley<riley@ece.gatech.edu>
 */

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
#include "ns3/netanim-module.h"
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





int main (int argc, char *argv[])
{
  std :: string fileNameWithNoExtension = "Throughput";
  std :: string graphicsFileName  = fileNameWithNoExtension + ".png";
  std :: string plotFileName  = fileNameWithNoExtension + ".plt";
  std :: string plotTitle  = "Graph";
  std :: string dataTitle  = "Throughput varied over time";


  // Instantiate the plot and set its title.
  Gnuplot plot(graphicsFileName);
  plot.SetTitle(plotTitle);

  // Make the graphics file, which the plot file will create when it
  // is used with Gnuplot, be a PNG file.
  plot.SetTerminal("png");
  // Set the labels for each axis.
  plot.SetLegend("Time(in seconds)", "Throughput");
  // Set the range for the x axis.
  plot.AppendExtra("set xrange [0:11]");
  // Instantiate the dataset, set its title, and make the points be
  // plotted along with connecting lines.
  Gnuplot2dDataset dataset;
  dataset.SetTitle(dataTitle);
  dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

 for(int simtime=1;simtime<=10;simtime++)
{
 
  uint32_t    nLeftLeaf = 4;
  uint32_t    nRightLeaf = 2;
  uint32_t    nLeaf = 0; // If non-zero, number of both left and right
  std::string animFile = "networklab3.xml" ;  // Name of file for animation output

  if (nLeaf > 0)
    {
      nLeftLeaf = nLeaf;
      nRightLeaf = nLeaf;
    }

  // Create the point-to-point link helpers
  PointToPointHelper pointToPointRouter;
  pointToPointRouter.SetQueue ("ns3::DropTailQueue","MaxSize",StringValue("50p"));
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
  appc->Setup (ns3TcpSocketc, sinkAddressc, 100, 100000,  DataRate ("800Kbps"));
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
  appd->Setup (ns3TcpSocketd, sinkAddressd, 100, 100000, DataRate ("800Kbps"));
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
  appa->Setup (ns3TcpSocketa, sinkAddressa, 100, 100000,  DataRate ("800Kbps"));
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
  appb->Setup (ns3TcpSocketb, sinkAddressb, 100, 100000, DataRate ("800Kbps"));
  d.GetLeft(1)->AddApplication (appb);
  appb->SetStartTime (Seconds (1.0));
  appb->SetStopTime (Seconds (10.0));
                    
                         
  // Set the bounding box for animation
  d.BoundingBox (1, 1, 100, 100);

  // Create the animation object and configure for specified output
  AnimationInterface anim (animFile);
  anim.EnablePacketMetadata (); // Optional
  anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (10)); // Optional
  
 //
// Calculate Throughput using Flowmonitor
//
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
 
  Simulator::Stop (Seconds(simtime));
  Simulator::Run ();

	monitor->CheckForLostPackets ();

  std::cout << "\nTime: " << Simulator::Now ().GetSeconds () << "\n";
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  double Sumx = 0;
  int n=0;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
	   Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      
      std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ") - ";
      std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
      std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
        
      double TPut = i->second.rxBytes * 8.0 /(i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024;
      Sumx += TPut;
      n++;
    }

    dataset.Add(Simulator::Now ().GetSeconds (), Sumx/n);
    std :: cout << " NetThroughput: " << Sumx/n << std :: endl;


  monitor->SerializeToXmlFile("lab-1.flowmon", true, true);

  
  Simulator::Destroy ();
}

  plot.AddDataset(dataset);
  // Open the plot file.
  std :: ofstream plotFile(plotFileName.c_str());
  // Write the plot file.
  plot.GenerateOutput(plotFile);
  // Close the plot file.
  plotFile.close();
  return 0;
}
