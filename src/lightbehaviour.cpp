//
//  lightbehaviour.cpp
//  p44bridged
//
//  Created by Lukas Zeller on 19.05.13.
//  Copyright (c) 2013 plan44.ch. All rights reserved.
//

#include "lightbehaviour.hpp"

using namespace p44;



#pragma mark - LightScene

typedef struct {
  Brightness sceneValue; ///< output value for this scene
  bool slowTransition; ///< set if transition must be slow
  bool flashing; ///< flashing active for this scene
  bool ignoreLocalPriority; ///< if set, local priority is ignored when calling this scene
  bool dontCare; ///< if set, applying this scene does not change the output value
  bool specialBehaviour; ///< special behaviour active
} DefaultSceneParams;

#define NUMDEFAULTSCENES 80 ///< Number of default scenes

static const DefaultSceneParams defaultScenes[NUMDEFAULTSCENES+1] = {
  // group related scenes
  { 0, false, false, false, false, false }, // 0 : Preset 0 - T0_S0
  { 0, false, false, true, false, false }, // 1 : Area 1 Off - T1_S0
  { 0, false, false, true, false, false }, // 2 : Area 2 Off - T2_S0
  { 0, false, false, true, false, false }, // 3 : Area 3 Off - T3_S0
  { 0, false, false, true, false, false }, // 4 : Area 4 Off - T4_S0
  { 255, false, false, false, false, false }, // 5 : Preset 1 - T0_S1
  { 255, false, false, true, false, false }, // 6 : Area 1 On - T1_S1
  { 255, false, false, true, false, false }, // 7 : Area 2 On - T1_S1
  { 255, false, false, true, false, false }, // 8 : Area 3 On - T1_S1
  { 255, false, false, true, false, false }, // 9 : Area 4 On - T1_S1
  { 0, false, false, true, false, false }, // 10 : Area Stepping continue - T1234_CONT
  { 0, false, false, false, false, false }, // 11 : Decrement - DEC_S
  { 0, false, false, false, false, false }, // 12 : Increment - INC_S
  { 0, false, false, true, false, false }, // 13 : Minimum - MIN_S
  { 255, false, false, true, false, false }, // 14 : Maximum - MAX_S
  { 0, false, false, true, false, false }, // 15 : Stop - STOP_S
  { 0, false, false, false, true, false }, // 16 : Reserved
  { 192, false, false, false, false, false }, // 17 : Preset 2 - T0_S2
  { 128, false, false, false, false, false }, // 18 : Preset 3 - T0_S3
  { 64, false, false, false, false, false }, // 19 : Preset 4 - T0_S4
  { 192, false, false, false, false, false }, // 20 : Preset 12 - T1_S2
  { 128, false, false, false, false, false }, // 21 : Preset 13 - T1_S3
  { 64, false, false, false, false, false }, // 22 : Preset 14 - T1_S4
  { 192, false, false, false, false, false }, // 23 : Preset 22 - T2_S2
  { 168, false, false, false, false, false }, // 24 : Preset 23 - T2_S3
  { 64, false, false, false, false, false }, // 25 : Preset 24 - T2_S4
  { 192, false, false, false, false, false }, // 26 : Preset 32 - T3_S2
  { 168, false, false, false, false, false }, // 27 : Preset 33 - T3_S3
  { 64, false, false, false, false, false }, // 28 : Preset 34 - T3_S4
  { 192, false, false, false, false, false }, // 29 : Preset 42 - T4_S2
  { 168, false, false, false, false, false }, // 30 : Preset 43 - T4_S3
  { 64, false, false, false, false, false }, // 31 : Preset 44 - T4_S4
  { 0, false, false, false, false, false }, // 32 : Preset 10 - T1E_S0
  { 255, false, false, false, false, false }, // 33 : Preset 11 - T1E_S1
  { 0, false, false, false, false, false }, // 34 : Preset 20 - T2E_S0
  { 255, false, false, false, false, false }, // 35 : Preset 21 - T2E_S1
  { 0, false, false, false, false, false }, // 36 : Preset 30 - T3E_S0
  { 255, false, false, false, false, false }, // 37 : Preset 31 - T3E_S1
  { 0, false, false, false, false, false }, // 38 : Preset 40 - T4E_S0
  { 255, false, false, false, false, false }, // 39 : Preset 41 - T4E_S1
  { 0, false, false, false, true, false }, // 40 : Reserved
  { 0, false, false, false, true, false }, // 41 : Reserved
  { 0, false, false, true, false, false }, // 42 : Area 1 Decrement - T1_DEC
  { 0, false, false, true, false, false }, // 43 : Area 1 Increment - T1_INC 
  { 0, false, false, true, false, false }, // 44 : Area 2 Decrement - T2_DEC
  { 0, false, false, true, false, false }, // 45 : Area 2 Increment - T2_INC 
  { 0, false, false, true, false, false }, // 46 : Area 3 Decrement - T3_DEC
  { 0, false, false, true, false, false }, // 47 : Area 3 Increment - T3_INC
  { 0, false, false, true, false, false }, // 48 : Area 4 Decrement - T4_DEC
  { 0, false, false, true, false, false }, // 49 : Area 4 Increment - T4_INC 
  { 0, false, false, true, false, false }, // 50 : Device (Local Button) on : LOCAL_OFF
  { 255, false, false, true, false, false }, // 51 : Device (Local Button) on : LOCAL_ON
  { 0, false, false, true, false, false }, // 52 : Area 1 Stop - T1_STOP_S
  { 0, false, false, true, false, false }, // 53 : Area 2 Stop - T2_STOP_S
  { 0, false, false, true, false, false }, // 54 : Area 3 Stop - T3_STOP_S
  { 0, false, false, true, false, false }, // 55 : Area 4 Stop - T4_STOP_S
  { 0, false, false, false, true, false }, // 56 : Reserved
  { 0, false, false, false, true, false }, // 57 : Reserved
  { 0, false, false, false, true, false }, // 58 : Reserved
  { 0, false, false, false, true, false }, // 59 : Reserved
  { 0, false, false, false, true, false }, // 60 : Reserved
  { 0, false, false, false, true, false }, // 61 : Reserved
  { 0, false, false, false, true, false }, // 62 : Reserved
  { 0, false, false, false, true, false }, // 63 : Reserved
  // global, appartment-wide, group independent scenes
  { 0, true, false, true, false, false }, // 64 : Auto Standby - AUTO_STANDBY
  { 255, false, false, true, false, false }, // 65 : Panic - SIG_PANIC
  { 0, false, false, false, true, false }, // 66 : Reserved (ENERGY_OL)
  { 0, false, false, true, false, false }, // 67 : Standby - STANDBY
  { 0, false, false, true, false, false }, // 68 : Deep Off - DEEP_OFF
  { 0, false, false, true, false, false }, // 69 : Sleeping - SLEEPING
  { 255, false, false, true, true, false }, // 70 : Wakeup - WAKE_UP
  { 255, false, false, true, true, false }, // 71 : Present - PRESENT
  { 0, false, false, true, false, false }, // 72 : Absent - ABSENT
  { 0, false, false, true, true, false }, // 73 : Door Bell - SIG_BELL
  { 0, false, false, false, true, false }, // 74 : Reserved (SIG_ALARM)
  { 255, false, false, false, true, false }, // 75 : Zone Active
  { 255, false, false, false, true, false }, // 76 : Reserved
  { 255, false, false, false, true, false }, // 77 : Reserved
  { 0, false, false, false, true, false }, // 78 : Reserved
  { 0, false, false, false, true, false }, // 79 : Reserved
  // all other scenes equal or higher
  { 0, false, false, false, true, false }, // 80..n : Reserved
};



LightScene::LightScene(ParamStore &aParamStore, SceneNo aSceneNo) :
  inherited(aParamStore),
  sceneNo(aSceneNo)
{
  // fetch from defaults
  if (aSceneNo>NUMDEFAULTSCENES)
    aSceneNo = NUMDEFAULTSCENES; // last entry in the table is the default for all higher scene numbers
  const DefaultSceneParams &p = defaultScenes[aSceneNo];
  sceneValue = p.sceneValue;
  slowTransition = p.slowTransition;
  flashing = p.flashing;
  ignoreLocalPriority = p.ignoreLocalPriority;
  dontCare = p.dontCare;
  specialBehaviour = p.specialBehaviour;
}


// SQLIte3 table name to store these parameters to
const char *LightScene::tableName()
{
  return "dsLightScenes";
}


// primary key field definitions
const FieldDefinition *LightScene::getKeyDefs()
{
  static const FieldDefinition keyDefs[] = {
    { "parentID", SQLITE_TEXT }, // not included in loadFromRow() and bindToStatement()
    { "sceneNo", SQLITE_INTEGER }, // first field included in loadFromRow() and bindToStatement()
    { NULL, 0 },
  };
  return keyDefs;
}


/// data field definitions
const FieldDefinition *LightScene::getFieldDefs()
{
  static const FieldDefinition dataDefs[] = {
    { "sceneValue", SQLITE_INTEGER },
    { "sceneFlags", SQLITE_INTEGER },
    { NULL, 0 },
  };
  return dataDefs;
}


enum {
  LightSceneFlag_dontCare = 0x0001,
  LightSceneFlag_ignoreLocalPriority = 0x0002,
  LightSceneFlag_specialBehaviour = 0x0004,
  LightSceneFlag_flashing = 0x0008,
  LightSceneFlag_slowTransition = 0x0010,
};


/// load values from passed row
void LightScene::loadFromRow(sqlite3pp::query::iterator &aRow, int &aIndex)
{
  inherited::loadFromRow(aRow, aIndex);
  // get the fields
  sceneNo = aRow->get<int>(aIndex++);
  sceneValue = aRow->get<int>(aIndex++);
  int flags = aRow->get<int>(aIndex++);
  // decode the flags
  dontCare = flags & LightSceneFlag_dontCare;
  ignoreLocalPriority = flags & LightSceneFlag_ignoreLocalPriority;
  specialBehaviour = flags & LightSceneFlag_specialBehaviour;
  flashing = flags & LightSceneFlag_flashing;
  slowTransition = flags & LightSceneFlag_slowTransition;

}


/// bind values to passed statement
void LightScene::bindToStatement(sqlite3pp::statement &aStatement, int &aIndex, const char *aParentIdentifier)
{
  inherited::bindToStatement(aStatement, aIndex, aParentIdentifier);
  // encode the flags
  int flags = 0;
  if (dontCare) flags |= LightSceneFlag_dontCare;
  if (ignoreLocalPriority) flags |= LightSceneFlag_ignoreLocalPriority;
  if (specialBehaviour) flags |= LightSceneFlag_specialBehaviour;
  if (flashing) flags |= LightSceneFlag_flashing;
  if (slowTransition) flags |= LightSceneFlag_slowTransition;  
  // bind the fields
  aStatement.bind(aIndex++, sceneNo);
  aStatement.bind(aIndex++, sceneValue);
  aStatement.bind(aIndex++, flags);
}


#pragma mark - LightSettings


LightSettings::LightSettings(ParamStore &aParamStore) :
  inherited(aParamStore),
  isDimmable(false),
  onThreshold(128),
  minDim(1),
  maxDim(255)
{
  
}



LightScenePtr LightSettings::getScene(SceneNo aSceneNo)
{
  // check if we have modified from default data
  LightSceneMap::iterator pos = scenes.find(aSceneNo);
  if (pos!=scenes.end()) {
    // found scene params in map
    return pos->second;
  }
  else {
    // just return default values for this scene
    return LightScenePtr(new LightScene(paramStore, aSceneNo));
  }
}


void LightSettings::updateScene(LightScenePtr aScene)
{
  if (aScene->rowid==0) {
    // unstored so far, add to map of non-default scenes
    scenes[aScene->sceneNo] = aScene;
  }
  // anyway, mark dirty
  aScene->markDirty();
}



// SQLIte3 table name to store these parameters to
const char *LightSettings::tableName()
{
  return "dsLight";
}

/// data field definitions
const FieldDefinition *LightSettings::getFieldDefs()
{
  static const FieldDefinition dataDefs[] = {
    { "isDimmable", SQLITE_INTEGER },
    { "onThreshold", SQLITE_INTEGER },
    { "minDim", SQLITE_INTEGER },
    { "maxDim", SQLITE_INTEGER },
    { NULL, 0 },
  };
  return dataDefs;
}


/// load values from passed row
void LightSettings::loadFromRow(sqlite3pp::query::iterator &aRow, int &aIndex)
{
  inherited::loadFromRow(aRow, aIndex);
  // get the fields
  isDimmable = aRow->get<bool>(aIndex++);
  onThreshold = aRow->get<int>(aIndex++);
  minDim = aRow->get<int>(aIndex++);
  maxDim = aRow->get<int>(aIndex++);
}


// bind values to passed statement
void LightSettings::bindToStatement(sqlite3pp::statement &aStatement, int &aIndex, const char *aParentIdentifier)
{
  inherited::bindToStatement(aStatement, aIndex, aParentIdentifier);
  // bind the fields
  aStatement.bind(aIndex++, isDimmable);
  aStatement.bind(aIndex++, onThreshold);
  aStatement.bind(aIndex++, minDim);
  aStatement.bind(aIndex++, maxDim);
}


// load child parameters (scenes)
ErrorPtr LightSettings::loadChildren()
{
  ErrorPtr err;
  // my own ROWID is the parent key for the children
  string parentID = string_format("%d",rowid);
  // create a template
  LightScenePtr scene = LightScenePtr(new LightScene(paramStore, 0));
  // get the query
  sqlite3pp::query * queryP = scene->newLoadAllQuery(parentID.c_str());
  if (queryP==NULL) {
    // real error preparing query
    err = paramStore.error();
  }
  else {
    for (sqlite3pp::query::iterator row = queryP->begin(); row!=queryP->end(); ++row) {
      // got record
      // - load record fields into scene object
      int index = 0;
      scene->loadFromRow(row, index);
      // - put scene into map of non-default scenes
      scenes[scene->sceneNo] = scene;
      // - fresh object for next row
      scene = LightScenePtr(new LightScene(paramStore, 0));
    }
  }
  if (Error::isOK(err)) {
    err = loadChildren();
  }
  return err;

}

// save child parameters (scenes)
ErrorPtr LightSettings::saveChildren()
{
  ErrorPtr err;
  // my own ROWID is the parent key for the children
  string parentID = string_format("%d",rowid);
  // save all elements of the map (only dirty ones will be actually stored to DB
  for (LightSceneMap::iterator pos = scenes.begin(); pos!=scenes.end(); ++pos) {
    err = pos->second->saveToStore(parentID.c_str());
  }
  return err;
}








