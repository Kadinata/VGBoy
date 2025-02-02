target_include_directories(${PROJECT_NAME} PRIVATE
  include/
)

target_sources(${PROJECT_NAME} PRIVATE
  src/audio.c
  src/display.c
  src/file_manager.c
  src/fps_sync.c
  src/key_input.c
  src/main_window.c
  src/save_state.c
  src/tile_debug_window.c
  src/window_manager.c
)
