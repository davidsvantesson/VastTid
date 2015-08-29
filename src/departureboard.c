#include <pebble.h>
#include "departureboard.h"
#include "utils.h"

#define MAX_DEPARTURE_LINES 30

Window *departureBoardWindow;
MenuLayer *departureBoardLayer;

char departureBoardTitle[32];
char routeNr[MAX_DEPARTURE_LINES][7];
char direction[MAX_DEPARTURE_LINES][32];
char minutesLeft[MAX_DEPARTURE_LINES][8];
uint8_t fgColor[MAX_DEPARTURE_LINES];
uint8_t bgColor[MAX_DEPARTURE_LINES];
int nrDepartures = 0;
char  reloadStatus[14] = "Updated XX:XX";

// HEADER MAX                               "                "
// SUB MAX                                  "                        "
#define DEPARTUREBOARD_OK_HEAD              "Status: OK"
#define DEPARTUREBOARD_OK_SUB               "Reload each min / press"
#define DEPARTUREBOARD_WAITING_HEAD         "Status: Fetching"
#define DEPARTUREBOARD_WAITING_SUB          "Wait..."
#define DEPARTUREBOARD_NO_DEPARTURES_HEAD   "Status: No dep."
#define DEPARTUREBOARD_NO_DEPARTURES_SUB    "No departures in 60 min."
#define DEPARTUREBOARD_NO_RESPONSE_HEAD     "Status: No resp."
#define DEPARTUREBOARD_NO_RESPONSE_SUB      "Failure fetch. departur."
uint8_t departureBoard_status;

uint8_t selectedFavorite = 0;
char selectedStopId[17];
uint8_t selectedType = 0;

void departureBoardWindow_load(Window *window);
void departureBoardWindow_unload(Window *window);
void departureboard_update(struct tm *tick_time, TimeUnits units_changed);

#define RouteNrPosX 3
#define RouteNrPosY 3
#define RouteNrSizeW 40
#define RouteNrSizeH 20
#define DirectionPosX 5
#define DirectionPosY 22
#define DirectionSizeW 144-DirectionPosX-5
#define DirectionSizeH 20
#define MinutesLeftPosX 45 //RouteNrPosX+RouteNrSizeW+2
#define MinutesLeftPosY 1
#define MinutesLeftSizeW 144-MinutesLeftPosX-7
#define MinutesLeftSizeH 20
GRect RouteNrFill;
GRect RouteNrText;
GRect DirectionText;
GRect MinutesLeftText;

void departueboard_init()
{
  RouteNrFill = GRect(RouteNrPosX, RouteNrPosY, RouteNrSizeW, RouteNrSizeH);
  RouteNrText = GRect(RouteNrPosX,RouteNrPosY-3,RouteNrSizeW,RouteNrSizeH);
  DirectionText = GRect(DirectionPosX,DirectionPosY,DirectionSizeW,DirectionSizeH);
  MinutesLeftText = GRect(MinutesLeftPosX,MinutesLeftPosY,MinutesLeftSizeW,MinutesLeftSizeH);

  departureBoardWindow = window_create();
  window_set_window_handlers(departureBoardWindow, (WindowHandlers) {
    .load = departureBoardWindow_load,
    .unload = departureBoardWindow_unload
  });

}

void departureboard_deinit()
{
  window_destroy(departureBoardWindow);
}

void departureboard_process_message(int *key, uint32_t *value, char *string_value) {
  static int departure_i = 0;
  static int internal_c = 0;

  switch(*key) {
    case KEY_DEPARTUREBOARD_ROUTENR:
    case KEY_DEPARTUREBOARD_TIME:
    case KEY_DEPARTUREBOARD_DIRECTION:
    case KEY_DEPARTUREBOARD_FGCOLOR:
    case KEY_DEPARTUREBOARD_BGCOLOR:
      internal_c++;

      if (internal_c==6) {
        if (departure_i<MAX_DEPARTURE_LINES-1) departure_i++;
        internal_c = 1;
      }
      break;
    default:
      break;
  }

    switch(*key) {
      case KEY_DEPARTUREBOARD_STATUS:
        departureBoard_status = *value;
        switch (*value) {
          case DEPARTUREBOARD_STATUS_NO_DEPARTURES:
            APP_LOG(APP_LOG_LEVEL_DEBUG,"No departures");
          case DEPARTUREBOARD_STATUS_NO_RESPONSE:
            nrDepartures = 0;

          if (window_is_loaded(departureBoardWindow)) {
              menu_layer_reload_data(departureBoardLayer);
          }
          break;
        }
        break;

      case KEY_DEPARTUREBOARD_NAME:
        departure_i = 0;
        internal_c = 0;
        strcpy(departureBoardTitle,string_value);
        break;

     case KEY_DEPARTUREBOARD_ROUTENR:
        strcpy(routeNr[departure_i],string_value);
        break;
      case KEY_DEPARTUREBOARD_TIME:
        strcpy(minutesLeft[departure_i],string_value);
        break;
      case KEY_DEPARTUREBOARD_DIRECTION:
        strcpy(direction[departure_i],string_value);
        break;
      case KEY_DEPARTUREBOARD_FGCOLOR:
        fgColor[departure_i] = *value;
        //APP_LOG(APP_LOG_LEVEL_DEBUG,"FG Color received: %i",fgColor[departure_i]);
        break;
      case KEY_DEPARTUREBOARD_BGCOLOR:
        bgColor[departure_i] = *value;
        //APP_LOG(APP_LOG_LEVEL_DEBUG,"BG Color received: %i",bgColor[departure_i]);
        break;

      case KEY_DEPARTUREBOARD_COMPLETE:
        nrDepartures = departure_i+1;
        strncpy(reloadStatus+8,string_value,5);
        departureBoard_status = DEPARTUREBOARD_STATUS_OK;
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Reload departureboard, departures: %i",nrDepartures);
        if (window_is_loaded(departureBoardWindow)) {
          menu_layer_reload_data(departureBoardLayer);
        }
        break;
    }
}

uint16_t departureBoard_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 1;
}

uint16_t departureBoard_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return nrDepartures+1;
}

int16_t departureBoard_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return 22;
}

void departureBoard_draw_header_callback (GContext* ctx, const Layer *cell_layer,  uint16_t section_index, void *data) {
  menu_cell_basic_header_draw(ctx, cell_layer, departureBoardTitle);
}

int16_t departureBoard_get_row_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return 44;
}




void departureBoard_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->row > 0) { // nrDepartures){
      graphics_context_set_fill_color(ctx,(GColor) fgColor[cell_index->row -1]);
      graphics_fill_rect(ctx,RouteNrFill,4,GCornersAll);
      graphics_context_set_text_color(ctx,(GColor) bgColor[cell_index->row -1]);
      graphics_draw_text(ctx,routeNr[cell_index->row -1],fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),RouteNrText,GTextOverflowModeWordWrap,GTextAlignmentCenter,NULL);

      graphics_context_set_text_color(ctx,GColorBlack);
      graphics_draw_text(ctx,direction[cell_index->row -1],fonts_get_system_font(FONT_KEY_GOTHIC_18),DirectionText,GTextOverflowModeWordWrap,GTextAlignmentLeft,NULL);
      graphics_draw_text(ctx,minutesLeft[cell_index->row -1],fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),MinutesLeftText,GTextOverflowModeWordWrap,GTextAlignmentRight,NULL );

  }
  else if (cell_index->row == 0) {
    switch(departureBoard_status) {
      case DEPARTUREBOARD_STATUS_OK:
        menu_cell_basic_draw(ctx, cell_layer, reloadStatus, DEPARTUREBOARD_OK_SUB, NULL);
        break;
      case DEPARTUREBOARD_STATUS_WAITING:
        menu_cell_basic_draw(ctx, cell_layer, DEPARTUREBOARD_WAITING_HEAD, DEPARTUREBOARD_WAITING_SUB, NULL);
        break;
      case DEPARTUREBOARD_STATUS_NO_DEPARTURES:
        menu_cell_basic_draw(ctx, cell_layer, DEPARTUREBOARD_NO_DEPARTURES_HEAD, DEPARTUREBOARD_NO_DEPARTURES_SUB, NULL);
        break;
      case DEPARTUREBOARD_STATUS_NO_RESPONSE:
        menu_cell_basic_draw(ctx, cell_layer, DEPARTUREBOARD_NO_RESPONSE_HEAD, DEPARTUREBOARD_NO_RESPONSE_SUB, NULL);
        break;
    }


  }



}

void departureBoard_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // so far only reload
  if (cell_index->row==0) { //nrDepartures) {
    departureboard_update(NULL,MINUTE_UNIT);
    departureBoard_status = DEPARTUREBOARD_STATUS_WAITING;
    menu_layer_reload_data(departureBoardLayer);
  }
}


void departureBoardWindow_load(Window *window)
{
  nrDepartures = 0;

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  departureBoardLayer = menu_layer_create(bounds);
  menu_layer_set_callbacks(departureBoardLayer, NULL, (MenuLayerCallbacks){
    .get_num_sections = departureBoard_get_num_sections_callback,
    .get_num_rows = departureBoard_get_num_rows_callback,
    .get_header_height = departureBoard_get_header_height_callback,
    .draw_header = departureBoard_draw_header_callback,
    .get_cell_height = departureBoard_get_row_height_callback,
    .draw_row = departureBoard_draw_row_callback,
    .select_click = departureBoard_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(departureBoardLayer, window);
#ifdef PBL_COLOR
  menu_layer_set_normal_colors(departureBoardLayer,GColorWhite,GColorBlack);
  menu_layer_set_highlight_colors(departureBoardLayer,GColorElectricBlue,GColorBlack);
#endif


  layer_add_child(window_layer, menu_layer_get_layer(departureBoardLayer));

  tick_timer_service_subscribe(MINUTE_UNIT, departureboard_update);
}

void departureBoardWindow_unload(Window *window)
{
  tick_timer_service_unsubscribe();
  menu_layer_destroy(departureBoardLayer);
}

void departureboard_update(struct tm *tick_time, TimeUnits units_changed)
{
  switch(selectedType) {
    case DEPARTUREBOARD_SELECTION_FAVORITE:
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Send request for favorite %i",selectedFavorite);
    send_int(KEY_REQUEST_FAVORITE_DEPARTUREBOARD, selectedFavorite);
    break;

    case DEPARTUREBOARD_SELECTION_STOPID:
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Send request for nearby, stop id %s", selectedStopId);
    send_str(KEY_REQUEST_NEARBY_DEPARTUREBOARD,selectedStopId);
    break;
  }
}

void departueboard_activate_favorite(uint8_t favoriteId, char * title) {
      strcpy(departureBoardTitle,title);
      departureBoard_status = DEPARTUREBOARD_STATUS_WAITING;
      selectedType = DEPARTUREBOARD_SELECTION_FAVORITE;
      selectedFavorite = favoriteId;
      window_stack_push(departureBoardWindow, true);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Initiated and sending request for favorite %i",selectedFavorite);
      send_int(KEY_REQUEST_FAVORITE_DEPARTUREBOARD, selectedFavorite);
}

void departueboard_activate_stopId(char *stopId, char * title) {
      strcpy(departureBoardTitle,title);
      departureBoard_status = DEPARTUREBOARD_STATUS_WAITING;
      selectedType = DEPARTUREBOARD_SELECTION_STOPID;
      strcpy(selectedStopId,stopId);
      window_stack_push(departureBoardWindow, true);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Initiated and sending request for nearby, stop id %s", selectedStopId);
      send_str(KEY_REQUEST_NEARBY_DEPARTUREBOARD,selectedStopId);
}
