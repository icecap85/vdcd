//
//  lightbehaviour.hpp
//  p44bridged
//
//  Created by Lukas Zeller on 19.05.13.
//  Copyright (c) 2013 plan44.ch. All rights reserved.
//

#ifndef __p44bridged__lightbehaviour__
#define __p44bridged__lightbehaviour__

#include "device.hpp"

#include "persistentparams.hpp"

using namespace std;

namespace p44 {

  typedef uint8_t Brightness;
  typedef uint8_t SceneNo;


  class LightSettings;

  class LightScene : public PersistentParams
  {
    typedef PersistentParams inherited;
  public:
    LightScene(ParamStore &aParamStore, SceneNo aSceneNo); ///< constructor, sets values according to dS specs' default values

    SceneNo sceneNo; ///< scene number
    Brightness sceneValue; ///< output value for this scene
    bool dontCare; ///< if set, applying this scene does not change the output value
    bool ignoreLocalPriority; ///< if set, local priority is ignored when calling this scene
    bool specialBehaviour; ///< special behaviour active
    bool flashing; ///< flashing active for this scene
    bool slowTransition; ///< set if transition must be slow

    /// @name PersistentParams methods which implement actual storage
    /// @{

    /// SQLIte3 table name to store these parameters to
    virtual const char *tableName();
    /// primary key field definitions
    virtual const FieldDefinition *getKeyDefs();
    /// data field definitions
    virtual const FieldDefinition *getFieldDefs();
    /// load values from passed row
    virtual void loadFromRow(sqlite3pp::query::iterator &aRow, int &aIndex);
    /// bind values to passed statement
    virtual void bindToStatement(sqlite3pp::statement &aStatement, int &aIndex, const char *aParentIdentifier);

    /// @}
  };
  typedef boost::shared_ptr<LightScene> LightScenePtr;
  typedef map<SceneNo, LightScenePtr> LightSceneMap;



  /// the persistent parameters of a device with light behaviour
  class LightSettings : public PersistentParams
  {
    typedef PersistentParams inherited;
    friend class LightScene;
  public:
    LightSettings(ParamStore &aParamStore);
    bool isDimmable; ///< if set, device can be dimmed
    Brightness onThreshold; ///< if !isDimmable, output will be on when output value is >= the threshold
    Brightness minDim; ///< minimal dimming value, dimming down will not go below this
    Brightness maxDim; ///< maximum dimming value, dimming up will not go above this
  private:
    LightSceneMap scenes; ///< the user defined light scenes (default scenes will be created on the fly)
  public:

    /// get the parameters for the scene
    LightScenePtr getScene(SceneNo aSceneNo);

    /// update scene (mark dirty, add to list of non-default scene objects)
    void updateScene(LightScenePtr aScene);

    /// @name PersistentParams methods which implement actual storage
    /// @{

    /// SQLIte3 table name to store these parameters to
    virtual const char *tableName();
    /// data field definitions
    virtual const FieldDefinition *getFieldDefs();
    /// load values from passed row
    virtual void loadFromRow(sqlite3pp::query::iterator &aRow, int &aIndex);
    /// bind values to passed statement
    virtual void bindToStatement(sqlite3pp::statement &aStatement, int &aIndex, const char *aParentIdentifier);
    /// load child parameters (if any)
    virtual ErrorPtr loadChildren();
    /// save child parameters (if any)
    virtual ErrorPtr saveChildren();
    
    /// @}
  };


  class LightBehaviour : public DSBehaviour
  {
    bool localPriority; ///< if set, device is in local priority, i.e. ignores scene calls
    bool isLocigallyOn; ///< if set, device is logically ON (but may be below threshold to enable the output)
    Brightness outputValue; ///< current internal output value. For non-dimmables, output is on only if outputValue>onThreshold
    LightSettings lightSettings; ///< the persistent params of this lighting device

  public:
    /// @return current brightness value, 0..255, linear brightness as perceived by humans (half value = half brightness)
    Brightness getBrightness();

  };

}

#endif /* defined(__p44bridged__lightbehaviour__) */
