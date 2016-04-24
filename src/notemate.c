#include <pebble.h>

static Window *s_window;
static TextLayer *s_text_layer;

static DictationSession *s_dictation_session;
static char s_last_text[256];
static bool phone_ready = false;

static int sendMsg(char* buf) {
  APP_LOG(APP_LOG_LEVEL_INFO, "sendMsg() init");
  DictionaryIterator *out_iter;
  if (!phone_ready) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Phone Pebble JS is not ready yet");
    return 1;
  }
  
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  if(result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
    return 1;
  }
  
  dict_write_cstring(out_iter, 1, buf);

  result = app_message_outbox_send();
  if(result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
    return 2;
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "sendMsg() fin");
  return 0;
}

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, 
                                       char *transcription, void *context) {
  if(status == DictationSessionStatusSuccess) {
    // Display the dictated text
    snprintf(s_last_text, sizeof(s_last_text), "Transcription:\n\n%s", transcription);
    text_layer_set_text(s_text_layer, s_last_text);
    sendMsg(transcription);
  } else {
    // Display the reason for any error
    static char s_failed_buff[128];
    snprintf(s_failed_buff, sizeof(s_failed_buff), "Transcription failed.\n\nError ID:\n%d", (int)status);
    text_layer_set_text(s_text_layer, s_failed_buff);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  //sendMsg("Helloworld");
  dictation_session_start(s_dictation_session);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  // A message was received, but had to be dropped
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}
static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  // A new message has been successfully received
  

//        foo(tuple->value->data, tuple->length);
//        bar(tuple->value->cstring);

  Tuple *tuple = dict_read_first(iter);
  while (tuple) {
    switch (tuple->key) {
      case 0: //AppKeyJSReady:
        APP_LOG(APP_LOG_LEVEL_INFO, "PebbleKit JS ready");
        phone_ready = true;
        break;
      case 1: //AppKeyNote:
        APP_LOG(APP_LOG_LEVEL_INFO, "Got note from server (%s)", tuple->value->cstring);
        snprintf(s_last_text, sizeof(s_last_text), "Response:\n\n%s", tuple->value->cstring);
        text_layer_set_text(s_text_layer, s_last_text);
        break;
      case 2: //AppKeyResponseFailure:
        APP_LOG(APP_LOG_LEVEL_INFO, "Got response failure");
        snprintf(s_last_text, sizeof(s_last_text), "Server response failure");
        text_layer_set_text(s_text_layer, s_last_text);
        break;
    }
    tuple = dict_read_next(iter);
  }
}

static void outbox_sent_callback(DictionaryIterator *iter, void *context) {
  // The message just sent has been successfully delivered
  APP_LOG(APP_LOG_LEVEL_INFO, "Message send OK");
}
static void outbox_failed_callback(DictionaryIterator *iter,
                                      AppMessageResult reason, void *context) {
  // The message just sent failed to be delivered
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message send failed. Reason: %d", (int)reason);
}


static void init(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Start init()");
  const uint32_t inbox_size = 64;
  const uint32_t outbox_size = 256;
  
  // Open AppMessage
  app_message_open(inbox_size, outbox_size);
  
  // Register to be notified about inbox received events
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  
	// Create a window and get information about the window
	s_window = window_create();
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_click_config_provider(s_window, click_config_provider);
  //window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);

  // Create a text layer and set the text
//  s_text_layer = text_layer_create(GRect(bounds.origin.x, (bounds.size.h - 24) / 2, bounds.size.w, bounds.size.h));
  s_text_layer = text_layer_create(bounds);
	text_layer_set_text(s_text_layer, "Hi, I'm a Pebble!");
  
  // Set the font and text alignment
	text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);

	// Add the text layer to the window
	layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_text_layer));
  
  // Enable text flow and paging on the text layer, with a slight inset of 10, for round screens
  text_layer_enable_screen_text_flow_and_paging(s_text_layer, 10);

	// Push the window, setting the window animation to 'true'
	window_stack_push(s_window, true);
	
	// App Logging!
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Just pushed a window!");



  // Create new dictation session
  s_dictation_session = dictation_session_create(sizeof(s_last_text), dictation_session_callback, NULL);
  APP_LOG(APP_LOG_LEVEL_INFO, "Finish init()");

}

static void deinit(void) {
	// Destroy the text layer
	text_layer_destroy(s_text_layer);
  // Free the last session data
  dictation_session_destroy(s_dictation_session);
	
	// Destroy the window
	window_destroy(s_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}


