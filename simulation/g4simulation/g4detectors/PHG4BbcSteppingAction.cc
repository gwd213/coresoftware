#include "PHG4BbcSteppingAction.h"
#include "PHG4BbcDetector.h"
#include "PHG4StepStatusDecode.h"

#include <g4main/PHG4Hit.h>
#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hitv1.h>
#include <g4main/PHG4Shower.h>
#include <g4main/PHG4SteppingAction.h>         // for PHG4SteppingAction
#include <g4main/PHG4TrackUserInfoV1.h>

#include <phool/getClass.h>
#include <phool/phool.h>                       // for PHWHERE

#include <TSystem.h>

#include <Geant4/G4ParticleDefinition.hh>      // for G4ParticleDefinition
#include <Geant4/G4ReferenceCountedHandle.hh>  // for G4ReferenceCountedHandle
#include <Geant4/G4Step.hh>
#include <Geant4/G4StepPoint.hh>               // for G4StepPoint
#include <Geant4/G4StepStatus.hh>              // for fGeomBoundary, fAtRest...
#include <Geant4/G4String.hh>                  // for G4String
#include <Geant4/G4SystemOfUnits.hh>
#include <Geant4/G4ThreeVector.hh>             // for G4ThreeVector
#include <Geant4/G4TouchableHandle.hh>         // for G4TouchableHandle
#include <Geant4/G4Track.hh>                   // for G4Track
#include <Geant4/G4TrackStatus.hh>             // for fStopAndKill
#include <Geant4/G4Types.hh>                   // for G4double
#include <Geant4/G4VPhysicalVolume.hh>         // for G4VPhysicalVolume
#include <Geant4/G4VTouchable.hh>              // for G4VTouchable
#include <Geant4/G4VUserTrackInformation.hh>   // for G4VUserTrackInformation

#include <cmath>                               // for isfinite
#include <iostream>
#include <string>                              // for operator<<, string

class PHCompositeNode;

using namespace std;
//____________________________________________________________________________..
PHG4BbcSteppingAction::PHG4BbcSteppingAction(PHG4BbcDetector* detector, const PHParameters* parameters)
  : PHG4SteppingAction(detector->GetName())
  , m_Detector(detector)
//  , m_Params(parameters)
  , m_HitContainer(nullptr)
  , m_Hit(nullptr)
  , m_SaveHitContainer(nullptr)
  , m_SaveVolPre(nullptr)
  , m_SaveVolPost(nullptr)
  , m_SaveTrackId(-1)
  , m_SavePreStepStatus(-1)
  , m_SavePostStepStatus(-1)
//  , m_ActiveFlag(m_Params->get_int_param("active"))
//  , m_BlackHoleFlag(m_Params->get_int_param("blackhole"))
  , m_EdepSum(0)
  , m_EionSum(0)
{}

PHG4BbcSteppingAction::~PHG4BbcSteppingAction()
{
  // if the last hit was a zero energie deposit hit, it is just reset
  // and the memory is still allocated, so we need to delete it here
  // if the last hit was saved, hit is a nullptr pointer which are
  // legal to delete (it results in a no operation)
  delete m_Hit;
}

//____________________________________________________________________________..
bool PHG4BbcSteppingAction::UserSteppingAction(const G4Step* aStep, bool was_used)
{
  //cout << PHWHERE << " In PHG4BbcSteppingAction::UserSteppingAction()" << endl;

  G4TouchableHandle touch = aStep->GetPreStepPoint()->GetTouchableHandle();
  G4TouchableHandle touchpost = aStep->GetPostStepPoint()->GetTouchableHandle();
  // get volume of the current step
  G4VPhysicalVolume* volume = touch->GetVolume();
  // IsInBbc(volume) returns
  //  == 0 outside of bbc
  //  > 0 for hits in active volume
  //  < 0 for hits in passive material
  int whichactive = m_Detector->IsInBbc(volume);
  if (!whichactive)
  {
    return false;
  }

  //cout << PHWHERE << " Found Hit" << endl;

  // collect energy and track length step by step
  G4double edep = aStep->GetTotalEnergyDeposit() / GeV;
  G4double eion = (aStep->GetTotalEnergyDeposit() - aStep->GetNonIonizingEnergyDeposit()) / GeV;
  const G4Track* aTrack = aStep->GetTrack();

  // if this detector stops everything, just put all kinetic energy into edep
  /*
  if (m_BlackHoleFlag)
  {
    edep = aTrack->GetKineticEnergy() / GeV;
    G4Track *killtrack = const_cast<G4Track *>(aTrack);
    killtrack->SetTrackStatus(fStopAndKill);
  }
  */

  bool geantino = false;
  // the check for the pdg code speeds things up, I do not want to make
  // an expensive string compare for every track when we know
  // geantino or chargedgeantino has pid=0
  if (aTrack->GetParticleDefinition()->GetPDGEncoding() == 0 &&
      aTrack->GetParticleDefinition()->GetParticleName().find("geantino") != string::npos)
  {
    geantino = true;
  }

  G4StepPoint* prePoint = aStep->GetPreStepPoint();
  G4StepPoint* postPoint = aStep->GetPostStepPoint();
  //       cout << "track id " << aTrack->GetTrackID() << endl;
  //       cout << "time prepoint: " << prePoint->GetGlobalTime() << endl;
  //       cout << "time postpoint: " << postPoint->GetGlobalTime() << endl;

  int detector_id = touch->GetCopyNumber();
  /*
  if (detector_id != whichactive)
  {
    cout << PHWHERE << " inconsistency between G4 copy number: " 
	 << detector_id << " and module id from detector: "
	 << whichactive << endl;
    //gSystem->Exit(1);
  }
  */

  switch (prePoint->GetStepStatus())
  {
    case fPostStepDoItProc:
      if (m_SavePostStepStatus != fGeomBoundary)
      {
        // this is the okay case, fPostStepDoItProc called in a volume, not first thing inside
        // a new volume, just proceed here
        break;
      }
      else
      {
        // this is an impossible G4 Step print out diagnostic to help debug, not sure if
        // this is still with us
        cout << GetName() << ": New Hit for  " << endl;
        cout << "prestep status: " << PHG4StepStatusDecode::GetStepStatus(prePoint->GetStepStatus())
          << ", poststep status: " << PHG4StepStatusDecode::GetStepStatus(postPoint->GetStepStatus())
          << ", last pre step status: " << PHG4StepStatusDecode::GetStepStatus(m_SavePreStepStatus)
          << ", last post step status: " << PHG4StepStatusDecode::GetStepStatus(m_SavePostStepStatus) << endl;
        cout << "last track: " << m_SaveTrackId
          << ", current trackid: " << aTrack->GetTrackID() << endl;
        cout << "phys pre vol: " << volume->GetName()
          << " post vol : " << touchpost->GetVolume()->GetName() << endl;
        cout << " previous phys pre vol: " << m_SaveVolPre->GetName()
          << " previous phys post vol: " << m_SaveVolPost->GetName() << endl;
      }
    // These are the normal cases
    case fGeomBoundary:
    case fUndefined:
      if (!m_Hit)
      {
        m_Hit = new PHG4Hitv1();
      }
      m_Hit->set_layer(detector_id);
      //here we set the entrance values in cm
      m_Hit->set_x(0, prePoint->GetPosition().x() / cm);
      m_Hit->set_y(0, prePoint->GetPosition().y() / cm);
      m_Hit->set_z(0, prePoint->GetPosition().z() / cm);
      // time in ns
      m_Hit->set_t(0, prePoint->GetGlobalTime() / nanosecond);
      //set the track ID
      m_Hit->set_trkid(aTrack->GetTrackID());
      m_SaveTrackId = aTrack->GetTrackID();
      if (G4VUserTrackInformation* p = aTrack->GetUserInformation())
      {
        if (PHG4TrackUserInfoV1* pp = dynamic_cast<PHG4TrackUserInfoV1*>(p))
        {
          m_Hit->set_trkid(pp->GetUserTrackId());
        }
      }
      //set the initial energy deposit
      m_EdepSum = 0;
      if (whichactive > 0) 
      {
        m_EionSum = 0;
        m_Hit->set_eion(0);
        m_SaveHitContainer = m_HitContainer;
      }
      else
      {
        cout << "implement stuff for whichactive < 0" << endl;
        gSystem->Exit(1);
      }
      if (G4VUserTrackInformation* p = aTrack->GetUserInformation())
      {
        if (PHG4TrackUserInfoV1* pp = dynamic_cast<PHG4TrackUserInfoV1*>(p))
        {
          m_Hit->set_trkid(pp->GetUserTrackId());
          pp->GetShower()->add_g4hit_id(m_SaveHitContainer->GetID(), m_Hit->get_hit_id());
        }
      }

    break;
  default:
    break;
  }
    
    // some sanity checks for inconsistencies
    // check if this hit was created, if not print out last post step status
    if (!m_Hit || !isfinite(m_Hit->get_x(0)))
    {
      cout << GetName() << ": hit was not created" << endl;
      cout << "prestep status: " << PHG4StepStatusDecode::GetStepStatus(prePoint->GetStepStatus())
           << ", poststep status: " << PHG4StepStatusDecode::GetStepStatus(postPoint->GetStepStatus())
           << ", last pre step status: " << PHG4StepStatusDecode::GetStepStatus(m_SavePreStepStatus)
           << ", last post step status: " << PHG4StepStatusDecode::GetStepStatus(m_SavePostStepStatus) << endl;
      cout << "last track: " << m_SaveTrackId
           << ", current trackid: " << aTrack->GetTrackID() << endl;
      cout << "phys pre vol: " << volume->GetName()
           << " post vol : " << touchpost->GetVolume()->GetName() << endl;
      cout << " previous phys pre vol: " << m_SaveVolPre->GetName()
           << " previous phys post vol: " << m_SaveVolPost->GetName() << endl;
      gSystem->Exit(1);
    }
    // check if track id matches the initial one when the hit was created
    if (aTrack->GetTrackID() != m_SaveTrackId)
    {
      cout << GetName() << ": hits do not belong to the same track" << endl;
      cout << "saved track: " << m_SaveTrackId
           << ", current trackid: " << aTrack->GetTrackID()
           << ", prestep status: " << prePoint->GetStepStatus()
           << ", previous post step status: " << m_SavePostStepStatus
           << endl;

      gSystem->Exit(1);
    }
    m_SavePreStepStatus = prePoint->GetStepStatus();
    m_SavePostStepStatus = postPoint->GetStepStatus();
    m_SaveVolPre = volume;
    m_SaveVolPost = touchpost->GetVolume();

    // here we just update the exit values, it will be overwritten
    // for every step until we leave the volume or the particle
    // ceases to exist
    //sum up the energy to get total deposited
    m_EdepSum += edep;
    if (whichactive > 0)
    {
      m_EionSum += eion;
    }
    // if any of these conditions is true this is the last step in
    // this volume and we need to save the hit
    // postPoint->GetStepStatus() == fGeomBoundary: track leaves this volume
    // postPoint->GetStepStatus() == fWorldBoundary: track leaves this world
    // (happens when your detector goes outside world volume)
    // postPoint->GetStepStatus() == fAtRestDoItProc: track stops (typically
    // aTrack->GetTrackStatus() == fStopAndKill is also set)
    // aTrack->GetTrackStatus() == fStopAndKill: track ends
    if (postPoint->GetStepStatus() == fGeomBoundary ||
        postPoint->GetStepStatus() == fWorldBoundary ||
        postPoint->GetStepStatus() == fAtRestDoItProc ||
        aTrack->GetTrackStatus() == fStopAndKill)
    {
      // save only hits with energy deposit (or geantino)
      if (m_EdepSum > 0 || geantino)
      {
        // update values at exit coordinates and set keep flag
        // of track to keep
        m_Hit->set_x(1, postPoint->GetPosition().x() / cm);
        m_Hit->set_y(1, postPoint->GetPosition().y() / cm);
        m_Hit->set_z(1, postPoint->GetPosition().z() / cm);

      m_Hit->set_t(1, postPoint->GetGlobalTime() / nanosecond);
      if (G4VUserTrackInformation* p = aTrack->GetUserInformation())
      {
	if (PHG4TrackUserInfoV1* pp = dynamic_cast<PHG4TrackUserInfoV1*>(p))
	{
	  pp->SetKeep(1);  // we want to keep the track
	}
      }
      if (geantino)
      {
	m_Hit->set_edep(-1);  // only energy=0 g4hits get dropped, this way geantinos survive the g4hit compression
	if (whichactive > 0)
	{
	  m_Hit->set_eion(-1);
	}
      }
      else
      {
	m_Hit->set_edep(m_EdepSum);
      }
      if (whichactive > 0)
      {
	m_Hit->set_eion(m_EionSum);
      }
      m_SaveHitContainer->AddHit(detector_id, m_Hit);
      // ownership has been transferred to container, set to null
      // so we will create a new hit for the next track
      m_Hit = nullptr;
    }
    else
    {
      // if this hit has no energy deposit, just reset it for reuse
      // this means we have to delete it in the dtor. If this was
      // the last hit we processed the memory is still allocated
      m_Hit->Reset();
    }
  }
  // return true to indicate the hit was used
  return true;
}

//____________________________________________________________________________..
void PHG4BbcSteppingAction::SetInterfacePointers(PHCompositeNode* topNode)
{
  string hitnodename = "G4HIT_" + m_Detector->GetName();
  /*
  if (m_Detector->SuperDetector() != "NONE")
  {
    hitnodename = "G4HIT_" + m_Detector->SuperDetector();
  }
  else
  {
    hitnodename = "G4HIT_" + m_Detector->GetName();
  }
  */

  //now look for the map and grab a pointer to it.
  m_HitContainer = findNode::getClass<PHG4HitContainer>(topNode, hitnodename.c_str());

  // if we do not find the node we need to make it.
  if (!m_HitContainer)
  {
    std::cout << "PHG4BbcSteppingAction::SetTopNode - unable to find " << hitnodename << std::endl;
  }
}
