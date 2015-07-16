#pragma once

#define NEARBY_STATUS_OK 0
#define NEARBY_STATUS_WAITING 1
#define NEARBY_STATUS_NO_GPS_RETRY 2
#define NEARBY_STATUS_NO_GPS_FINAL 3
#define NEARBY_STATUS_NO_STOPS_FOUND 4

void stopBoardDeInit();
void stopBoard_init();
void favorites_process_message(int *key, uint32_t *value, char *string_value);
void nearby_process_message(int *key, uint32_t *value, char *string_value);

void stopBoard_setActive();

void load_favorites();
