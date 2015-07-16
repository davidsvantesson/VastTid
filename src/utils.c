#include <pebble.h>
#include "utils.h"

  char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

void send_int(uint8_t key, uint8_t cmd)
{
  AppMessageResult resp;
  
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  Tuplet value = TupletInteger(key, cmd);
  dict_write_tuplet(iter, &value);

  resp = app_message_outbox_send();
  if (resp!=APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"send_int failed: %i, %s",resp,translate_error(resp));
  }
}

void send_str(uint8_t key, const char* str) {
  AppMessageResult resp;

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  
  Tuplet value = TupletCString(key, str);
  dict_write_tuplet(iter, &value);
  
  resp = app_message_outbox_send();
  if (resp!=APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"send_str failed: %i, %s",resp,translate_error(resp));
  }
}

bool check_stored_version() {
  char readVersion[6];
  if (!persist_exists(STORAGE_APP_VERSION)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Nothing stored before");
    return false;
  }
  persist_read_string(STORAGE_APP_VERSION,readVersion,6);
  if (strcmp(readVersion,APP_VERSION)==0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Same app version");
    return true;
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Saved setting was for previous version");
  return false;
}

void set_stored_version() {
  persist_write_string(STORAGE_APP_VERSION, APP_VERSION);
}

void set_stored_timestamp(uint32_t timestamp) {
  persist_write_int(STORAGE_TIMESTAMP, timestamp);
}