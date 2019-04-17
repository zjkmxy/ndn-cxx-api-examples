#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ndn-cxx/face.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <functional>
using namespace ndn;

const std::string prefix = "/room/aircon";

class AirCon{
public:
  AirCon():m_state("off"){}

  void run(){
    m_face.registerPrefix(prefix,
                          RegisterPrefixSuccessCallback(),
                          bind(&AirCon::onRegisterFailed, this, _1, _2));
    m_face.setInterestFilter(prefix + "/command",
                             std::bind(&AirCon::onCommand, this, _2));
    m_face.setInterestFilter(prefix + "/state",
                             std::bind(&AirCon::onInterest, this, _2));
    m_face.processEvents();
  }

private:
  void onInterest(const Interest& interest){
    Data data(Name(interest.getName()).appendTimestamp());
    data.setFreshnessPeriod(10_ms);
    data.setContent(reinterpret_cast<const uint8_t*>(m_state.c_str()), m_state.length() + 1);
    m_keyChain.sign(data);
    std::cout << "Name: " << interest.getName().toUri() 
              << " State: " << m_state 
              << std::endl;
    m_face.put(data);
  }

  void onCommand(const Interest& interest){
    Data data(interest.getName());
    m_state = interest.getName()[-1].toUri();
    m_keyChain.sign(data);
    std::cout << "Name: " << interest.getName().toUri() 
              << " State: " << m_state 
              << std::endl;
    m_face.put(data);
  }

  void onRegisterFailed(const Name& prefix, const std::string& reason){
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();
  }


private:
  Face m_face;
  KeyChain m_keyChain;
  std::string m_state;
};

int main(int argc, char* argv[]){
  AirCon app;
  try {
    app.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
