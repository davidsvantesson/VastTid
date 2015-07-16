#pragma once

#define DEPARTUREBOARD_SELECTION_NONE 0
#define DEPARTUREBOARD_SELECTION_FAVORITE 1
#define DEPARTUREBOARD_SELECTION_STOPID 2

#define DEPARTUREBOARD_STATUS_OK 0
#define DEPARTUREBOARD_STATUS_WAITING 1
#define DEPARTUREBOARD_STATUS_NO_DEPARTURES 2
#define DEPARTUREBOARD_STATUS_NO_RESPONSE 3
  
void departueboard_init();
void departureboard_deinit();
void departureboard_process_message(int *key, uint32_t *value, char *string_value);

void departueboard_activate_favorite(uint8_t favoriteId, char * title);
void departueboard_activate_stopId(char *stopId, char * title);