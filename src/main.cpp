#include <Arduino.h>
#include <myIOT2.h>
#include <smartSwitch.h>

#define NUM_SW 4
#define JSON_DOC_SIZE 1200
#define ACT_JSON_DOC_SIZE 800
#define READ_PARAMTERS_FROM_FLASH true /* Flash or HardCoded Parameters */
#define veboseMode true

myIOT2 iot;
smartSwitch *SW_Array[NUM_SW]{};
const char *verApp = "smartSWApp_v0.2";

uint8_t SW_inUse = 0;
bool firstLoop = true;
bool bootSucceeded = false;

#include "readP.h"

void getPWMval(uint8_t i, char msg[])
{
  SW_props sw_properties;
  SW_Array[i]->get_SW_props(sw_properties);
  if (sw_properties.PWM_intense && SW_Array[i]->get_SWstate())
  {
    sprintf(msg, ", Power: [%d%%]", SW_Array[i]->telemtryMSG.pwm);
  }
  else if (sw_properties.PWM_intense && !SW_Array[i]->get_SWstate())
  {
    sprintf(msg, ", Power: [%d%%]", sw_properties.PWM_intense);
  }
  else
  {
    strcpy(msg, "");
  }
}
void createTelemetry_post(uint8_t i)
{
  char clk[25];
  char msg[300];
  char topic[50];
  iot.get_timeStamp(clk);

  sprintf(topic, "%s/SW%d/tele", iot.topics_sub[0], i);
  sprintf(msg, "{\"timeStamp\":%s, \"newMSG\":%s, \"lockdown\":%s, \"indic_state\":%s, \"pwm\":%d, \"state\":%d, \"reason\":%d, \"pressCount\":%d, \"clk_end\":%ld, \"clk_start\":%ld}",
          clk,
          SW_Array[i]->telemtryMSG.newMSG ? "true" : "false", SW_Array[i]->telemtryMSG.lockdown ? "true" : "false",
          SW_Array[i]->telemtryMSG.indic_state ? "true" : "false", SW_Array[i]->telemtryMSG.pwm,
          SW_Array[i]->telemtryMSG.state, SW_Array[i]->telemtryMSG.reason, SW_Array[i]->telemtryMSG.pressCount,
          SW_Array[i]->telemtryMSG.clk_end, SW_Array[i]->telemtryMSG.clk_start);

  iot.pub_noTopic(msg, topic, true);
}
void createEntity_post(uint8_t i)
{
  char clk[25];
  char msg[300];
  char topic[50];
  iot.get_timeStamp(clk);

  sprintf(topic, "%s/SW%d/entity", iot.topics_sub[0], i);
  const char *swTypes[] = {"None", "Button", "Switch", "MultiPress"};

  SW_props sw_properties;
  SW_Array[i]->get_SW_props(sw_properties);
  sprintf(msg, "{\"id\":%d, \"name\":%s, \"button_type\":%s, \"PWM_intense\":%d, \"lockdown\":%s, \"timeout\":%s, \"Duration\":%d, \"inputPin\":%d, \"outPin\":%d, \"indicationPin\":%d, \"virtual\":%s}",
          sw_properties.id,
          sw_properties.name,
          swTypes[sw_properties.type],
          sw_properties.PWM_intense,
          sw_properties.lockdown ? "yes" : "no",
          sw_properties.timeout ? "yes" : "no",
          sw_properties.TO_dur,
          sw_properties.inpin,
          sw_properties.outpin,
          sw_properties.indicpin,
          sw_properties.virtCMD ? "yes" : "no");

  iot.pub_noTopic(msg, topic, true);
}

void extMQTT(char *incoming_msg, char *_topic)
{
  char msg[270];
  if (strcmp(incoming_msg, "status") == 0)
  {
    for (uint8_t i = 0; i < SW_inUse; i++)
    {
      char msg2[160];
      SW_props sw_properties;
      SW_Array[i]->get_SW_props(sw_properties);

      char sss[20];
      char clk[20];
      char clk2[20];
      char clk3[20];
      char clk4[20];

      getPWMval(i, sss);
      iot.convert_epoch2clock(SW_Array[i]->get_timeout() / 1000, 0, clk3);
      const char *trigs[] = {"Button", "Timeout", "MQTT", "atBoot", "Resume"};

      if (SW_Array[i]->get_SWstate())
      {
        if (SW_Array[i]->useTimeout())
        {
          iot.convert_epoch2clock(SW_Array[i]->get_remain_time() / 1000, 0, clk);
          iot.convert_epoch2clock(SW_Array[i]->get_elapsed() / 1000, 0, clk2);
          sprintf(msg2, "timeout: [%s], elapsed: [%s], remain: [%s], triggered: [%s]", clk3, clk2, clk, trigs[SW_Array[i]->telemtryMSG.reason]);
        }
        else
        {
          iot.convert_epoch2clock((millis() - SW_Array[i]->telemtryMSG.clk_start) / 1000, 0, clk4);
          sprintf(msg2, "timeout: [%s], elapsed: [%s], remain: [%s], triggered: [%s]", "NA", clk4, "NA", trigs[SW_Array[i]->telemtryMSG.reason]);
        }
      }
      else
      {
        if (SW_Array[i]->useTimeout())
        {
          sprintf(msg2, "Timeout: [%s]", clk3);
        }
        else
        {
          sprintf(msg2, "Timeout: [%s]", "NA");
        }
      }

      sprintf(msg, "[Status]: [SW#%d] name: [%s] state: [%s] %s%s",
              i, SW_Array[i]->name, SW_Array[i]->get_SWstate() ? "On" : "Off", msg2, sss);
      iot.pub_msg(msg);
    }
  }
  else if (strcmp(incoming_msg, "help2") == 0)
  {
    sprintf(msg, "help #2:{[i],on,[timeout],[pwm_percentage]},{[i],off}, {[i], add_time,[timeout]}, {[i], remain}, {[i], timeout}, {[i], elapsed}, {[i], show_params}");
    iot.pub_msg(msg);
  }
  else if (strcmp(incoming_msg, "ver2") == 0)
  {
    sprintf(msg, "ver #2: %s %s", verApp, SW_Array[0]->ver);
    iot.pub_msg(msg);
  }
  else if (strcmp(incoming_msg, "show_params") == 0)
  {
    DynamicJsonDocument DOC(JSON_DOC_SIZE);

    if (select_SWdefinition_src(DOC, swParameters_filename))
    {
      char clk[25];
      char msg2[300];

      const int mqtt_size = iot.mqtt_len - 30;
      iot.get_timeStamp(clk, iot.now());
      sprintf(msg2, "\n~~~~~~ %s %s ~~~~~~", iot.topics_sub[0], clk);
      iot.pub_debug(msg2);

      serializeJson(DOC, msg2);
      uint8_t w = strlen(msg2) / mqtt_size;

      for (uint8_t i = 0; i <= w; i++)
      {
        int x = 0;
        char msg3[mqtt_size + 5];
        for (int n = i * mqtt_size; n < i * mqtt_size + mqtt_size; n++)
        {
          msg3[x++] = msg2[n];
        }
        msg3[x] = '\0';
        iot.pub_debug(msg3);
      }
      iot.pub_msg("[Paramters]: Published in debug");
    }
  }
  else
  {
    if (iot.num_p > 1)
    {
      char clk[25];
      SW_props sw_properties;
      SW_Array[atoi(iot.inline_param[0])]->get_SW_props(sw_properties);

      if (strcmp(iot.inline_param[1], "on") == 0)
      {
        if (iot.num_p > 3)
        { /* PWM */
          SW_Array[atoi(iot.inline_param[0])]->turnON_cb(EXT_0, atoi(iot.inline_param[2]), atoi(iot.inline_param[3]));
        }
        else if (iot.num_p > 2)
        { /* NO -PWM with timeout */
          SW_Array[atoi(iot.inline_param[0])]->turnON_cb(EXT_0, atoi(iot.inline_param[2]));
        }
        else if (iot.num_p > 1)
        { /* NO -PWM with timeout */
          SW_Array[atoi(iot.inline_param[0])]->turnON_cb(EXT_0);
        }
      }
      else if (strcmp(iot.inline_param[1], "off") == 0)
      {
        SW_Array[atoi(iot.inline_param[0])]->turnOFF_cb(EXT_0);
      }
      else if (strcmp(iot.inline_param[1], "remain") == 0)
      {
        iot.convert_epoch2clock(SW_Array[atoi(iot.inline_param[0])]->get_remain_time() / 1000, 0, clk);
        sprintf(msg, "[Timeout]: [%s] remain: [%s]", sw_properties.name, clk);
        iot.pub_msg(msg);
      }
      else if (strcmp(iot.inline_param[1], "elapsed") == 0)
      {
        iot.convert_epoch2clock(SW_Array[atoi(iot.inline_param[0])]->get_elapsed() / 1000, 0, clk);
        sprintf(msg, "[Timeout]: [%s] elapsed: [%s]", sw_properties.name, clk);
        iot.pub_msg(msg);
      }
      else if (strcmp(iot.inline_param[1], "timeout") == 0)
      {
        iot.convert_epoch2clock(SW_Array[atoi(iot.inline_param[0])]->get_timeout() / 1000, 0, clk);
        sprintf(msg, "[Timeout]: [%s] Current: [%s]", sw_properties.name, clk);
        iot.pub_msg(msg);
      }
      else if (strcmp(iot.inline_param[1], "add_time") == 0)
      {
        if (SW_Array[atoi(iot.inline_param[0])]->useTimeout())
        {
          SW_Array[atoi(iot.inline_param[0])]->set_additional_timeout(atoi(iot.inline_param[2]), EXT_0);
          iot.convert_epoch2clock(atoi(iot.inline_param[2]) * TimeFactor / 1000, 0, clk);
          sprintf(msg, "[Timeout]: [%s] add time: [%s]", sw_properties.name, clk);
        }
        else
        {
          sprintf(msg, "[Timeout]: [SW#%d] does not use timeout", atoi(iot.inline_param[0]));
        }
        iot.pub_msg(msg);
      }
      else if (strcmp(iot.inline_param[1], "pow") == 0)
      {
        if (SW_Array[atoi(iot.inline_param[0])]->telemtryMSG.pwm < 102)
        {
          // SW_Array[atoi(iot.inline_param[0])]->set_additional_timeout(atoi(iot.inline_param[2]), EXT_0);
          // iot.convert_epoch2clock(atoi(iot.inline_param[2]) * TimeFactor / 1000, 0, clk);
          // sprintf(msg, "[Timeout]: [%s] add time: [%s]", sw_properties.name, clk);
        }
        else
        {
          sprintf(msg, "[Timeout]: [SW#%d] does not use PWM", atoi(iot.inline_param[0]));
        }
        iot.pub_msg(msg);
      }
    }
  }
}
void start_iot2(JsonDocument &DOC)
{
  iot.useSerial = true;
  iot.useFlashP = false;
  iot.noNetwork_reset = 2;
  iot.ignore_boot_msg = false;

  // const char *gtp1 = DOC["gen_pubTopic"][0] | "DvirHome/Messages";
  // const char *gtp2 = DOC["gen_pubTopic"][1] | "DvirHome/log";
  // const char *gtp3 = DOC["gen_pubTopic"][2] | "DvirHome/debug";
  // const char *st = DOC["subTopic"][0] | "DvirHome/Device";
  // const char *st2 = DOC["subTopic"][1] | "DvirHome/All";
  // const char *at = DOC["availTopic"][0] | "DvirHome/Device/Avail";
  // const char *stt = DOC["stateTopic"][0] | "DvirHome/Device/State";
  // iot.add_gen_pubTopic(gtp1);                            /* Messages */
  // iot.add_gen_pubTopic(gtp2);                            /* log */
  // iot.add_gen_pubTopic(gtp3);                            /* debug */
  // iot.add_subTopic(st);                                  /* Device's Topic */
  // iot.add_subTopic(DOC["subTopic"][1] | "DvirHome/All"); /* Device's Topic */
  // iot.add_pubTopic(at);                                  /* Avail */
  // iot.add_pubTopic(stt);

  /* Default values */
  const char *t[] = {"DvirHome/Messages", "DvirHome/log", "DvirHome/debug"};
  const char *t2[] = {"DvirHome/Device", "DvirHome/All"};
  const char *t3[] = {"DvirHome/Device/Avail", "DvirHome/Device/State"};
  /* Default values*/

  for (uint8_t i = 0; i < (DOC["gen_pubTopic"].size() | 3); i++)
  {
    iot.add_gen_pubTopic(DOC["gen_pubTopic"][i] | t[i]);
  }
  for (uint8_t i = 0; i < (DOC["subTopic"].size() | 2); i++)
  {
    iot.add_subTopic(DOC["subTopic"][i] | t2[i]);
  }
  for (uint8_t i = 0; i < (DOC["pubTopic"].size() | 3); i++)
  {
    iot.add_pubTopic(DOC["pubTopic"][i] | t3[i]);
  }

  iot.start_services(extMQTT);
}

void createSW(SW_props &sw)
{
  SW_Array[sw.id] = new smartSwitch(veboseMode);

  SW_Array[sw.id]->set_id(sw.id);                                                 /* Instances counter- generally don't need to interfere */
  SW_Array[sw.id]->set_timeout(sw.TO_dur);                                        /* timeout is optional */
  SW_Array[sw.id]->set_name(sw.name);                                             /* Optional */
  SW_Array[sw.id]->set_input(sw.inpin, sw.type, sw.inputPressed);                 /* 2-Toggle; 1-Button */
  SW_Array[sw.id]->set_output(sw.outpin, sw.PWM_intense, sw.outputON, sw.onBoot); /* Can be Relay, PWM, or Virual */
  SW_Array[sw.id]->set_indiction(sw.indicpin, sw.outputON);                       /* Optional */
  SW_Array[sw.id]->set_useLockdown(sw.lockdown);                                  /* Optional */

  SW_Array[sw.id]->print_preferences();
  SW_inUse++;
}
void build_SWdefinitions(JsonDocument &DOC)
{
  uint8_t numSW = DOC["numSW"] | 0;
  uint8_t m = numSW < NUM_SW ? numSW : NUM_SW;

  for (uint8_t n = 0; n < m; n++)
  {
    SW_props sw;
    sw.id = n;
    sw.type = DOC["inputType"][n] | 0;
    sw.inpin = DOC["inputPins"][n] | 0;
    sw.outpin = DOC["outputPins"][n] | 0;
    sw.indicpin = DOC["indicPins"][n] | 0;
    sw.TO_dur = DOC["swTimeout"][n] | 0;

    sw.name = DOC["swName"][n] | "NO_NAME";
    sw.lockdown = DOC["lockdown"][n] | 0;

    sw.PWM_intense = DOC["pwm_intense"][n] | 0;
    sw.virtCMD = DOC["virtCMD"][n] | 0;
    sw.timeout = sw.TO_dur > 0;
    sw.outputON = DOC["outputON"][n] | 0;
    sw.inputPressed = DOC["inputPressed"][n] | 0;
    sw.onBoot = DOC["onBoot"][n] | 0;
    createSW(sw);
  }
}

void restoreSaved_SwState_afterReboot()
{
  DynamicJsonDocument DOC(ACT_JSON_DOC_SIZE);

  if (readActivity_file(DOC))
  {
    for (uint8_t i = 0; i < SW_inUse; i++)
    {
      if ((DOC["state"][i].as<uint8_t>() | SW_OFF) == SW_ON && (DOC["lockdown"][i].as<bool>() | false) == false)
      {
        unsigned long _t = DOC["clk_start"][i].as<unsigned long>() + DOC["clk_end"][i].as<unsigned long>() * 0.001;
        if (DOC["clk_end"][i].as<unsigned long>() == 0)
        {
          SW_Array[i]->turnON_cb(EXT_2);
        }
        else if (_t > iot.now())
        {
          SW_Array[i]->turnON_cb(EXT_2, _t - iot.now(), DOC["pwm"][i].as<uint8_t>());
        }
      }
    }
  }
}
void On_atBoot()
{
  for (uint8_t i = 0; i < SW_inUse; i++)
  {
    SW_props sw;
    SW_Array[i]->get_SW_props(sw);
    if (sw.onBoot)
    {
      Serial.println(i);
      SW_Array[i]->turnON_cb(EXT_1);
    }
  }
}
void post_succes_reboot()
{
  if (firstLoop && iot.isMqttConnected() && iot.now() > 1640803233)
  {
    firstLoop = false;
    restoreSaved_SwState_afterReboot(); /* Read from flash activity, for restore after reboot - in case timeouts are not over */
    On_atBoot();                        /* Start Switches that need to be ON after reboot */

    for (uint8_t i = 0; i < SW_inUse; i++)
    {
      createEntity_post(i);
    }
  }
}

void turnON_notifications(uint8_t i, char *msg, const char *stat, const char *trig)
{
  char msg2[50];
  getPWMval(i, msg2);

  if (SW_Array[i]->telemtryMSG.clk_end != 0)
  {
    char clk[20];
    iot.convert_epoch2clock(SW_Array[i]->get_timeout() / 1000, 0, clk);
    if (SW_Array[i]->get_elapsed() < 1000)
    {
      sprintf(msg, "[%s]: [%s] turned [%s], timeout: [%s] %s", trig, SW_Array[i]->name, stat, clk, msg2);
    }
    else
    {
      sprintf(msg, "[%s]: [%s] updated timeout: [%s]", trig, SW_Array[i]->name, clk);
    }
  }
  else
  {
    sprintf(msg, "[%s]: [%s] turned [%s] %s", trig, SW_Array[i]->name, stat, msg2);
  }
  saveActivity_file(i);
}
void turnOFF_notifications(uint8_t i, char *msg, const char *stat, const char *trig)
{
  if (SW_Array[i]->telemtryMSG.clk_end != 0)
  {
    char clk[25];
    SW_props sw_properties;
    SW_Array[i]->get_SW_props(sw_properties);

    iot.convert_epoch2clock(SW_Array[i]->get_elapsed() / 1000, 0, clk);
    sprintf(msg, "[%s]: [%s] turned [%s], elapsed [%s]", trig, SW_Array[i]->name, stat, clk);
  }
  else
  {
    sprintf(msg, "[%s]: [%s] turned [%s]", trig, SW_Array[i]->name, stat);
  }
  saveActivity_file(i);
}

void smartSW_loop()
{
  if (bootSucceeded)
  {
    for (uint8_t i = 0; i < SW_inUse; i++)
    {
      if (SW_Array[i]->loop())
      {
        if (!SW_Array[i]->is_virtCMD())
        {
          char newmsg[200];
          const char *state[] = {"off", "on"};
          const char *trigs[] = {"Button", "Timeout", "MQTT", "atBoot", "Resume"};

          iot.pub_state(state[SW_Array[i]->telemtryMSG.state], i);

          if (SW_Array[i]->telemtryMSG.state == SW_ON) /*Turn on */
          {
            turnON_notifications(i, newmsg, state[SW_Array[i]->telemtryMSG.state], trigs[SW_Array[i]->telemtryMSG.reason]);
          }
          else if (SW_Array[i]->telemtryMSG.state == SW_OFF) /* Turn off */
          {
            turnOFF_notifications(i, newmsg, state[SW_Array[i]->telemtryMSG.state], trigs[SW_Array[i]->telemtryMSG.reason]);
          }
          iot.pub_msg(newmsg);
          createTelemetry_post(i);
          SW_Array[i]->clear_newMSG();
        }
        else
        {
          SW_props prop;
          SW_Array[i]->get_SW_props(prop);

          createTelemetry_post(i);
          const char *a[]={"on","off"};
          iot.pub_noTopic(a[SW_Array[i]->telemtryMSG.state],prop.name);
          // Serial.println(SW_Array[i]->name());
          // Serial.println(SW_Array[i]->telemtryMSG.state);
          SW_Array[i]->clear_newMSG();
        }
      }
      post_succes_reboot();
    }
  }
}

void startService()
{
  DynamicJsonDocument DOC(JSON_DOC_SIZE);
  init_SW(DOC);
  init_iot2(DOC);
}

void setup()
{
  Serial.begin(115200);
  startService();
}
void loop()
{
  smartSW_loop();
  iot.looper();
}
