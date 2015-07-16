#include <pebble.h>

#include "utils.h"
#include "departureboard.h"
#include "stopboard.h"


uint32_t storageTimestamp;




void process_tuple(Tuple *t)
{
  int key = t->key;
  uint32_t value = t->value->uint32;
  char string_value[32];
  strcpy(string_value, t->value->cstring);
  
  if (key<10) departureboard_process_message(&key,&value,string_value);
  else if (key<20) favorites_process_message(&key,&value,string_value);
  else if (key<30) nearby_process_message(&key, &value, string_value);
  else if (key==KEY_PHONE_STARTUP) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Phone settings timestamp: %i, storageTimestamp:%i",(int)value,(int)storageTimestamp);
    if (value > storageTimestamp) {
        send_int(KEY_REQUEST_SETTINGS,storageTimestamp);
    }
  }
}


static void in_received_handler(DictionaryIterator *iter, void *context)
{
	//Get data
  Tuple *t = dict_read_first(iter);
  if(t)
  {
    process_tuple(t);
  }

  //Get next
  while(t != NULL)
  {
    t = dict_read_next(iter);
    if(t)
    {
      process_tuple(t);
    }
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Out failed: %i: %s", reason, translate_error(reason));
}

static void out_failed_handler(DictionaryIterator *iter,AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App message send failed: %i: %s",reason, translate_error(reason));
}

void load_settings() {
  
  if (!check_stored_version()) {
    storageTimestamp = 0;
    // Favorites / settings only cached for the same version
    //APP_LOG(APP_LOG_LEVEL_DEBUG,"No stored settings to use, request.");
    //send_int(KEY_REQUEST_SETTINGS, 0);  // No point to send if phone not initated
  }
  else {
    //APP_LOG(APP_LOG_LEVEL_DEBUG,"Use stored settings.");
    storageTimestamp = persist_read_int(STORAGE_TIMESTAMP);
    //APP_LOG(APP_LOG_LEVEL_DEBUG,"Storage timestamp: %i",(int) storageTimestamp);
    //send_int(KEY_REQUEST_SETTINGS,storageTimestamp); // No point to send if phone not initiated
    load_favorites();
  }
}

void init()
{
  
  stopBoard_init();
  departueboard_init();
  
	//Register AppMessage events
	app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  //app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());   //Large input and output buffer sizes

  load_settings();
  
  stopBoard_setActive();  

}

void deinit()
{
  stopBoardDeInit();
  departureboard_deinit();
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}