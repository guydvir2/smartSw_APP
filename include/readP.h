const char *paramterFiles[] = {"/iot_config.json"};
const char *swTopics_filename = "/sw_topics.json";
const char *savedActivity_filename = "/activity.json";
const char *swParameters_filename = "/sw_properies.json";

uint8_t readParameters_hardCoded(JsonDocument &DOC)
{
  const char *params = "{ \"numSW\": 1,\
                          \"inputType\":[1],\
                          \"virtCMD\":[0],\
                          \"inputPins\":[0],\
                          \"outputPins\":[5],\
                          \"indicPins\":[2],\
                          \"swTimeout\":[10],\
                          \"swName\":[\"sw0\"],\
                          \"lockdown\":[false],\
                          \"pwm_intense\":[0],\
                          \"outputON\":[1],\
                          \"indicON\":[0],\
                          \"inputPressed\":[0],\
                          \"onBoot\":[1]}";
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
    serializeJsonPretty(DOC,Serial);
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
bool select_SWdefinition_src(JsonDocument &DOC, const char *file)
{
  if (READ_PARAMTERS_FROM_FLASH)
  {
    return iot.readJson_inFlash(DOC, file);
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

  if (SW_Array[i]->telemtryMSG.state) /* if ON, save clk else store 0 */
  {
    DOC["clk_start"][i] = iot.now();
  }
  else
  {
    DOC["clk_start"][i] = 0;
  }

  DOC["indic_state"][i] = SW_Array[i]->telemtryMSG.indic_state;
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