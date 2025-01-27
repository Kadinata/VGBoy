target_include_directories(${PROJECT_NAME} PRIVATE
  ./
  callback/
  ring_buf/
)

target_sources(${PROJECT_NAME} PRIVATE
  callback/callback.c
  ring_buf/ring_buf.c
)
