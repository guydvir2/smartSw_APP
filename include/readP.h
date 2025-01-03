const char *dir1 = "/syscon";
const char *def_config_dir = "def";
const char *selection_filename = "selection.json";
const char *swTopics_filename = "sw_topics.json";
const char *savedActivity_filename = "activity.json";
const char *swParameters_filename = "sw_properies.json";

uint8_t readParameters_hardCoded(JsonDocument &DOC)
{
  const char *params = "{ \"numSW\": 1,\
                          \"inputType\":[1],\
                          \"virtCMD\":[0],\
                          \"inputPins\":[5],\
                          \"outputPins\":[0],\
                          \"indicPins\":[255],\
                          \"swTimeout\":[0],\
                          \"swName\":[\"sw0\"],\
                          \"lockdown\":[false],\
                          \"pwm_intense\":[0],\
                          \"outputON\":[1],\
                          \"inputPressed\":[0],\
                          \"onBoot\":[0]}";
  DeserializationError err = deserializeJson(DOC, params);
  return err.code();
}
uint8_t readTopics_hardCoded(JsonDocument &DOC)
{
  const char *params = "{ \"gen_pubTopic\":[\"DvirHome/Messages\",\"DvirHome/log\",\"DvirHome/debug\"],\
                          \"subTopic\":[\"DvirHome/light_CODE\"],\
                          \"availTopic\":[\"DvirHome/light_CODE/Avail\"],\
                          \"stateTopic\":[\"DvirHome/light_CODE/State\"]}";
  DeserializationError err = deserializeJson(DOC, params);
  return err.code();
}

bool direxsits(const char *dir)
{
  LittleFS.begin();
  return LittleFS.exists(dir);
}
void read_dirList(char dirlist[])
{
  LittleFS.begin();
  Dir dir = LittleFS.openDir(dir1);
  strcpy(dirlist, "");
  while (dir.next())
  {
    strcat(dirlist, dir.fileName().c_str());
    strcat(dirlist, "; ");
  }
}
bool find_config_dir(const char *d)
{
  LittleFS.begin();
  Dir dir = LittleFS.openDir(dir1);
  while (dir.next())
  {
    if (strcmp(d, dir.fileName().c_str()) == 0)
    {
      return true;
    }
  }
  return false;
}
bool construct_directory(char dirpath[])
{
  DynamicJsonDocument DOC(50);
  if (iot.readJson_inFlash(DOC, selection_filename)) // read directory from file in flash
  {
    sprintf(dirpath, "%s/%s", dir1, DOC["config"].as<const char *>());
    return true;
  }
  else // else goes to default directory in flash
  {
    sprintf(dirpath, "%s/%s", dir1, def_config_dir);
    return false;
  }
}
bool construct_filename(JsonDocument &DOC, char filename[], const char *File)
{
  if (construct_directory(filename))
  {
    strcat(filename, "/");
    strcat(filename, File);
    if (veboseMode)
    {
      Serial.println(filename);
    }
  }
  if (!direxsits(filename)) // if file in desired directory not found
  {
    sprintf(filename, "%s/%s/%s", dir1, def_config_dir, File); // default directory with asked file
    if (veboseMode)
    {
      Serial.println(filename);
      Serial.println(">> Filename construct failed. Default oath used.");
    }
    return false;
  }
  else
  {
    return true;
  }
}
bool update_config_dir(const char *configFile)
{
  DynamicJsonDocument DOC(150);
  myJflash ConfigJsonFile(iot.useSerial);
  DOC["config"] = configFile;
  serializeJsonPretty(DOC, Serial);
  return ConfigJsonFile.writeFile(DOC, selection_filename);
}
bool select_SWdefinition_src(JsonDocument &DOC)
{
  if (READ_PARAMTERS_FROM_FLASH)
  {
    Serial.println(">> Flash parameters");
    char file[30];
    if (construct_filename(DOC, file, swParameters_filename)) // able to construct file path
    {
      Serial.println("construct OK.");
      return iot.readJson_inFlash(DOC, file); // succeed to read file
    }
    else
    {
      Serial.println("construct failed.");
      return false;
    }
  }
  else
  {
    Serial.println("HardCoded");
    return readParameters_hardCoded(DOC) == 0;
  }
}
bool select_Topicsdefinition_src(JsonDocument &DOC)
{
  if (READ_PARAMTERS_FROM_FLASH)
  {
    char file[30];
    construct_filename(DOC, file, swTopics_filename);
    return iot.readJson_inFlash(DOC, file);
  }
  else
  {
    return readTopics_hardCoded(DOC) == 0;
  }
}
void Telemtry2JSON(JsonDocument &DOC, uint8_t i)
{
  DOC["newMSG"][i] = false;
  DOC["lockdown"][i] = SW_Array[i]->telemtryMSG.lockdown;
  DOC["pwm"][i] = SW_Array[i]->telemtryMSG.pwm;
  DOC["state"][i] = SW_Array[i]->telemtryMSG.state;
  DOC["reason"][i] = SW_Array[i]->telemtryMSG.reason;
  DOC["pressCount"][i] = SW_Array[i]->telemtryMSG.pressCount;
  DOC["clk_end"][i] = SW_Array[i]->telemtryMSG.clk_end;

  if (SW_Array[i]->telemtryMSG.state) /* if on save clk else store 0 */
  {
    DOC["clk_start"][i] = iot.now();
  }
  else
  {
    DOC["clk_start"][i] = 0;
  }

  DOC["indic_state"][i] = SW_Array[i]->telemtryMSG.indic_state;
}
bool readLastAction_file(JsonDocument &DOC)
{
  myJflash ActionSave;
  return ActionSave.readFile(DOC, savedActivity_filename);
}
bool savedLastAction_file(uint8_t i)
{
  myJflash ActionSave;
  DynamicJsonDocument DOC(ACT_JSON_DOC_SIZE);

  readLastAction_file(DOC);
  Telemtry2JSON(DOC, i);
  return ActionSave.writeFile(DOC, savedActivity_filename);
}