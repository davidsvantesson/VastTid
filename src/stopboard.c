#include <pebble.h>
#include "stopboard.h"
#include "utils.h"
#include "departureboard.h"

#define MAX_FAVORITES 10
#define MAX_NEARBY 10

Window *stopBoardWindow;
MenuLayer *stopBoardLayer;

char favoriteName[MAX_FAVORITES][32];
char favoriteDirection1[MAX_FAVORITES][32];
char favoriteDirection2[MAX_FAVORITES][32];
char favoriteDirection3[MAX_FAVORITES][32];
int8_t nrFavorites = 0;

char nearbyName[MAX_NEARBY][32];
char nearbyId[MAX_NEARBY][17];
char nearbySubtext[MAX_NEARBY][17];
int8_t nrNearby;

// HEADER MAX                     "                "
// SUB MAX                        "                        "
#define NEARBY_WAITING_HEAD       "Status: Fetching"
#define NEARBY_WAITING_SUB        "Wait..."
#define NEARBY_OK_HEAD            "Status: OK"
#define NEARBY_OK_SUB             "Press to re-scan nearby"
#define NEARBY_NO_GPS_RETRY_HEAD  "Status: No pos."
#define NEARBY_NO_GPS_RETRY_SUB   "Retrying to find gps pos"
#define NEARBY_NO_GPS_FINAL_HEAD  "Status: No pos."
#define NEARBY_NO_GPS_FINAL_SUB   "GPS fail. Press to retry"
#define NEARBY_NO_STOPS_FOUND_HEAD "Status: No found"
#define NEARBY_NO_STOPS_FOUND_SUB "No stops (3km). Retry?"
int8_t nearbyStatus;

void stopBoardWindow_load(Window *window);
void stopBoardWindow_unload(Window *window);
void save_favorites(uint32_t timestamp);

void stopBoardDeInit() {
  window_destroy(stopBoardWindow);
}

void stopBoard_init()
{
  nrNearby = 0;
  nearbyStatus = NEARBY_STATUS_WAITING;

  stopBoardWindow = window_create();
  window_set_window_handlers(stopBoardWindow, (WindowHandlers) {
    .load = stopBoardWindow_load,
    .unload = stopBoardWindow_unload
  });
}


void stopBoard_setActive(){
  window_stack_push(stopBoardWindow, true);
}

void favorites_process_message(int *key, uint32_t *value, char *string_value)
{
  static int directionNr = 0;
  static int favorite_i = 0;

  switch(*key) {
    case KEY_FAVORITES_INIT:
      favorite_i = -1;
      break;
    case KEY_FAVORITES_NAME:
      if (favorite_i<MAX_FAVORITES-1) favorite_i++;
      strcpy(favoriteName[favorite_i],string_value);
      favoriteDirection1[favorite_i][0] = 0;
      favoriteDirection2[favorite_i][0] = 0;
      favoriteDirection3[favorite_i][0] = 0;
      directionNr = 0;
      break;
    case KEY_FAVORITES_DIRECTION:
      switch(directionNr) {
        case 0:
          strcpy(favoriteDirection1[favorite_i],string_value);
          break;
        case 1:
          strcpy(favoriteDirection2[favorite_i],string_value);
          break;
        case 2:
          strcpy(favoriteDirection3[favorite_i],string_value);
          break;
      }
      directionNr++;
      break;
    case KEY_FAVORITES_COMPLETE:
      nrFavorites = favorite_i+1;
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Reloading favorite menu, nr of favorites: %i", (int) nrFavorites );
      if (window_is_loaded(stopBoardWindow)) {
        menu_layer_reload_data(stopBoardLayer);
      }
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Caching favorites in persistant memory, timestamp: %i",(int) *value);
      save_favorites(*value);
      break;
  }
}

void nearby_process_message(int *key, uint32_t *value, char *string_value)
{
  static int nearby_i = 0;
  static int internal_c = 0;

  switch(*key) {
    case KEY_NEARBY_NAME:
    case KEY_NEARBY_DISTANCE:
    case KEY_NEARBY_ID:
      internal_c++;

      if (internal_c==4) {
        if (nearby_i<MAX_NEARBY-1) nearby_i++;
        internal_c = 1;
      }
      break;
    default:
      break;
  }

  switch(*key) {
    case KEY_NEARBY_STATUS:
      nearbyStatus = *value;
      switch (*value) {
        case NEARBY_STATUS_NO_GPS_RETRY:
        case NEARBY_STATUS_NO_GPS_FINAL:
        case NEARBY_STATUS_NO_STOPS_FOUND:
          nrNearby = 0;
          if (window_is_loaded(stopBoardWindow)) {
            menu_layer_reload_data(stopBoardLayer);
          }
          break;
      }
      break;
    case KEY_NEARBY_INIT:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Nearby init");
      nearby_i = 0;
      internal_c = 0;
      break;
    case KEY_NEARBY_NAME:
      strcpy(nearbyName[nearby_i],string_value);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Nearby station (%i): %s",nearby_i,nearbyStationName[nearby_i]);
      break;
    case KEY_NEARBY_DISTANCE:
      strcpy(nearbySubtext[nearby_i],string_value);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Nearby distance (%i): %s",nearby_i,nearbyStationSubtext[nearby_i]);
      break;
    case KEY_NEARBY_ID:
      strcpy(nearbyId[nearby_i],string_value);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Nearby id (%i): %s",nearby_i,nearbyStationId[nearby_i]);
      break;
    case KEY_NEARBY_COMPLETE:
      nrNearby = nearby_i+1;
      nearbyStatus = NEARBY_STATUS_OK;
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Nearby size: %i / %i",nearby_i,nrNearbyStations);
      if (window_is_loaded(stopBoardWindow)) {
        menu_layer_reload_data(stopBoardLayer);
      }
      break;
  }
}

uint16_t stopBoard_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 2;
}

uint16_t stopBoard_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      return nrFavorites;
      break;
    case 1:
      return nrNearby+1;
      break;
    default:
      return 0;
      break;
  }
}

int16_t stopBoard_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      if (nrFavorites==0) return 0;
      else return 22;
      break;
    case 1:
    default:
      return 22;
      break;
  }
}

void stopBoard_draw_header_callback (GContext* ctx, const Layer *cell_layer,  uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      menu_cell_basic_header_draw(ctx, cell_layer, "Favorite stops");
      break;
    case 1:
      menu_cell_basic_header_draw(ctx, cell_layer, "Nearby stops");
      break;
  }
}

int8_t get_nr_favorite_directions(int8_t favoriteNr) {
  if (strlen(favoriteDirection1[favoriteNr])==0) return 0;
  if (strlen(favoriteDirection2[favoriteNr])==0) return 1;
  if (strlen(favoriteDirection3[favoriteNr])==0) return 2;
  return 3;
}

int16_t stopBoard_get_row_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 0:
      switch (get_nr_favorite_directions(cell_index->row)) {
        case 0: return 26; break;
        case 1: return 48; break;
        case 2: return 67; break;
        case 3: return 86; break;
        return 44;
        break;
      }
      break;
    case 1:
      return 44;
      break;
  }
  return 0;
}

void stopBoard_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 0:

      switch (get_nr_favorite_directions(cell_index->row)) {
       case 3:
          graphics_draw_text(ctx, favoriteDirection3[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(5, 63, 139, 18), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
        case 2:
          graphics_draw_text(ctx, favoriteDirection2[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(5, 44, 139, 18), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
        case 1:
          graphics_draw_text(ctx, favoriteDirection1[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(5, 25, 139, 18), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
          graphics_draw_text(ctx, favoriteName[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(5, 0, 139, 24), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
          //menu_cell_basic_draw(ctx, cell_layer, favoriteStationName[cell_index->row],favoriteStationDirection1[cell_index->row],NULL );
          break;
        case 0:
        default:
          menu_cell_basic_draw(ctx, cell_layer, favoriteName[cell_index->row], NULL, NULL);
          break;
      }
      break;
    case 1:
      if (cell_index->row > 0) { //<nrNearby){
        menu_cell_basic_draw(ctx, cell_layer, nearbyName[cell_index->row-1], nearbySubtext[cell_index->row-1], NULL);
      }
      else if (cell_index->row == 0) { //==nrNearby) {
        //Status
        switch (nearbyStatus) {
          case NEARBY_STATUS_OK:
            menu_cell_basic_draw(ctx, cell_layer, NEARBY_OK_HEAD, NEARBY_OK_SUB, NULL);
            break;
          case NEARBY_STATUS_WAITING:
            menu_cell_basic_draw(ctx, cell_layer, NEARBY_WAITING_HEAD, NEARBY_WAITING_SUB, NULL);
            break;
          case NEARBY_STATUS_NO_GPS_RETRY:
            menu_cell_basic_draw(ctx, cell_layer, NEARBY_NO_GPS_RETRY_HEAD, NEARBY_NO_GPS_RETRY_SUB, NULL);
            break;
          case NEARBY_STATUS_NO_GPS_FINAL:
            menu_cell_basic_draw(ctx, cell_layer, NEARBY_NO_GPS_FINAL_HEAD, NEARBY_NO_GPS_FINAL_SUB, NULL);
            break;
          case NEARBY_STATUS_NO_STOPS_FOUND:
            menu_cell_basic_draw(ctx, cell_layer, NEARBY_NO_STOPS_FOUND_HEAD, NEARBY_NO_STOPS_FOUND_SUB, NULL);
            break;
        }

      }
      break;
  }
}

void stopBoard_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 0:
      departueboard_activate_favorite(cell_index->row,favoriteName[cell_index->row]);
      break;
    case 1:
      if (cell_index->row > 0) { //<nrNearby){
        departueboard_activate_stopId(nearbyId[cell_index->row -1],nearbyName[cell_index->row -1]);
      }
      else if (cell_index->row == 0 && nearbyStatus != NEARBY_STATUS_WAITING) {
        send_int(KEY_REQUEST_NEARBY_STOPS, 1);
        nearbyStatus = NEARBY_STATUS_WAITING;
        menu_layer_reload_data(stopBoardLayer);
      }
      break;
  }

}

void stopBoardWindow_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  stopBoardLayer = menu_layer_create(bounds);
  menu_layer_set_callbacks(stopBoardLayer, NULL, (MenuLayerCallbacks){
    .get_num_sections = stopBoard_get_num_sections_callback,
    .get_num_rows = stopBoard_get_num_rows_callback,
    .get_header_height = stopBoard_get_header_height_callback,
    .draw_header = stopBoard_draw_header_callback,
    .get_cell_height = stopBoard_get_row_height_callback,
    .draw_row = stopBoard_draw_row_callback,
    .select_click = stopBoard_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(stopBoardLayer, window);
#ifdef PBL_COLOR
  menu_layer_set_normal_colors(stopBoardLayer,GColorWhite,GColorOxfordBlue);
  menu_layer_set_highlight_colors(stopBoardLayer,GColorDukeBlue,GColorWhite);
#endif


  layer_add_child(window_layer, menu_layer_get_layer(stopBoardLayer));
}

void stopBoardWindow_unload(Window *window) {
  menu_layer_destroy(stopBoardLayer);
}

void load_favorites() {
  nrFavorites = persist_read_int(STORAGE_NR_FAVORITES);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Nr of saved favorites: %i", nrFavorites);
  for (int i=0; i<nrFavorites; i++) {
    persist_read_string(STORAGE_FAVORITE_NAME_BASE+i,favoriteName[i],32);
    persist_read_string(STORAGE_FAVORITE_DIRECTION1_BASE+i,favoriteDirection1[i],32);
    persist_read_string(STORAGE_FAVORITE_DIRECTION2_BASE+i,favoriteDirection2[i],32);
    persist_read_string(STORAGE_FAVORITE_DIRECTION3_BASE+i,favoriteDirection3[i],32);
  }
}

void save_favorites(uint32_t timestamp) {
  set_stored_version();
  set_stored_timestamp(timestamp);
  persist_write_int(STORAGE_NR_FAVORITES, nrFavorites);
  for (int i=0; i<nrFavorites; i++) {
    persist_write_string(STORAGE_FAVORITE_NAME_BASE+i, favoriteName[i]);
    persist_write_string(STORAGE_FAVORITE_DIRECTION1_BASE+i,favoriteDirection1[i]);
    persist_write_string(STORAGE_FAVORITE_DIRECTION2_BASE+i,favoriteDirection2[i]);
    persist_write_string(STORAGE_FAVORITE_DIRECTION3_BASE+i,favoriteDirection3[i]);
  }
}
