#include <pebble.h>

static Window *s_window;
static Layer *s_date_layer, *s_time_layer, *s_battery_layer;
static BitmapLayer *s_image_layer, *s_battery_light_layer;
static TextLayer *s_day_label, *s_num_label, *s_ampm_label, *s_digits_label, *s_battery_level_label;
static GBitmap *s_image;
#ifdef PBL_COLOR
static GBitmap *s_battery_image;
#endif
static uint8_t s_hour;
static uint8_t s_minute;
static uint8_t s_second;
static uint16_t s_charge_percent;
static char s_num_buffer[4], s_day_buffer[6], s_ampm_buffer[2], s_battery_level_buffer[12];

static void update_battery(Layer *layer, GContext *ctx) {
  
  #ifdef PBL_COLOR
    
  if (s_charge_percent > 0) {
    if (s_charge_percent <= 20) {
      s_battery_image = gbitmap_create_with_resource(RESOURCE_ID_PROTOMEN_LIGHT_10_20);
    }
    else if (s_charge_percent <= 40) {
      s_battery_image = gbitmap_create_with_resource(RESOURCE_ID_PROTOMEN_LIGHT_20_40);
    }
    else if (s_charge_percent <= 60) {
      s_battery_image = gbitmap_create_with_resource(RESOURCE_ID_PROTOMEN_LIGHT_40_60);
    }
    else if (s_charge_percent <= 80) {
      s_battery_image = gbitmap_create_with_resource(RESOURCE_ID_PROTOMEN_LIGHT_60_80);
    }
    else {
      s_battery_image = gbitmap_create_with_resource(RESOURCE_ID_PROTOMEN_LIGHT_80_100);
    }
  }
  
  bitmap_layer_set_bitmap(s_battery_light_layer, s_battery_image);
  
  #endif
    
  //snprintf(s_battery_level_buffer, sizeof(s_battery_level_buffer), "%d%%", s_charge_percent);
  //text_layer_set_text(s_battery_level_label, s_battery_level_buffer);
  
}


static void update_date(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(s_day_buffer, sizeof(s_day_buffer), "%b", t);
  text_layer_set_text(s_day_label, s_day_buffer);

  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
  text_layer_set_text(s_num_label, s_num_buffer);
  
  strftime(s_ampm_buffer, sizeof(s_ampm_buffer), "%p", t);
  text_layer_set_text(s_ampm_label, s_ampm_buffer);
}

// Update the current time values for the watchface
static void update_time(struct tm *tick_time) {
  s_hour = tick_time->tm_hour;
  s_minute = tick_time->tm_min;
  s_second = tick_time->tm_sec;
  
  // Need to be static because they're used by the system later.
  static char s_time_text[] = "00:00";
  static char s_ampm_text[] = "AM";
  
  char *time_format;
  char *ampm_format;
  
  if (clock_is_24h_style()) {
    time_format = "%R";
    ampm_format = "";
  } else {
    time_format = "%I:%M";
    ampm_format = "%p";
  }
  
  strftime(s_time_text, sizeof(s_time_text), time_format, tick_time);
  strftime(s_ampm_text, sizeof(s_ampm_text), ampm_format, tick_time);

  // Handle lack of non-padded hour format string for twelve hour clock.
  if (!clock_is_24h_style() && (s_time_text[0] == '0')) {
    memmove(s_time_text, &s_time_text[1], sizeof(s_time_text) - 1);
  }
  
  text_layer_set_text(s_digits_label, s_time_text);
  text_layer_set_text(s_ampm_label, s_ampm_text);
  
  layer_set_frame(text_layer_get_layer(s_ampm_label), GRect(text_layer_get_content_size(s_digits_label).w + 12, 14, 30, 24));
  
  layer_mark_dirty(s_time_layer);
  layer_mark_dirty(s_date_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

static void handle_battery(BatteryChargeState charge_state) {
  s_charge_percent = charge_state.charge_percent;
  layer_mark_dirty(s_battery_layer);
}

static void window_load(Window *window) {
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  s_image = gbitmap_create_with_resource(RESOURCE_ID_PROTOMEN_ALBUM_ART);
  s_image_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_image_layer, s_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_image_layer));
  
  //Create time layer
  s_time_layer = layer_create(layer_get_bounds(window_get_root_layer(s_window)));
  layer_add_child(bitmap_layer_get_layer(s_image_layer), s_time_layer);
  
  s_digits_label = text_layer_create(GRect(10, 5, 72, 32));
  text_layer_set_background_color(s_digits_label, GColorClear);
  text_layer_set_text_color(s_digits_label, GColorWhite);
  text_layer_set_font(s_digits_label, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_digits_label, GTextAlignmentLeft);
  text_layer_set_text(s_digits_label, "00:00");
  
  layer_add_child(s_time_layer, text_layer_get_layer(s_digits_label));
  
  s_ampm_label = text_layer_create(GRect(82, 5, 62, 32));
  text_layer_set_background_color(s_ampm_label, GColorClear);
  text_layer_set_text_color(s_ampm_label, GColorWhite);
  text_layer_set_font(s_ampm_label, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_ampm_label, GTextAlignmentLeft);
  text_layer_set_text(s_digits_label, "AM");
  
  layer_add_child(s_time_layer, text_layer_get_layer(s_ampm_label));
  
  //Create date layer
  s_date_layer = layer_create(layer_get_bounds(window_get_root_layer(s_window)));
  layer_set_update_proc(s_date_layer, update_date);
  layer_add_child(bitmap_layer_get_layer(s_image_layer), s_date_layer);

  s_day_label = text_layer_create(GRect(10, 35, 25, 20));
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorClear);
  text_layer_set_text_color(s_day_label, GColorWhite);
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_18));

  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

  s_num_label = text_layer_create(GRect(35, 35, 18, 20));
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, GColorClear);
  text_layer_set_text_color(s_num_label, GColorWhite);
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));
  
  //Create battery layer
  s_battery_layer = layer_create(bounds);
  layer_set_update_proc(s_battery_layer, update_battery);
  layer_add_child(bitmap_layer_get_layer(s_image_layer), s_battery_layer);
  
  s_battery_light_layer = bitmap_layer_create(GRect(92, 58, 22, 26));
  layer_add_child(s_battery_layer, bitmap_layer_get_layer(s_battery_light_layer));
  
  s_battery_level_label = text_layer_create(GRect(99, 8, 40, 18));
  //text_layer_set_text(s_battery_level_label, "--%");
  text_layer_set_background_color(s_battery_level_label, GColorClear);
  text_layer_set_text_color(s_battery_level_label, GColorWhite);
  text_layer_set_text_alignment(s_battery_level_label, GTextAlignmentRight);
  text_layer_set_font(s_battery_level_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

  layer_add_child(s_battery_layer, text_layer_get_layer(s_battery_level_label));
  
  handle_battery(battery_state_service_peek());
  layer_mark_dirty(bitmap_layer_get_layer(s_image_layer));
}

static void window_unload(Window *window) {
  bitmap_layer_destroy(s_image_layer);
  layer_destroy(s_time_layer);
  layer_destroy(s_date_layer);
  layer_destroy(s_battery_layer);
  text_layer_destroy(s_digits_label);
  text_layer_destroy(s_ampm_label);
  text_layer_destroy(s_day_label);
  text_layer_destroy(s_num_label);
}

static void init(void) {
  s_window = window_create();
  
#ifdef PBL_SDK_2
  window_set_fullscreen(s_window, true);
#endif
  
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_set_background_color(s_window, GColorBlack);
  window_stack_push(s_window, true);

  time_t start = time(NULL);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(handle_battery);
  update_time(localtime(&start));
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}