#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/security/verification-helpers.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/security/v2/validator.hpp>
#include <ndn-cxx/security/v2/validation-callback.hpp>
#include <ndn-cxx/security/v2/certificate-fetcher-offline.hpp>
#include <boost/asio/io_service.hpp>
#include <iostream>
#include <functional>
using namespace ndn;

class Controller{
public:
  Controller(): m_face(m_ioService), m_scheduler(m_ioService){}

  void run(){
    Interest::setDefaultCanBePrefix(true);
    restart();
    m_ioService.run();
  }

private:
  void after10sec(){
    std::cout << "Start a new loop." << std::endl;
    Interest interest("/room/temp");
    interest.setMustBeFresh(true);
    interest.setCanBePrefix(true);

    // sign the Interest
    const auto& identity = m_keyChain.getPib().getIdentity(Name("/zhiyi"));
    m_keyChain.sign(interest, security::signingByIdentity(identity));

    m_face.expressInterest(interest,
                           bind(&Controller::afterGetTemperature, this, _2),
                           bind([this]{ restart(); }),
                           bind([this]{ restart(); }));
  }

  void afterGetTemperature(const Data& data){
    // Approach 1: directly use cert to verify
    const auto& pib = m_keyChain.getPib();
    const auto& identity = pib.getDefaultIdentity();
    const auto& key = identity.getDefaultKey();
    const auto& cert = key.getDefaultCertificate();
    if (security::verifySignature(data, cert)) {
      std::cout << "NICE. Replied Data has a valid signature" << std::endl;
    }

    // Approach 2: use validator config (trust schema)
    namespace ndnsec = ndn::security::v2;
    security::v2::Validator validator(make_unique<ndnsec::ValidationPolicyConfig>(), make_unique<ndnsec::CertificateFetcherOffline>());
    auto& config = static_cast<ndnsec::ValidationPolicyConfig&>(validator.getPolicy());
    config.load("home.conf");
    validator.validate(data,
    [](auto data) {
      std::cout << "ValidatorConfig::NICE. Replied Data has a valid signature" << std::endl;
    },
    [](auto data, auto error) {
      std::cout << "Error is " << error.getInfo() << std::endl;
    });

    int temperature = *reinterpret_cast<const int*>(data.getContent().value());
    std::cout << "Temperature: " << temperature << std::endl;
    m_face.expressInterest(Interest("/room/aircon/state").setMustBeFresh(true),
                           bind(&Controller::afterGetAirconState, this, _2, temperature),
                           bind([this]{ restart(); }),
                           bind([this]{ restart(); }));
  }

  void afterGetAirconState(const Data& data, int temperature){
    std::string aircon_state(reinterpret_cast<const char*>(data.getContent().value()));
    Name aircon_command("/room/aircon/command");
    std::string action = "none";
    if(temperature < 65 && aircon_state != "heat"){
      action = "heat";
    }else if(temperature > 68 && aircon_state == "heat"){
      action = "off";
    }
    std::cout << "State: " << aircon_state << std::endl;
    std::cout << "Action: " << action << std::endl;
    if(action != "none"){
      auto interest = Interest(aircon_command.append(action)).setMustBeFresh(true);
      m_face.expressInterest(interest,
                             DataCallback(),
                             NackCallback(),
                             TimeoutCallback());
    }
    restart();
  }

  void restart(){
    std::cout << "Start to wait 5s..." << std::endl;
    m_scheduler.schedule(5_s, bind(&Controller::after10sec, this));
  }

private:
  boost::asio::io_service m_ioService;
  KeyChain m_keyChain;
  Face m_face;
  Scheduler m_scheduler;
};

int main(int argc, char* argv[]){
  Controller app;
  try {
    app.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
