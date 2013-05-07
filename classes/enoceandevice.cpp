//
//  enoceandevice.cpp
//  p44bridged
//
//  Created by Lukas Zeller on 07.05.13.
//  Copyright (c) 2013 plan44.ch. All rights reserved.
//

#include "enoceandevice.hpp"


using namespace p44;


EnoceanDevice::EnoceanDevice(EnoceanDeviceContainer *aClassContainerP) :
  Device((DeviceClassContainer *)aClassContainerP)
{
}


void EnoceanDevice::setEnoceanID(EnoceanAddress aAddress, int aSubDeviceIndex)
{
  enoceanAddress = aAddress;
  subDeviceIndex = aSubDeviceIndex;
  deriveDSID();
}


void EnoceanDevice::deriveDSID()
{
  dsid.setObjectClass(DSID_OBJECTCLASS_MACADDRESS);
  // TODO: validate, now we are using the MAC-address class with bits 48..51 set to 6
  dsid.setSerialNo(0x6000000000000ll+enoceanAddress+(((uint64_t)subDeviceIndex&0xF)<<32));
}



string EnoceanDevice::description()
{
  string s = inherited::description();
  string_format_append(s, "Enocean Address = 0x%08lX", enoceanAddress);
  return s;
}
