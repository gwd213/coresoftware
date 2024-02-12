#include "SingleMbdTriggerInput.h"

#include "Fun4AllPrdfInputTriggerManager.h"
#include "InputManagerType.h"

#include <ffarawobjects/MbdPacketv1.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>    // for PHIODataNode
#include <phool/PHNode.h>          // for PHNode
#include <phool/PHNodeIterator.h>  // for PHNodeIterator
#include <phool/PHObject.h>        // for PHObject
#include <phool/getClass.h>
#include <phool/phool.h>

#include <Event/Event.h>
#include <Event/EventTypes.h>
#include <Event/Eventiterator.h>
#include <Event/packet.h>  // for Packet

#include <cstdint>   // for uint64_t
#include <iostream>  // for operator<<, basic_ostream<...
#include <iterator>  // for reverse_iterator
#include <limits>    // for numeric_limits
#include <memory>
#include <set>
#include <utility>  // for pair

SingleMbdTriggerInput::SingleMbdTriggerInput(const std::string &name)
  : SingleTriggerInput(name)
{
  SubsystemEnum(InputManagerType::MBD);
}

SingleMbdTriggerInput::~SingleMbdTriggerInput()
{
  CleanupUsedPackets(std::numeric_limits<int>::max());
}

void SingleMbdTriggerInput::FillPool(const unsigned int /*nbclks*/)
{
  if (AllDone())  // no more files and all events read
  {
    return;
  }
  while (GetEventiterator() == nullptr)  // at startup this is a null pointer
  {
    if (!OpenNextFile())
    {
      AllDone(1);
      return;
    }
  }
  while (GetSomeMoreEvents())
  {
    std::unique_ptr<Event> evt(GetEventiterator()->getNextEvent());
    while (!evt)
    {
      fileclose();
      if (!OpenNextFile())
      {
        AllDone(1);
        return;
      }
      evt.reset(GetEventiterator()->getNextEvent());
    }
    if (Verbosity() > 2)
    {
      std::cout << PHWHERE << "Fetching next Event" << evt->getEvtSequence() << std::endl;
    }
    RunNumber(evt->getRunNumber());
    if (GetVerbosity() > 1)
    {
      evt->identify();
    }
    if (evt->getEvtType() != DATAEVENT)
    {
      m_NumSpecialEvents++;
      continue;
    }
    int EventSequence = evt->getEvtSequence();
    Packet *packet = evt->getPacket(14001);

    if (Verbosity() > 1)
    {
      packet->identify();
    }

    // by default use previous bco clock for gtm bco
    MbdPacketv1 *newhit = new MbdPacketv1();
    uint64_t gtm_bco = packet->lValue(0, "BCO");
    newhit->setBCO(packet->lValue(0, "BCO"));
    newhit->setIdentifier(packet->getIdentifier());
    newhit->setEvtSequence(EventSequence);
    newhit->setBunchNumber(packet->lValue(0, "BunchNumber"));
    if (Verbosity() > 2)
    {
      std::cout << PHWHERE << "evtno: " << EventSequence
                << ", bco: 0x" << std::hex << gtm_bco << std::dec
                << std::endl;
    }
    if (TriggerInputManager())
    {
       TriggerInputManager()->AddMbdPacket(EventSequence, newhit);
    }
    m_MbdPacketMap[EventSequence].push_back(newhit);
    m_EventStack.insert(EventSequence);

    delete packet;
  }
}

void SingleMbdTriggerInput::Print(const std::string &what) const
{
  if (what == "ALL" || what == "STORAGE")
  {
    for (const auto &bcliter : m_MbdPacketMap)
    {
      std::cout << PHWHERE << "Event: " << bcliter.first << std::endl;
    }
  }
  if (what == "ALL" || what == "STACK")
  {
    for (auto iter : m_EventStack)
    {
      std::cout << PHWHERE << "stacked event: " << iter << std::endl;
    }
  }
}

void SingleMbdTriggerInput::CleanupUsedPackets(const int eventno)
{
  std::vector<int> toclearevents;
  for (const auto &iter : m_MbdPacketMap)
  {
    if (iter.first <= eventno)
    {
      for (auto pktiter : iter.second)
      {
        delete pktiter;
      }
      toclearevents.push_back(iter.first);
    }
    else
    {
      break;
    }
  }

  for (auto iter : toclearevents)
  {
    m_EventStack.erase(iter);
    m_MbdPacketMap.erase(iter);
  }
}


void SingleMbdTriggerInput::ClearCurrentEvent()
{
  // called interactively, to get rid of the current event
  int currentevent = *m_EventStack.begin();
  //  std::cout << PHWHERE << "clearing bclk 0x" << std::hex << currentbclk << std::dec << std::endl;
  CleanupUsedPackets(currentevent);
  return;
}

bool SingleMbdTriggerInput::GetSomeMoreEvents()
{
  if (AllDone())
  {
    return false;
  }
  if (m_MbdPacketMap.empty())
  {
    return true;
  }

  int first_event = m_MbdPacketMap.begin()->first;
  int last_event = m_MbdPacketMap.rbegin()->first;
  if (Verbosity() > 1)
  {
    std::cout << PHWHERE << "first event: " << first_event
              << " last event: " << last_event
              << std::endl;
  }
  if (first_event >= last_event)
  {
    return true;
  }
  return false;
}

void SingleMbdTriggerInput::CreateDSTNode(PHCompositeNode *topNode)
{
  PHNodeIterator iter(topNode);
  PHCompositeNode *dstNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
  {
    dstNode = new PHCompositeNode("DST");
    topNode->addNode(dstNode);
  }
  PHNodeIterator iterDst(dstNode);
  PHCompositeNode *detNode = dynamic_cast<PHCompositeNode *>(iterDst.findFirst("PHCompositeNode", "MBD"));
  if (!detNode)
  {
    detNode = new PHCompositeNode("MBD");
    dstNode->addNode(detNode);
  }
  OfflinePacket *mbdhitcont = findNode::getClass<OfflinePacket>(detNode, "MBDPacket");
  if (!mbdhitcont)
  {
    mbdhitcont = new MbdPacketv1();
    PHIODataNode<PHObject> *newNode = new PHIODataNode<PHObject>(mbdhitcont, "MBDPacket", "PHObject");
    detNode->addNode(newNode);
  }
}

// void SingleMbdTriggerInput::ConfigureStreamingInputManager()
// {
//   if (StreamingInputManager())
//   {
//     StreamingInputManager()->SetMbdBcoRange(m_BcoRange);
//   }
//   return;
// }
