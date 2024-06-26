// include guard to prevent TpcFFTQA.h from being preprocessed multiple times
#ifndef TpcChanQA_H
#define TpcChanQA_H
//

// Inclusion of necessary header files
#include <fun4all/SubsysReco.h>
#include <string>
#include <vector>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
//

// Call classes to be used in code
class PHCompositeNode;
//

class TpcChanQA : public SubsysReco // Inherit public parts of SubsysReco
{
 // list of public attributes
 public:
  // Function that sets file name and allocated memory to adcSamples
  explicit TpcChanQA(const std::string &name="TpcChanQA.root");

  ~TpcChanQA() override = default;

  // Initialization of run: where you fetch data and place histograms that
  // care about run number
  int InitRun(PHCompositeNode *topNode) override;
  
  // Where you do the desired work associated with the run (analysis)
  int process_event(PHCompositeNode *topNode) override;

  // called at the end of the run when processing is over
  int End(PHCompositeNode *topNode) override;

  // Define function that stores packets to a vector
  void AddPacket(int packet)
  {
    m_packets.push_back(packet);
  }

 // Define vector of packets as protected attribute 
 protected:
  std::vector<int> m_packets;

  // List of private attributes
 private:
  std::string m_fname; // Name of file given to program
  std::string sectorNum; // Sector number associated with data file
  TFile *m_file = nullptr; // File being processed
  TH1F *h_channel_hits = nullptr; // Histogram of hits per channel
  TH2F *h_channel_ADCs = nullptr; // Histogram of ADC counts per channel

  int side = 0; // Face of the TPC (0==North && 1==South)
  int m_packet = 0; // packet number
  int m_nWaveformInFrame = 0; // Number of waveforms in a frame
  int m_Channel = 0; // Channel number
  int m_nSamples = 0; // Number of samples in waveform

  std::vector<unsigned short> m_adcSamples; // ADC values in waveform
};

#endif //ends preprocessing if TpcFFTQA.h has already been defined
