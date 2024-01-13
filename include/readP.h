const char *swTopics_filename = "/sw_topics.json";
const char *savedActivity_filename = "/activity.json";
const char *swParameters_filename = "/sw_properies.json";

extern void start_iot2(JsonDocument &DOC);
extern void build_SWdefinitions(JsonDocument &DOC);

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
bool select_Topicsdefinition_src(JsonDocument &DOC, const char *file)
{
  if (READ_PARAMTERS_FROM_FLASH)
  {
    return iot.readJson_inFlash(DOC, file);
  }
  else
  {
    return readTopics_hardCoded(DOC) == 0;
  }
}
void init_SW(JsonDocument &DOC)
{
  if (select_SWdefinition_src(DOC, swParameters_filename)) /* Stored in flash or hard-coded */
  {
    build_SWdefinitions(DOC);
    bootSucceeded = true;
  }
}
void init_iot2(JsonDocument &DOC)
{
  select_Topicsdefinition_src(DOC, swTopics_filename); /* Stored in flash or hard-coded */
  build_SWdefinitions(DOC);
  start_iot2(DOC); /* iot2 should start always, regardless success of SW */
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