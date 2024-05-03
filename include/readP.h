const char *savedActivity_filename = "/activity.json";
const char *paramterFiles[] = {"/iot_config.json", "/sw_topics.json", "/sw_properies.json", "/vCMDs.json"};
struct VirtTopic
{
  uint8_t SWn = 0;
  uint8_t CMDn = 0;
  const char *topic{};
  const char *cmd{};
};

uint8_t readParameters_hardCoded(JsonDocument &DOC)
{
  const char *params = "{ \"numSW\": 4,\
                          \"inputType\":[1,1,2,0],\
                          \"inputPins\":[0,5,4,2],\
                          \"outputPins\":[255,14,12,13],\
                          \"indicPins\":[255,15,255,255],\
                          \"swTimeout\":[0,10,1,0],\
                          \"swName\":[\"Vsw0\",\"sw1\",\"sw2\",\"sw3\"],\
                          \"lockdown\":[false,false,false,false],\
                          \"pwm_intense\":[0,90,50,0],\
                          \"outputON\":[1,1,1,1],\
                          \"indicON\":[0,0,0,0],\
                          \"inputPressed\":[0,0,0,0],\
                          \"onBoot\":[0,1,0,0]}";
  DeserializationError err = deserializeJson(DOC, params);
  return err.code();
}
uint8_t readTopics_hardCoded(JsonDocument &DOC)
{
  const char *params = "{ \"gen_pubTopic\":[\"DvirHome/Messages\",\"DvirHome/log\",\"DvirHome/debug\"],\
                          \"subTopic\":[\"DvirHome/Device_2\",\"DvirHome/All\"],\
                          \"pubTopic\":[\"DvirHome/Device_2/Avail\",\"DvirHome/Device_2/State\"]}";
  DeserializationError err = deserializeJson(DOC, params);
  return err.code();
}

void getVirtCMD(JsonDocument &DOC, const char *filename, VirtTopic &vTopic)
{
  if (iot.readJson_inFlash(DOC, filename))
  {
    vTopic.topic = DOC["topics"][vTopic.SWn];
    vTopic.cmd = DOC["cmds"][vTopic.SWn][vTopic.CMDn];
  }
  else
  {
    vTopic.topic = "MyHome";
    vTopic.cmd = "Err";
  }
}
void readTopics_flash(JsonDocument &DOC, const char *filename)
{
  bool a = iot.readJson_inFlash(DOC, filename);
  bool b = DOC.containsKey("gen_pubTopic") && DOC.containsKey("subTopic") && DOC.containsKey("pubTopic");

  if (!a && !b) /* case of error */
  {
    const char *topics = "{ \"gen_pubTopic\":[\"DvirHome/Messages\",\"DvirHome/log\",\"DvirHome/debug\"],\
                            \"subTopic\":[\"DvirHome/Device_1\",\"DvirHome/All\"],\
                            \"pubTopic\":[\"DvirHome/Device_1/Avail\",\"DvirHome/Device_1/State\"]}";
    deserializeJson(DOC, topics);
  }
}
void update_topics_iot(JsonDocument &DOC, const char *file)
{
  if (READ_PARAMTERS_FROM_FLASH)
  {
    readTopics_flash(DOC, file);
  }
  else
  {
    readTopics_hardCoded(DOC);
  }

  for (uint8_t t = 0; t < DOC["gen_pubTopic"].size(); t++)
  {
    iot.add_gen_pubTopic(DOC["gen_pubTopic"][t]);
  }
  for (uint8_t t = 0; t < DOC["subTopic"].size(); t++)
  {
    iot.add_subTopic(DOC["subTopic"][t]);
  }
  for (uint8_t t = 0; t < DOC["pubTopic"].size(); t++)
  {
    iot.add_pubTopic(DOC["pubTopic"][t]);
  }
}
bool read_SW_Topics(JsonDocument &DOC, const char *file)
{
  if (READ_PARAMTERS_FROM_FLASH)
  {
    if (!iot.readJson_inFlash(DOC, file))
    {
      return readParameters_hardCoded(DOC) == 0;
    }
    else
    {
      return 1;
    }
  }
  else
  {
    return readParameters_hardCoded(DOC) == 0;
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
  DOC["indic_state"][i] = SW_Array[i]->telemtryMSG.indic_state;
  SW_Array[i]->telemtryMSG.state ? DOC["clk_start"][i] = iot.now() : DOC["clk_start"][i] = 0; /* if ON, save clk else store 0 */
}

bool readActivity_file(JsonDocument &DOC)
{
  myJflash ActionSave;
  return ActionSave.readFile(DOC, savedActivity_filename);
}
bool saveActivity_file(uint8_t i)
{
  myJflash ActionSave;
  DynamicJsonDocument DOC(ACT_JSON_DOC_SIZE);
  readActivity_file(DOC);
  Telemtry2JSON(DOC, i);
  return ActionSave.writeFile(DOC, savedActivity_filename);
}