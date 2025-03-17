constexpr const char *ROOT_CONFIG_DIRECTORY = "/syscon";
const char *DEF_CONFIG_DIRECTORY = "def";
const char *SELECTION_FILENAME = "selection.json";
const char *SW_TOPICS_FILENAME = "sw_topics.json";
const char *ACTIVITY_FILENAME = "activity.json";
const char *SW_PARAMETERS_FILENAME = "sw_properies.json";

uint8_t readParameters_hardCoded(JsonDocument &DOC)
{
  constexpr char *params = "{ \"numSW\": 1,\
                          \"inputType\":[1],\
                          \"inputPins\":[5],\
                          \"outputPins\":[0],\
                          \"indicPins\":[255],\
                          \"swTimeout\":[0],\
                          \"swName\":[\"sw0\"],\
                          \"lockdown\":[false],\
                          \"pwm_intense\":[0],\
                          \"outputON\":[1],\
                          \"inputPressed\":[0],\
                          \"onBoot\":[0],\
                          \"timeFactor\": [60000]}";
  DeserializationError err = deserializeJson(DOC, params);
  return err.code();
}
uint8_t readTopics_hardCoded(JsonDocument &DOC)
{
  constexpr const char *params = "{ \"gen_pubTopic\":[\"DvirHome/Messages\",\"DvirHome/log\",\"DvirHome/debug\"],\
                          \"subTopic\":[\"DvirHome/light_CODE\",\"DvirHome/All\"],\
                          \"pubTopic\":[\"DvirHome/light_CODE/Avail\",\"DvirHome/light_CODE/State\"]}";
  DeserializationError err = deserializeJson(DOC, params);
  return err.code();
}

bool direxsits(const char *dir)
{
  LittleFS.begin();
  return LittleFS.exists(dir);
}
void get_directory_list(char dirlist[])
{
  LittleFS.begin();
  Dir dir = LittleFS.openDir(ROOT_CONFIG_DIRECTORY);
  strcpy(dirlist, "");
  while (dir.next())
  {
    strcat(dirlist, dir.fileName().c_str());
    strcat(dirlist, "; ");
  }
}
bool find_directory(const char *d)
{
  LittleFS.begin();
  Dir dir = LittleFS.openDir(ROOT_CONFIG_DIRECTORY);
  while (dir.next())
  {
    if (strcmp(d, dir.fileName().c_str()) == 0)
    {
      return true;
    }
  }
  return false;
}
bool getConfig_directory(char dirpath[])
{
  DynamicJsonDocument DOC(50);
  if (iot.readJson_inFlash(DOC, SELECTION_FILENAME)) // read directory from file in flash
  {
    sprintf(dirpath, "%s/%s", ROOT_CONFIG_DIRECTORY, DOC["config"].as<const char *>());
    return true;
  }
  else // else goes to default directory in flash
  {
    sprintf(dirpath, "%s/%s", ROOT_CONFIG_DIRECTORY, DEF_CONFIG_DIRECTORY);
    return false;
  }
}
bool build_filename_path(JsonDocument &DOC, char filename[], const char *File)
{
  if (getConfig_directory(filename))
  {
    strcat(filename, "/");
    strcat(filename, File);
    if (veboseMode)
    {
      Serial.print(">> Filename constructed: ");
      Serial.println(filename);
      Serial.flush();
    }
  }
  if (!direxsits(filename)) // if file in desired directory not found
  {
    sprintf(filename, "%s/%s/%s", ROOT_CONFIG_DIRECTORY, DEF_CONFIG_DIRECTORY, File); // default directory with asked file
    if (veboseMode)
    {
      Serial.println(">> Filename construct failed. Default oath used.");
      Serial.flush();
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
  if (find_directory(configFile))
  {
    DOC["config"] = configFile;
    serializeJsonPretty(DOC, Serial);
    if (ConfigJsonFile.writeFile(DOC, SELECTION_FILENAME))
    {
      iot.pub_log("Config file updated");
      return true;
    }
    else
    {
      iot.pub_log("Directory Exists. Config file failed updating");
      return false;
    }
  }
  else
  {
    iot.pub_log("Directory not Exists. Config file failed updating");

    return false;
  }
}
bool get_sw_defs(JsonDocument &DOC)
{
  if (READ_PARAMTERS_FROM_FLASH)
  {
    Serial.println(">> Flash parameters");
    char file[30];
    if (build_filename_path(DOC, file, SW_PARAMETERS_FILENAME)) // able to construct file path
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
bool readTopics_defs(JsonDocument &DOC)
{
  if (READ_PARAMTERS_FROM_FLASH)
  {
    char file[30];
    build_filename_path(DOC, file, SW_TOPICS_FILENAME);
    return iot.readJson_inFlash(DOC, file); // succeed to read JSON from flash
  }
  else
  {
    return readTopics_hardCoded(DOC) == 0; // succeed to read JSON from hardcoded backup
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
  return ActionSave.readFile(DOC, ACTIVITY_FILENAME);
}
bool savedLastAction_file(uint8_t i)
{
  myJflash ActionSave;
  DynamicJsonDocument DOC(ACT_JSON_DOC_SIZE);

  readLastAction_file(DOC);
  Telemtry2JSON(DOC, i);
  return ActionSave.writeFile(DOC, ACTIVITY_FILENAME);
}