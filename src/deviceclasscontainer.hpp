//
//  deviceclasscontainer.hpp
//  p44bridged
//
//  Created by Lukas Zeller on 17.04.13.
//  Copyright (c) 2013 plan44.ch. All rights reserved.
//

#ifndef __p44bridged__deviceclasscontainer__
#define __p44bridged__deviceclasscontainer__

#include "p44bridged_common.hpp"

#include "devicecontainer.hpp"

#include "dsid.hpp"

#include <boost/range/detail/any_iterator.hpp>


using namespace std;

namespace p44 {

  // Errors
  typedef enum {
    DeviceClassErrorOK,
    DeviceClassErrorInitialize,
  } DeviceClassErrors;
	
  class DeviceClassError : public Error
  {
  public:
    static const char *domain() { return "DeviceClass"; }
    virtual const char *getErrorDomain() const { return DeviceClassError::domain(); };
    DeviceClassError(DeviceClassErrors aError) : Error(ErrorCode(aError)) {};
    DeviceClassError(DeviceClassErrors aError, std::string aErrorMessage) : Error(ErrorCode(aError), aErrorMessage) {};
  };
	
	
	
  class Device;

  typedef boost::shared_ptr<Device> DevicePtr;


//  template<typename T>  class device_iterator :
//    public std::iterator<std::forward_iterator_tag, /* type of iterator */ T,ptrdiff_t,const T*,const T&> // Info about iterator
//  {
//  public:
//    const T& operator*() const;
//    const T* operator->() const;
//    device_iterator& operator++();
//    device_iterator operator++(int);
//    bool equal(device_iterator const& rhs) const;
//  };
//
//  template<typename T>
//  inline bool operator==(device_iterator<T> const& lhs,device_iterator<T> const& rhs)
//  {
//    return lhs.equal(rhs);
//  }



  class DeviceClassContainer;
  typedef boost::shared_ptr<DeviceClassContainer> DeviceClassContainerPtr;
  typedef boost::weak_ptr<DeviceClassContainer> DeviceClassContainerWeakPtr;
  class DeviceClassContainer
  {
    typedef boost::range_detail::any_iterator<DevicePtr, std::forward_iterator_tag, DevicePtr&, std::ptrdiff_t> iterator;

    DeviceClassContainerWeakPtr mySelf; ///< weak pointer to myself
    DeviceContainer *deviceContainerP; ///< link to the deviceContainer
    int instanceNumber; ///< the instance number identifying this instance among other instances of this class
		string persistentDataDir; ///<directory path to directory where to store persistent data
  public:
    /// @param aInstanceNumber index which uniquely (and as stable as possible) identifies a particular instance
    ///   of this class container. This is used when generating dsids for devices that don't have their own
    ///   unique ID, by using a hashOf(DeviceContainer's id, deviceClassIdentifier(), aInstanceNumber)
    DeviceClassContainer(int aInstanceNumber);

    /// associate with container
    /// @param aDeviceContainerP device container this device class is contained in
    void setDeviceContainer(DeviceContainer *aDeviceContainerP);
    /// get associated container
    /// @return associated device container
    DeviceContainer *getDeviceContainerP() const;

		/// initialize
		/// @param aCompletedCB will be called when initialisation is complete
		///  callback will return an error if initialisation has failed and the device class is not functional
    virtual void initialize(CompletedCB aCompletedCB, bool aFactoryReset);
		
    /// @name iteration
    /// @{

    virtual iterator begin() = 0;
    virtual iterator end() = 0;

    /// @}



    /// @name persistence
    /// @{

    /// set the directory where to store persistent data (databases etc.)
    /// @param aPersistentDataDir full path to directory to save persistent data
    void setPersistentDataDir(const char *aPersistentDataDir);
		/// get the persistent data dir path
		/// @return full path to directory to save persistent data
		const char *getPersistentDataDir();
		
    /// @}
		
		
    /// @name identification
    /// @{

    /// deviceclass identifier
		/// @return constant identifier for this container class (no spaces, filename-safe)
    virtual const char *deviceClassIdentifier() const = 0;
		
    /// Instance number (to differentiate multiple device class containers of the same class)
		/// @return instance index number
		int getInstanceNumber();
		

    /// get a sufficiently unique identifier for this class container
    /// @return ID that identifies this container running on a specific hardware
    ///   the ID should not be dependent on the software version
    ///   the ID must differ for each of multiple device class containers run on the same hardware
    ///   the ID MUST change when same software runs on different hardware
    ///   Usually, MAC address is used as base ID.
    string deviceClassContainerInstanceIdentifier() const;

    /// @}


    /// @name device detection and registration
    /// @{

    /// collect devices from this device classes
    /// @param aCompletedCB will be called when device scan for this device class has been completed
    /// @param aExhaustive if set, device search is made exhaustive (may include longer lasting procedures to
    ///   recollect lost devices, assign bus addresses etc.). Without this flag set, device search should
    ///   still be complete under normal conditions, but might sacrifice corner case detection for speed.
    virtual void collectDevices(CompletedCB aCompletedCB, bool aExhaustive) = 0;

    /// Add device collected from hardware side (bus scan, etc.)
    /// @param aDevice a device object which has a valid dsid
    /// @note this can be called as part of a collectDevices scan, or when a new device is detected
    ///   by other means than a scan/collect operation
    virtual void addDevice(DevicePtr aDevice);


    /// Remove device known no longer connected to the system (for example: explicitly unlearned enOcean switch)
    /// @param aDevice a device object which has a valid dsid
    virtual void removeDevice(DevicePtr aDevice);


    /// Forget all previously collected devices
    virtual void forgetDevices();

		/// @}


    /// description of object, mainly for debug and logging
    /// @return textual description of object
    virtual string description();

  };

} // namespace p44

#endif /* defined(__p44bridged__deviceclasscontainer__) */