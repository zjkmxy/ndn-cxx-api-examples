#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ndn-cxx/face.hpp>
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
    m_face.setInterestFilter(Name("/room/temp").append(sensor_id),
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
    Data data(interest.getName());
    data.setFreshnessPeriod(10_ms);
    data.setContent(reinterpret_cast<uint8_t*>(&temperature), sizeof(temperature));
    m_keyChain.sign(data);
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
