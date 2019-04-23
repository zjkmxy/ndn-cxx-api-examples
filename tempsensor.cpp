#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <functional>
using namespace ndn;

class TempSensor{
public:
  TempSensor(){}

  void run(std::string filename, std::string sensor_id){
    m_file.open(filename);
    m_sensor_id = sensor_id;
    m_face.setInterestFilter("/room/temp",
                             std::bind(&TempSensor::onInterest, this, _2),
                             RegisterPrefixSuccessCallback(),
                             bind(&TempSensor::onRegisterFailed, this, _1, _2));
    m_face.processEvents();
    m_file.close();
  }

private:
  void onInterest(const Interest& interest){
    int temperature;
    if(!(m_file >> temperature)){
      m_file.seekg(std::ios_base::beg);
      m_file >> temperature;
    }
    Data data(Name("/room/temp").append(m_sensor_id).appendTimestamp());
    data.setFreshnessPeriod(10_ms);
    data.setContent(reinterpret_cast<uint8_t*>(&temperature), sizeof(temperature));
    // signed with an identity/key/cert
    const auto& pib = m_keyChain.getPib();
    const auto& identity = pib.getIdentity(Name("/zhiyi"));
    m_keyChain.sign(data, security::signingByIdentity(identity));

    std::cout << "Name: " << interest.getName().toUri() 
              << " Temperature: " << temperature 
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
  std::fstream m_file;
  std::string m_sensor_id;
};

int main(int argc, char* argv[]){
  if(argc != 3){
    std::cout << "Usage: " << argv[0] << " <id> <file>" << std::endl;
    return -1;
  }

  TempSensor app;
  try {
    app.run(argv[2], argv[1]);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
